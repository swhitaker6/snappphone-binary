#!/usr/bin/env python3
# Copyright 2012 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.

import os
import sys
import subprocess
import re
import json
import shutil

__scriptdir__ = os.path.dirname(os.path.abspath(__file__))
__rootdir__ = os.path.dirname(__scriptdir__)
sys.path.append(__rootdir__)

from tools.toolchain_profiler import ToolchainProfiler
from tools.utils import path_from_root
from tools import building, config, shared, utils

temp_files = shared.get_temp_files()


ACORN_OPTIMIZER = path_from_root('tools/acorn-optimizer.js')

NUM_CHUNKS_PER_CORE = 3
MIN_CHUNK_SIZE = int(os.environ.get('EMCC_JSOPT_MIN_CHUNK_SIZE') or 512 * 1024) # configuring this is just for debugging purposes
MAX_CHUNK_SIZE = int(os.environ.get('EMCC_JSOPT_MAX_CHUNK_SIZE') or 5 * 1024 * 1024)

WINDOWS = sys.platform.startswith('win')

DEBUG = os.environ.get('EMCC_DEBUG')

func_sig = re.compile(r'function ([_\w$]+)\(')
func_sig_json = re.compile(r'\["defun", ?"([_\w$]+)",')
import_sig = re.compile(r'(var|const) ([_\w$]+ *=[^;]+);')


def split_funcs(js, just_split=False):
  if just_split:
    return [('(json)', line) for line in js.splitlines()]
  # split properly even if there are no newlines,
  # which is important for deterministic builds (as which functions
  # are in each chunk may differ, so we need to split them up and combine
  # them all together later and sort them deterministically)
  parts = ['function ' + part for part in js.split('function ')[1:]]
  funcs = []
  for func in parts:
    m = func_sig.search(func)
    if not m:
      continue
    ident = m.group(1)
    assert ident
    funcs.append((ident, func))
  return funcs


class Minifier:
  """minification support. We calculate minification of
  globals here, then pass that into the parallel acorn-optimizer.js runners which
  perform minification of locals.
  """

  def __init__(self, js):
    self.js = js
    self.symbols_file = None
    self.profiling_funcs = False

  def minify_shell(self, shell, minify_whitespace):
    # Run through acorn-optimizer.js to find and minify the global symbols
    # We send it the globals, which it parses at the proper time. JS decides how
    # to minify all global names, we receive a dictionary back, which is then
    # used by the function processors

    shell = shell.replace('0.0', '13371337') # avoid optimizer doing 0.0 => 0

    # Find all globals in the JS functions code

    if not self.profiling_funcs:
      self.globs = [m.group(1) for m in func_sig.finditer(self.js)]
      if len(self.globs) == 0:
        self.globs = [m.group(1) for m in func_sig_json.finditer(self.js)]
    else:
      self.globs = []

    with temp_files.get_file('.minifyglobals.js') as temp_file:
      with open(temp_file, 'w') as f:
        f.write(shell)
        f.write('\n')
        f.write('// EXTRA_INFO:' + json.dumps(self.serialize()))

      cmd = config.NODE_JS + [ACORN_OPTIMIZER, temp_file, 'minifyGlobals']
      if minify_whitespace:
        cmd.append('minifyWhitespace')
      output = shared.run_process(cmd, stdout=subprocess.PIPE).stdout

    assert len(output) and not output.startswith('Assertion failed'), 'Error in js optimizer: ' + output
    code, metadata = output.split('// EXTRA_INFO:')
    self.globs = json.loads(metadata)

    if self.symbols_file:
      with open(self.symbols_file, 'w') as f:
        for key, value in self.globs.items():
          f.write(value + ':' + key + '\n')
      print('wrote symbol map file to', self.symbols_file, file=sys.stderr)

    return code.replace('13371337', '0.0')

  def serialize(self):
    return {
      'globals': self.globs
    }


start_funcs_marker = '// EMSCRIPTEN_START_FUNCS\n'
end_funcs_marker = '// EMSCRIPTEN_END_FUNCS\n'
start_asm_marker = '// EMSCRIPTEN_START_ASM\n'
end_asm_marker = '// EMSCRIPTEN_END_ASM\n'


