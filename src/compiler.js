#!/usr/bin/env node
/**
 * @license
 * Copyright 2010 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

// LLVM => JavaScript compiler, main entry point

const fs = require('fs');
global.assert = require('assert');
global.nodePath = require('path');

global.print = (x) => {
  process['stdout'].write(x + '\n');
};

global.printErr = (x) => {
  process['stderr'].write(x + '\n');
};

function find(filename) {
  const prefixes = [__dirname, process.cwd()];
  for (let i = 0; i < prefixes.length; ++i) {
    const combined = nodePath.join(prefixes[i], filename);
    if (fs.existsSync(combined)) {
      return combined;
    }
  }
  return filename;
}

global.read = (filename) => {
  const absolute = find(filename);
  return fs.readFileSync(absolute).toString();
};

function load(f) {
  eval.call(null, read(f));
};

// Basic utilities
load('utility.js');

// Load settings from JSON passed on the command line
const settingsFile = process['argv'][2];
assert(settingsFile);

const settings = JSON.parse(read(settingsFile));
Object.assign(global, settings);

EXPORTED_FUNCTIONS = new Set(EXPORTED_FUNCTIONS);
WASM_EXPORTS = new Set(WASM_EXPORTS);
SIDE_MODULE_EXPORTS = new Set(SIDE_MODULE_EXPORTS);
INCOMING_MODULE_JS_API = new Set(INCOMING_MODULE_JS_API);
WEAK_IMPORTS = new Set(WEAK_IMPORTS);

// Side modules are pure wasm and have no JS
assert(!SIDE_MODULE, 'JS compiler should not run on side modules');

// Output some info and warnings based on settings

if (VERBOSE) {
  printErr('VERBOSE is on, this generates a lot of output and can slow down compilation');
}

// Load compiler code

load('modules.js');
load('parseTools.js');
if (!STRICT) {
  load('parseTools_legacy.js');
}
load('jsifier.js');
load('runtime.js');

// ===============================
// Main
// ===============================

B = new Benchmarker();

try {
  runJSify();

  B.print('glue');
} catch (err) {
  if (err.toString().includes('Aborting compilation due to previous errors')) {
    // Compiler failed on user error, don't print the stacktrace in this case.
    printErr(err);
  } else {
    // Compiler failed on internal compiler error!
    printErr('Internal compiler error in src/compiler.js!');
    printErr('Please create a bug report at https://github.com/emscripten-core/emscripten/issues/');
    printErr('with a log of the build and the input files used to run. Exception message: "' + err + '" | ' + err.stack);
  }

  // Work around a node.js bug where stdout buffer is not flushed at process exit:
  // Instead of process.exit() directly, wait for stdout flush event.
  // See https://github.com/joyent/node/issues/1669 and https://github.com/emscripten-core/emscripten/issues/2582
  // Workaround is based on https://github.com/RReverser/acorn/commit/50ab143cecc9ed71a2d66f78b4aec3bb2e9844f6
  process['stdout']['once']('drain', () => process['exit'](1));
  // Make sure to print something to force the drain event to occur, in case the
  // stdout buffer was empty.
  console.log(' ');
  // Work around another node bug where sometimes 'drain' is never fired - make
  // another effort to emit the exit status, after a significant delay (if node
  // hasn't fired drain by then, give up)
  setTimeout(() => process['exit'](1), 500);
}