# Given a set of functions of form (ident, text), and a preferred chunk size,
# generates a set of chunks for parallel processing and caching.
@ToolchainProfiler.profile()
def chunkify(funcs, chunk_size):
  chunks = []
  # initialize reasonably, the rest of the funcs we need to split out
  curr = []
  total_size = 0
  for i in range(len(funcs)):
    func = funcs[i]
    curr_size = len(func[1])
    if total_size + curr_size < chunk_size:
      curr.append(func)
      total_size += curr_size
    else:
      chunks.append(curr)
      curr = [func]
      total_size = curr_size
  if curr:
    chunks.append(curr)
    curr = None
  return [''.join(func[1] for func in chunk) for chunk in chunks] # remove function names


def run_on_js(filename, passes, extra_info=None, just_split=False, just_concat=False):
  with ToolchainProfiler.profile_block('js_optimizer.split_markers'):
    if not isinstance(passes, list):
      passes = [passes]

    js = utils.read_file(filename)
    if os.linesep != '\n':
      js = js.replace(os.linesep, '\n') # we assume \n in the splitting code

    # Find suffix
    suffix_marker = '// EMSCRIPTEN_GENERATED_FUNCTIONS'
    suffix_start = js.find(suffix_marker)
    suffix = ''
    if suffix_start >= 0:
      suffix_end = js.find('\n', suffix_start)
      suffix = js[suffix_start:suffix_end] + '\n'
      # if there is metadata, we will run only on the generated functions. If there isn't, we will run on everything.

    # Find markers
    start_funcs = js.find(start_funcs_marker)
    end_funcs = js.rfind(end_funcs_marker)

    if start_funcs < 0 or end_funcs < start_funcs or not suffix:
      shared.exit_with_error('Invalid input file. Did not contain appropriate markers. (start_funcs: %s, end_funcs: %s, suffix_start: %s' % (start_funcs, end_funcs, suffix_start))

    minify_globals = 'minifyNames' in passes
    if minify_globals:
      passes = [p if p != 'minifyNames' else 'minifyLocals' for p in passes]
      start_asm = js.find(start_asm_marker)
      end_asm = js.rfind(end_asm_marker)
      assert (start_asm >= 0) == (end_asm >= 0)

    closure = 'closure' in passes
    if closure:
      passes = [p for p in passes if p != 'closure'] # we will do it manually

    cleanup = 'cleanup' in passes
    if cleanup:
      passes = [p for p in passes if p != 'cleanup'] # we will do it manually

  if not minify_globals:
    with ToolchainProfiler.profile_block('js_optimizer.no_minify_globals'):
      pre = js[:start_funcs + len(start_funcs_marker)]
      post = js[end_funcs + len(end_funcs_marker):]
      js = js[start_funcs + len(start_funcs_marker):end_funcs]
      if 'asm' not in passes:
        # can have Module[..] and inlining prevention code, push those to post
        class Finals:
          buf = []

        def process(line):
          if len(line) and (line.startswith(('Module[', 'if (globalScope)')) or line.endswith('["X"]=1;')):
            Finals.buf.append(line)
            return False
          return True

        js = '\n'.join(filter(process, js.split('\n')))
        post = '\n'.join(Finals.buf) + '\n' + post
      post = end_funcs_marker + post
  else:
    with ToolchainProfiler.profile_block('js_optimizer.minify_globals'):
      # We need to split out the asm shell as well, for minification
      pre = js[:start_asm + len(start_asm_marker)]
      post = js[end_asm:]
      asm_shell = js[start_asm + len(start_asm_marker):start_funcs + len(start_funcs_marker)] + '''
EMSCRIPTEN_FUNCS();
''' + js[end_funcs + len(end_funcs_marker):end_asm + len(end_asm_marker)]
      js = js[start_funcs + len(start_funcs_marker):end_funcs]

      # we assume there is a maximum of one new name per line
      minifier = Minifier(js)

      def check_symbol_mapping(p):
        if p.startswith('symbolMap='):
          minifier.symbols_file = p.split('=', 1)[1]
          return False
        if p == 'profilingFuncs':
          minifier.profiling_funcs = True
          return False
        return True

      passes = list(filter(check_symbol_mapping, passes))
      asm_shell_pre, asm_shell_post = minifier.minify_shell(asm_shell, 'minifyWhitespace' in passes).split('EMSCRIPTEN_FUNCS();')
      asm_shell_post = asm_shell_post.replace('});', '})')
      pre += asm_shell_pre + '\n' + start_funcs_marker
      post = end_funcs_marker + asm_shell_post + post

      minify_info = minifier.serialize()

      if extra_info:
        for key, value in extra_info.items():
          assert key not in minify_info or value == minify_info[key], [key, value, minify_info[key]]
          minify_info[key] = value

      # if DEBUG:
      #   print >> sys.stderr, 'minify info:', minify_info

  with ToolchainProfiler.profile_block('js_optimizer.remove_suffix_and_split'):
    # remove suffix if no longer needed
    if suffix and 'last' in passes:
      suffix_start = post.find(suffix_marker)
      suffix_end = post.find('\n', suffix_start)
      post = post[:suffix_start] + post[suffix_end:]

    total_size = len(js)
    funcs = split_funcs(js, just_split)
    js = None

  with ToolchainProfiler.profile_block('js_optimizer.split_to_chunks'):
    # if we are making source maps, we want our debug numbering to start from the
    # top of the file, so avoid breaking the JS into chunks
    cores = shared.get_num_cores()

    if not just_split:
      intended_num_chunks = int(round(cores * NUM_CHUNKS_PER_CORE))
      chunk_size = min(MAX_CHUNK_SIZE, max(MIN_CHUNK_SIZE, total_size / intended_num_chunks))
      chunks = chunkify(funcs, chunk_size)
    else:
      # keep same chunks as before
      chunks = [f[1] for f in funcs]

    chunks = [chunk for chunk in chunks if len(chunk)]
    if DEBUG and len(chunks):
      print('chunkification: num funcs:', len(funcs), 'actual num chunks:', len(chunks), 'chunk size range:', max(map(len, chunks)), '-', min(map(len, chunks)), file=sys.stderr)
    funcs = None

    if len(chunks):
      serialized_extra_info = suffix_marker + '\n'
      if minify_globals:
        serialized_extra_info += '// EXTRA_INFO:' + json.dumps(minify_info)
      elif extra_info:
        serialized_extra_info += '// EXTRA_INFO:' + json.dumps(extra_info)
      with ToolchainProfiler.profile_block('js_optimizer.write_chunks'):
        def write_chunk(chunk, i):
          temp_file = temp_files.get('.jsfunc_%d.js' % i).name
          with open(temp_file, 'w') as f:
            f.write(chunk)
            f.write(serialized_extra_info)
          return temp_file
        filenames = [write_chunk(chunks[i], i) for i in range(len(chunks))]
    else:
      filenames = []

  with ToolchainProfiler.profile_block('run_optimizer'):
    if len(filenames):
      commands = [config.NODE_JS + [ACORN_OPTIMIZER, f] + passes for f in filenames]

      if os.environ.get('EMCC_SAVE_OPT_TEMP') and os.environ.get('EMCC_SAVE_OPT_TEMP') != '0':
        for filename in filenames:
          saved = 'save_' + os.path.basename(filename)
          while os.path.exists(saved):
            saved = 'input' + str(int(saved.replace('input', '').replace('.txt', '')) + 1) + '.txt'
          shutil.copyfile(filename, os.path.join(shared.get_emscripten_temp_dir(), saved))

      filenames = shared.run_multiple_processes(commands, route_stdout_to_temp_files_suffix='js_opt.jo.js')

    for filename in filenames:
      temp_files.note(filename)

  with ToolchainProfiler.profile_block('split_closure_cleanup'):
    if closure or cleanup:
      # run on the shell code, everything but what we acorn-optimize
      start_asm = '// EMSCRIPTEN_START_ASM\n'
      end_asm = '// EMSCRIPTEN_END_ASM\n'
      cl_sep = 'wakaUnknownBefore(); var asm=wakaUnknownAfter(wakaGlobal,wakaEnv,wakaBuffer)\n'

      with temp_files.get_file('.cl.js') as cle:
        pre_1, pre_2 = pre.split(start_asm)
        post_1, post_2 = post.split(end_asm)
        with open(cle, 'w') as f:
          f.write(pre_1)
          f.write(cl_sep)
          f.write(post_2)
        cld = cle
        if closure:
          if DEBUG:
            print('running closure on shell code', file=sys.stderr)
          cld = building.closure_compiler(cld, pretty='minifyWhitespace' not in passes)
          temp_files.note(cld)
        elif cleanup:
          if DEBUG:
            print('running cleanup on shell code', file=sys.stderr)
          acorn_passes = ['JSDCE']
          if 'minifyWhitespace' in passes:
            acorn_passes.append('minifyWhitespace')
          cld = building.acorn_optimizer(cld, acorn_passes)
          temp_files.note(cld)
        coutput = utils.read_file(cld)

      coutput = coutput.replace('wakaUnknownBefore();', start_asm)
      after = 'wakaUnknownAfter'
      start = coutput.find(after)
      end = coutput.find(')', start)
      # If the closure comment to suppress useless code is present, we need to look one
      # brace past it, as the first is in there. Otherwise, the first brace is the
      # start of the function body (what we want).
      USELESS_CODE_COMMENT = '/** @suppress {uselessCode} */ '
      USELESS_CODE_COMMENT_BODY = 'uselessCode'
      brace = pre_2.find('{') + 1
      has_useless_code_comment = False
      if pre_2[brace:brace + len(USELESS_CODE_COMMENT_BODY)] == USELESS_CODE_COMMENT_BODY:
        brace = pre_2.find('{', brace) + 1
        has_useless_code_comment = True
      pre = coutput[:start] + '(' + (USELESS_CODE_COMMENT if has_useless_code_comment else '') + 'function(global,env,buffer) {\n' + pre_2[brace:]
      post = post_1 + end_asm + coutput[end + 1:]

  with ToolchainProfiler.profile_block('write_pre'):
    filename += '.jo.js'
    temp_files.note(filename)
    f = open(filename, 'w')
    f.write(pre)
    pre = None

  with ToolchainProfiler.profile_block('sort_or_concat'):
    if not just_concat:
      # sort functions by size, to make diffing easier and to improve aot times
      funcses = []
      for out_file in filenames:
        funcses.append(split_funcs(utils.read_file(out_file), False))
      funcs = [item for sublist in funcses for item in sublist]
      funcses = None
      if not os.environ.get('EMCC_NO_OPT_SORT'):
        funcs.sort(key=lambda x: (len(x[1]), x[0]), reverse=True)

      if 'last' in passes and len(funcs):
        count = funcs[0][1].count('\n')
        if count > 3000:
          print('warning: Output contains some very large functions (%s lines in %s), consider building source files with -Os or -Oz)' % (count, funcs[0][0]), file=sys.stderr)

      for func in funcs:
        f.write(func[1])
      funcs = None
    else:
      # just concat the outputs
      for out_file in filenames:
        f.write(utils.read_file(out_file))

  with ToolchainProfiler.profile_block('write_post'):
    f.write('\n')
    f.write(post)
    # No need to write suffix: if there was one, it is inside post which exists when suffix is there
    f.write('\n')
    f.close()

  return filename


@ToolchainProfiler.profile_block('js_optimizer.run_on_js')
def run(filename, passes, extra_info=None):
  just_split = 'receiveJSON' in passes
  just_concat = 'emitJSON' in passes
  return run_on_js(filename, passes, extra_info=extra_info, just_split=just_split, just_concat=just_concat)


def main():
  last = sys.argv[-1]
  if '{' in last:
    extra_info = json.loads(last)
    sys.argv = sys.argv[:-1]
  else:
    extra_info = None
  out = run(sys.argv[1], sys.argv[2:], extra_info=extra_info)
  shutil.copyfile(out, sys.argv[1] + '.jsopt.js')
  return 0


if __name__ == '__main__':
  sys.exit(main())
