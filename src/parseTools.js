/**
 * @license
 * Copyright 2010 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 *
 * Helpers and tools for use at compile time by JavaScript library files.
 *
 * Tests live in test/other/test_parseTools.js.
 */

const FOUR_GB = 4 * 1024 * 1024 * 1024;
const FLOAT_TYPES = new Set(['float', 'double']);

let currentlyParsedFilename = '';

// Does simple 'macro' substitution, using Django-like syntax,
// {{{ code }}} will be replaced with |eval(code)|.
// NOTE: Be careful with that ret check. If ret is |0|, |ret ? ret.toString() : ''| would result in ''!
function processMacros(text) {
  return text.replace(/{{{([^}]|}(?!}))+}}}/g, (str) => {
    str = str.substr(3, str.length - 6);
    try {
      const ret = eval(str);
      return ret !== null ? ret.toString() : '';
    } catch (ex) {
      ex.stack = 'In the following macro:\n\n' + str + '\n\n' + ex.stack;
      throw ex;
    }
  });
}

// Simple #if/else/endif preprocessing for a file. Checks if the
// ident checked is true in our global.
// Also handles #include x.js (similar to C #include <file>)
// Param filenameHint can be passed as a description to identify the file that is being processed, used
// to locate errors for reporting and for html files to stop expansion between <style> and </style>.
function preprocess(text, filenameHint) {
  if (EXPORT_ES6 && USE_ES6_IMPORT_META) {
    // `eval`, Terser and Closure don't support module syntax; to allow it,
    // we need to temporarily replace `import.meta` usages with placeholders
    // during preprocess phase, and back after all the other ops.
    // See also: `phase_final_emitting` in emcc.py.
    text = text.replace(/\bimport\.meta\b/g, 'EMSCRIPTEN$IMPORT$META');
  }

  const IGNORE = 0;
  const SHOW = 1;
  // This state is entered after we have shown one of the block of an if/elif/else sequence.
  // Once we enter this state we dont show any blocks or evaluate any
  // conditions until the sequence ends.
  const IGNORE_ALL = 2;
  const showStack = [];
  const showCurrentLine = () => showStack.every((x) => x == SHOW);

  currentlyParsedFilename = filenameHint;
  const fileExt = (filenameHint) ? filenameHint.split('.').pop().toLowerCase() : '';
  const isHtml = (fileExt === 'html' || fileExt === 'htm') ? true : false;
  let inStyle = false;
  const lines = text.split('\n');
  let ret = '';
  let emptyLine = false;

  try {
    for (let i = 0; i < lines.length; i++) {
      let line = lines[i];
      try {
        if (line[line.length - 1] === '\r') {
          line = line.substr(0, line.length - 1); // Windows will have '\r' left over from splitting over '\r\n'
        }
        if (isHtml && line.includes('<style') && !inStyle) {
          inStyle = true;
        }
        if (isHtml && line.includes('</style') && inStyle) {
          inStyle = false;
        }

        if (!inStyle) {
          const trimmed = line.trim();
          if (trimmed.startsWith('#')) {
            const first = trimmed.split(' ', 1)[0];
            if (first == '#if' || first == '#ifdef' || first == '#elif') {
              if (first == '#ifdef') {
                warn('use of #ifdef in js library.  Use #if instead.');
              }
              if (first == '#elif') {
                const curr = showStack.pop();
                if (curr == SHOW || curr == IGNORE_ALL) {
                  // If we showed to previous block we enter the IGNORE_ALL state
                  // and stay there until endif is seen
                  showStack.push(IGNORE_ALL);
                  continue;
                }
              }
              const after = trimmed.substring(trimmed.indexOf(' '));
              const truthy = !!eval(after);
              showStack.push(truthy ? SHOW : IGNORE);
            } else if (first === '#include') {
              if (showCurrentLine()) {
                let filename = line.substr(line.indexOf(' ') + 1);
                if (filename.startsWith('"')) {
                  filename = filename.substr(1, filename.length - 2);
                }
                const included = read(filename);
                const result = preprocess(included, filename);
                if (result) {
                  ret += `// include: ${filename}\n`;
                  ret += result;
                  ret += `// end include: ${filename}\n`;
                }
              }
            } else if (first === '#else') {
              assert(showStack.length > 0);
              const curr = showStack.pop();
              if (curr == IGNORE) {
                showStack.push(SHOW);
              } else {
                showStack.push(IGNORE);
              }
            } else if (first === '#endif') {
              assert(showStack.length > 0);
              showStack.pop();
            } else if (first === '#warning') {
              if (showCurrentLine()) {
                printErr(`${filenameHint}:${i + 1}: #warning ${trimmed.substring(trimmed.indexOf(' ')).trim()}`);
              }
            } else if (first === '#error') {
              if (showCurrentLine()) {
                error(`${filenameHint}:${i + 1}: #error ${trimmed.substring(trimmed.indexOf(' ')).trim()}`);
              }
            } else {
              throw new Error(`Unknown preprocessor directive on line ${i}: ``${line}```);
            }
          } else {
            if (showCurrentLine()) {
              // Never emit more than one empty line at a time.
              if (emptyLine && !line) {
                continue;
              }
              ret += line + '\n';
              if (!line) {
                emptyLine = true;
              } else {
                emptyLine = false;
              }
            }
          }
        } else { // !inStyle
          if (showCurrentLine()) {
            ret += line + '\n';
          }
        }
      } catch (e) {
        printErr('parseTools.js preprocessor error in ' + filenameHint + ':' + (i + 1) + ': \"' + line + '\"!');
        throw e;
      }
    }
    assert(showStack.length == 0, `preprocessing error in file ${filenameHint}, \
no matching #endif found (${showStack.length$}' unmatched preprocessing directives on stack)`);
    return ret;
  } finally {
    currentlyParsedFilename = null;
  }
}

// Returns true if ident is a niceIdent (see toNiceIdent). Also allow () and spaces.
function isNiceIdent(ident, loose) {
  return /^\(?[$_]+[\w$_\d ]*\)?$/.test(ident);
}

// Simple variables or numbers, or things already quoted, do not need to be quoted
function needsQuoting(ident) {
  if (/^[-+]?[$_]?[\w$_\d]*$/.test(ident)) return false; // number or variable
  if (ident[0] === '(' && ident[ident.length - 1] === ')' && ident.indexOf('(', 1) < 0) return false; // already fully quoted
  return true;
}

const POINTER_SIZE = MEMORY64 ? 8 : 4;
const POINTER_BITS = POINTER_SIZE * 8;
const POINTER_TYPE = 'u' + POINTER_BITS;
const POINTER_SHIFT = MEMORY64 ? '3' : '2';
const POINTER_HEAP = MEMORY64 ? 'HEAP64' : 'HEAP32';

const SIZE_TYPE = POINTER_TYPE;


// Similar to POINTER_TYPE, but this is the actual wasm type that is
// used in practice, while POINTER_TYPE is the more refined internal
// type (that is unsigned, where as core wasm does not have unsigned
// types).
const POINTER_WASM_TYPE = 'i' + POINTER_BITS;

function isPointerType(type) {
  return type[type.length - 1] == '*';
}

function isIntImplemented(type) {
  return type[0] == 'i' || type[0] == 'u' || isPointerType(type);
}

// Note: works for iX types and structure types, not pointers (even though they are implemented as ints)
function getBits(type, allowPointers) {
  if (allowPointers && isPointerType(type)) return POINTER_SIZE;
  if (!type) return 0;
  if (type[0] == 'i' || type[0] == 'u') {
    const left = type.substr(1);
    if (!isNumber(left)) return 0;
    return parseInt(left);
  }
  return 0;
}

// Given an expression like (VALUE=VALUE*2,VALUE<10?VALUE:t+1) , this will
// replace VALUE with value. If value is not a simple identifier of a variable,
// value will be replaced with tempVar.
function makeInlineCalculation(expression, value, tempVar) {
  if (!isNiceIdent(value)) {
    expression = tempVar + '=' + value + ',' + expression;
    value = tempVar;
  }
  return '(' + expression.replace(/VALUE/g, value) + ')';
}

// XXX Make all i64 parts signed

// Splits a number (an integer in a double, possibly > 32 bits) into an i64
// value, represented by a low and high i32 pair.
// Will suffer from rounding.
function splitI64(value) {
  // general idea:
  //
  //  $1$0 = ~~$d >>> 0;
  //  $1$1 = Math.abs($d) >= 1 ? (
  //     $d > 0 ? Math.min(Math.floor(($d)/ 4294967296.0), 4294967295.0)
  //            : Math.ceil(Math.min(-4294967296.0, $d - $1$0)/ 4294967296.0)
  //  ) : 0;
  //
  // We need to min on positive values here, since our input might be a double,
  // and large values are rounded, so they can be slightly higher than expected.
  // And if we get 4294967296, that will turn into a 0 if put into a HEAP32 or
  // |0'd, etc.
  //
  // For negatives, we need to ensure a -1 if the value is overall negative,
  // even if not significant negative component

  const low = value + '>>>0';
  const high = makeInlineCalculation(
      asmCoercion('Math.abs(VALUE)', 'double') + ' >= ' + asmEnsureFloat('1', 'double') + ' ? ' +
        '(VALUE > ' + asmEnsureFloat('0', 'double') + ' ? ' +
        asmCoercion('Math.min(' + asmCoercion('Math.floor((VALUE)/' +
        asmEnsureFloat(4294967296, 'double') + ')', 'double') + ', ' +
        asmEnsureFloat(4294967295, 'double') + ')', 'i32') + '>>>0' +
        ' : ' +
        asmFloatToInt(asmCoercion('Math.ceil((VALUE - +((' + asmFloatToInt('VALUE') + ')>>>0))/' +
        asmEnsureFloat(4294967296, 'double') + ')', 'double')) + '>>>0' +
        ')' +
      ' : 0',
      value,
      'tempDouble',
  );
  return [low, high];
}

// Misc

function indentify(text, indent) {
  // Don't try to indentify huge strings - we may run out of memory
  if (text.length > 1024 * 1024) return text;
  if (typeof indent == 'number') {
    const len = indent;
    indent = '';
    for (let i = 0; i < len; i++) {
      indent += ' ';
    }
  }
  return text.replace(/\n/g, '\n' + indent);
}

// Correction tools

function getHeapOffset(offset, type) {
  if (!WASM_BIGINT && Runtime.getNativeFieldSize(type) > 4 && type == 'i64') {
    // we emulate 64-bit integer values as 32 in asmjs-unknown-emscripten, but not double
    type = 'i32';
  }

  const sz = Runtime.getNativeTypeSize(type);
  const shifts = Math.log(sz) / Math.LN2;
  return `((${offset})>>${shifts})`;
}

function ensureDot(value) {
  value = value.toString();
  // if already dotted, or Infinity or NaN, nothing to do here
  // if smaller than 1 and running js opts, we always need to force a coercion
  // (0.001 will turn into 1e-3, which has no .)
  if ((value.includes('.') || /[IN]/.test(value))) return value;
  const e = value.indexOf('e');
  if (e < 0) return value + '.0';
  return value.substr(0, e) + '.0' + value.substr(e);
}

// ensures that a float type has either 5.5 (clearly a float) or +5 (float due to asm coercion)
function asmEnsureFloat(value, type) {
  if (!isNumber(value)) return value;
  if (type === 'float') {
    // normally ok to just emit Math.fround(0), but if the constant is large we
    // may need a .0 (if it can't fit in an int)
    if (value == 0) return 'Math.fround(0)';
    value = ensureDot(value);
    return 'Math.fround(' + value + ')';
  }
  if (FLOAT_TYPES.has(type)) {
    return ensureDot(value);
  }
  return value;
}

function asmCoercion(value, type) {
  assert(arguments.length == 2, 'asmCoercion takes exactly two arguments');
  if (type == 'void') {
    return value;
  }
  if (FLOAT_TYPES.has(type)) {
    if (isNumber(value)) {
      return asmEnsureFloat(value, type);
    }
    if (type === 'float') {
      return 'Math.fround(' + value + ')';
    }
    return '(+(' + value + '))';
  }
  return '((' + value + ')|0)';
}

function asmFloatToInt(x) {
  return '(~~(' + x + '))';
}

function makeGetTempDouble(i, type, forSet) { // get an aliased part of the tempDouble temporary storage
  // Cannot use makeGetValue because it uses us
  // this is a unique case where we *can* use HEAPF64
  const heap = getHeapForType(type);
  const ptr = getFastValue('tempDoublePtr', '+', Runtime.getNativeTypeSize(type) * i);
  let offset;
  if (type == 'double') {
    offset = '(' + ptr + ')>>3';
  } else {
    offset = getHeapOffset(ptr, type);
  }
  let ret = heap + '[' + offset + ']';
  if (!forSet) ret = asmCoercion(ret, type);
  return ret;
}

function makeSetTempDouble(i, type, value) {
  return makeGetTempDouble(i, type, true) + '=' + asmEnsureFloat(value, type);
}

// See makeSetValue
function makeGetValue(ptr, pos, type, noNeedFirst, unsigned, ignore, align) {
  assert(typeof align === 'undefined', 'makeGetValue no longer supports align parameter');
  if (typeof unsigned !== 'undefined') {
    // TODO(sbc): make this into an error at some point.
    printErr('makeGetValue: Please use u8/u16/u32/u64 unsigned types in favor of additional argument');
    if (unsigned && type.startsWith('i')) {
      type = 'u' + type.slice(1);
    }
  } else if (type.startsWith('u')) {
    // Set `unsigned` based on the type name.
    unsigned = true;
  }

  const offset = calcFastOffset(ptr, pos, noNeedFirst);
  if (type === 'i53' || type === 'u53') {
    return 'readI53From' + (unsigned ? 'U' : 'I') + '64(' + offset + ')';
  }

  const slab = getHeapForType(type);
  let ret = slab + '[' + getHeapOffset(offset, type) + ']';
  if (MEMORY64 && isPointerType(type)) {
    ret = `Number(${ret})`;
  }
  return ret;
}

/**
 * @param {nunber} ptr The pointer. Used to find both the slab and the offset in that slab. If the pointer
 *            is just an integer, then this is almost redundant, but in general the pointer type
 *            may in the future include information about which slab as well. So, for now it is
 *            possible to put |0| here, but if a pointer is available, that is more future-proof.
 * @param {nunber} pos The position in that slab - the offset. Added to any offset in the pointer itself.
 * @param {number} value The value to set.
 * @param {string} type A string defining the type. Used to find the slab (HEAPU8, HEAP16, HEAPU32, etc.).
 *             which means we should write to all slabs, ignore type differences if any on reads, etc.
 * @param {bool} noNeedFirst Whether to ignore the offset in the pointer itself.
 * @param {bool} ignore: legacy, ignored.
 * @param {number} align: legacy, ignored.
 * @param {string} sep: TODO
 * @return {TODO}
 */
function makeSetValue(ptr, pos, value, type, noNeedFirst, ignore, align, sep = ';') {
  assert(typeof align === 'undefined', 'makeSetValue no longer supports align parameter');
  if (type == 'double' && (align < 8)) {
    return '(' + makeSetTempDouble(0, 'double', value) + ',' +
            makeSetValue(ptr, pos, makeGetTempDouble(0, 'i32'), 'i32', noNeedFirst, ignore, align, ',') + ',' +
            makeSetValue(ptr, getFastValue(pos, '+', Runtime.getNativeTypeSize('i32')), makeGetTempDouble(1, 'i32'), 'i32', noNeedFirst, ignore, align, ',') + ')';
  } else if (!WASM_BIGINT && type == 'i64') {
    return '(tempI64 = [' + splitI64(value) + '],' +
            makeSetValue(ptr, pos, 'tempI64[0]', 'i32', noNeedFirst, ignore, align, ',') + ',' +
            makeSetValue(ptr, getFastValue(pos, '+', Runtime.getNativeTypeSize('i32')), 'tempI64[1]', 'i32', noNeedFirst, ignore, align, ',') + ')';
  }

  const bits = getBits(type);
  const needSplitting = bits > 0 && !isPowerOfTwo(bits); // an unnatural type like i24
  if (needSplitting) {
    // Alignment is important here, or we need to split this up for other reasons.
    const bytes = Runtime.getNativeTypeSize(type);
    if (needSplitting) {
      let ret = '';
      if (isIntImplemented(type)) {
        ret += 'tempBigInt=' + value + sep;
        for (let i = 0; i < bytes; i++) {
          ret += makeSetValue(ptr, getFastValue(pos, '+', i), 'tempBigInt&0xff', 'i8', noNeedFirst, ignore, 1);
          if (i < bytes - 1) ret += sep + 'tempBigInt = tempBigInt>>8' + sep;
        }
      } else {
        ret += makeSetValue('tempDoublePtr', 0, value, type, noNeedFirst, ignore, 8) + sep;
        ret += makeCopyValues(getFastValue(ptr, '+', pos), 'tempDoublePtr', Runtime.getNativeTypeSize(type), type, null, align, sep);
      }
      return ret;
    }
  }

  const offset = calcFastOffset(ptr, pos, noNeedFirst);

  const slab = getHeapForType(type);
  if (slab == 'HEAPU64' || slab == 'HEAP64') {
    value = `BigInt(${value})`;
  }
  return slab + '[' + getHeapOffset(offset, type) + '] = ' + value;
}

const UNROLL_LOOP_MAX = 8;

function makeCopyValues(dest, src, num, type, modifier, align, sep = ';') {
  assert(typeof align === 'undefined');
  function unroll(type, num, jump) {
    jump = jump || 1;
    const setValues = range(num).map((i) => makeSetValue(dest, i * jump, makeGetValue(src, i * jump, type), type));
    return setValues.join(sep);
  }
  // If we don't know how to handle this at compile-time, or handling it is best
  // done in a large amount of code, call memcpy
  if (!isNumber(num)) num = stripCorrections(num);
  if (!isNumber(align)) align = stripCorrections(align);
  if (!isNumber(num) || (parseInt(num) / align >= UNROLL_LOOP_MAX)) {
    return '(_memcpy(' + dest + ', ' + src + ', ' + num + ')|0)';
  }
  num = parseInt(num);
  // remove corrections, since we will be correcting after we add anyhow,
  dest = stripCorrections(dest);
  src = stripCorrections(src);
  // and in the heap assignment expression
  const ret = [];
  [4, 2, 1].forEach((possibleAlign) => {
    if (num == 0) return;
    if (align >= possibleAlign) {
      ret.push(unroll('i' + (possibleAlign * 8), Math.floor(num / possibleAlign), possibleAlign));
      src = getFastValue(src, '+', Math.floor(num / possibleAlign) * possibleAlign);
      dest = getFastValue(dest, '+', Math.floor(num / possibleAlign) * possibleAlign);
      num %= possibleAlign;
    }
  });
  return ret.join(sep);
}

function makeHEAPView(which, start, end) {
  const size = parseInt(which.replace('U', '').replace('F', '')) / 8;
  const mod = size == 1 ? '' : ('>>' + Math.log2(size));
  return `HEAP${which}.subarray((${start})${mod}, (${end})${mod})`;
}

// When dynamically linking, some things like dynCalls may not exist in one module and
// be provided by a linked module, so they must be accessed indirectly using Module
function exportedAsmFunc(func) {
  if (!MAIN_MODULE) {
    return func;
  }
  return `Module['${func}']`;
}

const TWO_TWENTY = Math.pow(2, 20);

// Given two values and an operation, returns the result of that operation.
// Tries to do as much as possible at compile time.
// Leaves overflows etc. unhandled, *except* for integer multiply, in order to be efficient with Math.imul
function getFastValue(a, op, b, type) {
  a = a === 'true' ? '1' : (a === 'false' ? '0' : a);
  b = b === 'true' ? '1' : (b === 'false' ? '0' : b);

  let aNumber = null;
  let bNumber = null;
  if (typeof a == 'number') {
    aNumber = a;
    a = a.toString();
  } else if (isNumber(a)) aNumber = parseFloat(a);
  if (typeof b == 'number') {
    bNumber = b;
    b = b.toString();
  } else if (isNumber(b)) bNumber = parseFloat(b);

  if (aNumber !== null && bNumber !== null) {
    switch (op) {
      case '+': return (aNumber + bNumber).toString();
      case '-': return (aNumber - bNumber).toString();
      case '*': return (aNumber * bNumber).toString();
      case '/': {
        if (type[0] === 'i') {
          return ((aNumber / bNumber) | 0).toString();
        }
        return (aNumber / bNumber).toString();
      }
      case '%': return (aNumber % bNumber).toString();
      case '|': return (aNumber | bNumber).toString();
      case '>>>': return (aNumber >>> bNumber).toString();
      case '&': return (aNumber & bNumber).toString();
      case 'pow': return Math.pow(aNumber, bNumber).toString();
      default: assert(false, 'need to implement getFastValue pn ' + op);
    }
  }
  if (op === 'pow') {
    if (a === '2' && isIntImplemented(type)) {
      return `(1 << (${b}))`;
    }
    return `Math.pow(${a}, ${b})`;
  }
  if ((op === '+' || op === '*') && aNumber !== null) { // if one of them is a number, keep it last
    const c = b;
    b = a;
    a = c;
    const cNumber = bNumber;
    bNumber = aNumber;
    aNumber = cNumber;
  }
  if (op === '*') {
    // We can't eliminate where a or b are 0 as that would break things for creating
    // a negative 0.
    if ((aNumber === 0 || bNumber === 0) && !FLOAT_TYPES.has(type)) {
      return '0';
    } else if (aNumber === 1) {
      return b;
    } else if (bNumber === 1) {
      return a;
    } else if (bNumber !== null && type && isIntImplemented(type) && Runtime.getNativeTypeSize(type) <= 32) {
      const shifts = Math.log(bNumber) / Math.LN2;
      if (shifts % 1 === 0) {
        return `(${a}<<${shifts})`;
      }
    }
    if (!FLOAT_TYPES.has(type)) {
      // if guaranteed small enough to not overflow into a double, do a normal multiply
      // default is 32-bit multiply for things like getelementptr indexes
      const bits = getBits(type) || 32;
      // Note that we can emit simple multiple in non-asm.js mode, but asm.js
      // will not parse "16-bit" multiple, so must do imul there
      if ((aNumber !== null && Math.abs(a) < TWO_TWENTY) || (bNumber !== null && Math.abs(b) < TWO_TWENTY)) {
        // keep a non-eliminatable coercion directly on this
        return `(((${a})*(${b}))&${(Math.pow(2, bits) - 1) | 0})`;
      }
      return `(Math.imul(${a}, ${b})|0)`;
    }
  } else if (op === '/') {
    // careful on floats, since 0*NaN is not 0
    if (a === '0' && !FLOAT_TYPES.has(type)) {
      return '0';
    } else if (b === 1) {
      return a;
    } // Doing shifts for division is problematic, as getting the rounding right on negatives is tricky
  } else if (op === '+' || op === '-') {
    if (b[0] === '-') {
      op = op === '+' ? '-' : '+';
      b = b.substr(1);
    }
    if (aNumber === 0) {
      return op === '+' ? b : `(-${b})`;
    } else if (bNumber === 0) {
      return a;
    }
  }
  return `(${a})${op}(${b})`;
}

function calcFastOffset(ptr, pos, noNeedFirst) {
  assert(!noNeedFirst);
  if (typeof ptr == 'bigint') ptr = Number(ptr);
  if (typeof pos == 'bigint') pos = Number(pos);
  return getFastValue(ptr, '+', pos, 'i32');
}

function getHeapForType(type) {
  assert(type);
  if (isPointerType(type)) {
    type = POINTER_TYPE;
  }
  if (WASM_BIGINT) {
    switch (type) {
      case 'i64': return 'HEAP64';
      case 'u64': return 'HEAPU64';
    }
  }
  switch (type) {
    case 'i1':     // fallthrough
    case 'i8':     return 'HEAP8';
    case 'u8':     return 'HEAPU8';
    case 'i16':    return 'HEAP16';
    case 'u16':    return 'HEAPU16';
    case 'i64':    // fallthrough
    case 'i32':    return 'HEAP32';
    case 'u64':    // fallthrough
    case 'u32':    return 'HEAPU32';
    case 'double': return 'HEAPF64';
    case 'float':  return 'HEAPF32';
  }
  assert(false, 'bad heap type: ' + type);
}

function makeReturn64(value) {
  if (WASM_BIGINT) {
    return `BigInt(${value})`;
  }
  const pair = splitI64(value);
  // `return (a, b, c)` in JavaScript will execute `a`, and `b` and return the final
  // element `c`
  return `(setTempRet0(${pair[1]}), ${pair[0]})`;
}

function makeThrow(what) {
  if (ASSERTIONS && DISABLE_EXCEPTION_CATCHING) {
    what += ' + " - Exception catching is disabled, this exception cannot be caught. Compile with -sNO_DISABLE_EXCEPTION_CATCHING or -sEXCEPTION_CATCHING_ALLOWED=[..] to catch."';
    if (MAIN_MODULE) {
      what += ' + " (note: in dynamic linking, if a side module wants exceptions, the main module must be built with that support)"';
    }
  }
  return `throw ${what};`;
}

function stripCorrections(param) {
  let m;
  while (true) {
    if (m = /^\((.*)\)$/.exec(param)) {
      param = m[1];
      continue;
    }
    if (m = /^\(([$_\w]+)\)&\d+$/.exec(param)) {
      param = m[1];
      continue;
    }
    if (m = /^\(([$_\w()]+)\)\|0$/.exec(param)) {
      param = m[1];
      continue;
    }
    if (m = /^\(([$_\w()]+)\)\>>>0$/.exec(param)) {
      param = m[1];
      continue;
    }
    if (m = /CHECK_OVERFLOW\(([^,)]*),.*/.exec(param)) {
      param = m[1];
      continue;
    }
    break;
  }
  return param;
}

function charCode(char) {
  return char.charCodeAt(0);
}

function getTypeFromHeap(suffix) {
  switch (suffix) {
    case '8': return 'i8';
    case '16': return 'i16';
    case '32': return 'i32';
    case 'F32': return 'float';
    case 'F64': return 'double';
  }
  assert(false, 'bad type suffix: ' + suffix);
}

function ensureValidFFIType(type) {
  return type === 'float' ? 'double' : type; // ffi does not tolerate float XXX
}

// FFI return values must arrive as doubles, and we can force them to floats afterwards
function asmFFICoercion(value, type) {
  value = asmCoercion(value, ensureValidFFIType(type));
  if (type === 'float') value = asmCoercion(value, 'float');
  return value;
}

function makeDynCall(sig, funcPtr) {
  assert(!sig.includes('j'), 'Cannot specify 64-bit signatures ("j" in signature string) with makeDynCall!');

  const returnExpr = (sig[0] == 'v') ? '' : 'return';

  let args = [];
  for (let i = 1; i < sig.length; ++i) {
    args.push(`a${i}`);
  }
  args = args.join(', ');

  const needArgConversion = MEMORY64 && sig.includes('p');
  let callArgs = args;
  if (needArgConversion) {
    callArgs = [];
    for (let i = 1; i < sig.length; ++i) {
      if (sig[i] == 'p') {
        callArgs.push(`BigInt(a${i})`);
      } else {
        callArgs.push(`a${i}`);
      }
    }
    callArgs = callArgs.join(', ');
  }

  // Normalize any 'p' characters to either 'j' (wasm64) or 'i' (wasm32)
  if (sig.includes('p')) {
    let normalizedSig = '';
    for (let sigChr of sig) {
      if (sigChr == 'p') {
        sigChr = MEMORY64 ? 'j' : 'i';
      }
      normalizedSig += sigChr;
    }
    sig = normalizedSig;
  }


  if (funcPtr === undefined) {
    warn(`${currentlyParsedFilename}: \
Legacy use of {{{ makeDynCall("${sig}") }}}(funcPtr, arg1, arg2, ...). \
Starting from Emscripten 2.0.2 (Aug 31st 2020), syntax for makeDynCall has changed. \
New syntax is {{{ makeDynCall("${sig}", "funcPtr") }}}(arg1, arg2, ...). \
Please update to new syntax.`);

    if (DYNCALLS) {
      if (!hasExportedSymbol(`dynCall_${sig}`)) {
        if (ASSERTIONS) {
          return `(function(${args}) { throw 'Internal Error! Attempted to invoke wasm function pointer with signature "${sig}", but no such functions have gotten exported!'; })`;
        } else {
          return `(function(${args}) { /* a dynamic function call to signature ${sig}, but there are no exported function pointers with that signature, so this path should never be taken. Build with ASSERTIONS enabled to validate. */ })`;
        }
      }
      return `(function(cb, ${args}) { ${returnExpr} getDynCaller("${sig}", cb)(${callArgs}) })`;
    } else {
      return `(function(cb, ${args}) { ${returnExpr} getWasmTableEntry(cb)(${callArgs}) })`;
    }
  }

  if (DYNCALLS) {
    if (!hasExportedSymbol(`dynCall_${sig}`)) {
      if (ASSERTIONS) {
        return `(function(${args}) { throw 'Internal Error! Attempted to invoke wasm function pointer with signature "${sig}", but no such functions have gotten exported!'; })`;
      } else {
        return `(function(${args}) { /* a dynamic function call to signature ${sig}, but there are no exported function pointers with that signature, so this path should never be taken. Build with ASSERTIONS enabled to validate. */ })`;
      }
    }

    const dyncall = exportedAsmFunc(`dynCall_${sig}`);
    if (sig.length > 1) {
      return `(function(${args}) { ${returnExpr} ${dyncall}.apply(null, [${funcPtr}, ${callArgs}]); })`;
    }
    return `(function() { ${returnExpr} ${dyncall}.call(null, ${funcPtr}); })`;
  }

  if (needArgConversion) {
    return `(function(${args}) { ${returnExpr} getWasmTableEntry(${funcPtr}).call(null, ${callArgs}) })`;
  }
  return `getWasmTableEntry(${funcPtr})`;
}

function heapAndOffset(heap, ptr) { // given   HEAP8, ptr   , we return    splitChunk, relptr
  return heap + ',' + ptr;
}

function makeEval(code) {
  if (DYNAMIC_EXECUTION == 0) {
    // Treat eval as error.
    return "abort('DYNAMIC_EXECUTION=0 was set, cannot eval');";
  }
  let ret = '';
  if (DYNAMIC_EXECUTION == 2) {
    // Warn on evals, but proceed.
    ret += "err('Warning: DYNAMIC_EXECUTION=2 was set, but calling eval in the following location:');\n";
    ret += 'err(stackTrace());\n';
  }
  ret += code;
  return ret;
}

global.ATINITS = [];

function addAtInit(code) {
  ATINITS.push(code);
}

// TODO(sbc): There are no more uses to ATMAINS or addAtMain in emscripten.
// We should look into removing these.
global.ATMAINS = [];

function addAtMain(code) {
  assert(HAS_MAIN, 'addAtMain called but program has no main function');
  ATMAINS.push(code);
}

global.ATEXITS = [];

function addAtExit(code) {
  if (EXIT_RUNTIME) {
    ATEXITS.push(code);
  }
}

function makeRetainedCompilerSettings() {
  const ignore = new Set(['STRUCT_INFO']);
  if (STRICT) {
    for (const setting of LEGACY_SETTINGS) {
      ignore.add(setting);
    }
  }

  const ret = {};
  for (const x in global) {
    if (!ignore.has(x) && x[0] !== '_' && x == x.toUpperCase()) {
      const value = global[x];
      if (typeof value == 'number' || typeof value == 'boolean' || typeof value == 'string' || Array.isArray(x)) {
        ret[x] = value;
      }
    }
  }
  return ret;
}

// In wasm, the heap size must be a multiple of 64KiB.
const WASM_PAGE_SIZE = 65536;

// Receives a function as text, and a function that constructs a modified
// function, to which we pass the parsed-out name, arguments, body, and possible
// "async" prefix of the input function. Returns the output of that function.
function modifyFunction(text, func) {
  // Match a function with a name.
  let match = text.match(/^\s*(async\s+)?function\s+([^(]*)?\s*\(([^)]*)\)/);
  let async_;
  let names;
  let args;
  let rest;
  if (match) {
    async_ = match[1] || '';
    name = match[2];
    args = match[3];
    rest = text.substr(match[0].length);
  } else {
    // Match a function without a name (we could probably use a single regex
    // for both, but it would be more complex).
    match = text.match(/^\s*(async\s+)?function\(([^)]*)\)/);
    assert(match, 'could not match function ' + text + '.');
    name = '';
    async_ = match[1] || '';
    args = match[2];
    rest = text.substr(match[0].length);
  }
  const bodyStart = rest.indexOf('{');
  assert(bodyStart >= 0);
  const bodyEnd = rest.lastIndexOf('}');
  assert(bodyEnd > 0);
  return func(name, args, rest.substring(bodyStart + 1, bodyEnd), async_);
}

function runOnMainThread(text) {
  if (WASM_WORKERS && USE_PTHREADS) {
    return 'if (!ENVIRONMENT_IS_WASM_WORKER && !ENVIRONMENT_IS_PTHREAD) { ' + text + ' }';
  } else if (WASM_WORKERS) {
    return 'if (!ENVIRONMENT_IS_WASM_WORKER) { ' + text + ' }';
  } else if (USE_PTHREADS) {
    return 'if (!ENVIRONMENT_IS_PTHREAD) { ' + text + ' }';
  } else {
    return text;
  }
}

function expectToReceiveOnModule(name) {
  return INCOMING_MODULE_JS_API.has(name);
}

function makeRemovedModuleAPIAssert(moduleName, localName) {
  if (!ASSERTIONS) return '';
  if (!localName) localName = moduleName;
  return `legacyModuleProp('${moduleName}', '${localName}');`;
}

function checkReceiving(name) {
  // ALL_INCOMING_MODULE_JS_API contains all valid incoming module API symbols
  // so calling makeModuleReceive* with a symbol not in this list is an error
  assert(ALL_INCOMING_MODULE_JS_API.includes(name));
}

// Make code to receive a value on the incoming Module object.
function makeModuleReceive(localName, moduleName) {
  if (!moduleName) moduleName = localName;
  checkReceiving(moduleName);
  let ret = '';
  if (expectToReceiveOnModule(moduleName)) {
    // Usually the local we use is the same as the Module property name,
    // but sometimes they must differ.
    ret = `\nif (Module['${moduleName}']) ${localName} = Module['${moduleName}'];`;
  }
  ret += makeRemovedModuleAPIAssert(moduleName, localName);
  return ret;
}

function makeModuleReceiveExpr(name, defaultValue) {
  checkReceiving(name);
  if (expectToReceiveOnModule(name)) {
    return `Module['${name}'] || ${defaultValue}`;
  } else {
    return `${defaultValue}`;
  }
}

function makeModuleReceiveWithVar(localName, moduleName, defaultValue, noAssert) {
  if (!moduleName) moduleName = localName;
  checkReceiving(moduleName);
  let ret = 'var ' + localName;
  if (!expectToReceiveOnModule(moduleName)) {
    if (defaultValue) {
      ret += ' = ' + defaultValue;
    }
    ret += ';';
  } else {
    if (defaultValue) {
      ret += ` = Module['${moduleName}'] || ${defaultValue};`;
    } else {
      ret += ';' + makeModuleReceive(localName, moduleName);
      return ret;
    }
  }
  if (!noAssert) {
    ret += makeRemovedModuleAPIAssert(moduleName, localName);
  }
  return ret;
}

function makeRemovedFSAssert(fsName) {
  assert(ASSERTIONS);
  const lower = fsName.toLowerCase();
  if (JS_LIBRARIES.includes('library_' + lower + '.js')) return '';
  return `var ${fsName} = '${fsName} is no longer included by default; build with -l${lower}.js';`;
}

// Given an array of elements [elem1,elem2,elem3], returns a string "['elem1','elem2','elem3']"
function buildStringArray(array) {
  if (array.length > 0) {
    return "['" + array.join("','") + "']";
  } else {
    return '[]';
  }
}

// Generates access to a JS imports scope variable in pthreads worker.js. In MODULARIZE mode these flow into the imports object for the Module.
// In non-MODULARIZE mode, we can directly access the variables in global scope.
function makeAsmImportsAccessInPthread(variable) {
  if (!MINIMAL_RUNTIME) {
    // Regular runtime uses the name "Module" for both imports and exports.
    return `Module['${variable}']`;
  }
  if (MODULARIZE) {
    // MINIMAL_RUNTIME uses 'imports' as the name for the imports object in MODULARIZE builds.
    return `imports['${variable}']`;
  }
  // In non-MODULARIZE builds, can access the imports from global scope.
  return `self.${variable}`;
}

function _asmjsDemangle(symbol) {
  if (symbol in WASM_SYSTEM_EXPORTS) {
    return symbol;
  }
  if (symbol.startsWith('dynCall_')) {
    return symbol;
  }
  // Strip leading "_"
  assert(symbol.startsWith('_'), 'expected mangled symbol: ' + symbol);
  return symbol.substr(1);
}

// TODO(sbc): Remove this function along with _asmjsDemangle.
function hasExportedFunction(func) {
  warnOnce('hasExportedFunction has been replaced with hasExportedSymbol, which takes and unmangled (no leading underscore) symbol name');
  return WASM_EXPORTS.has(_asmjsDemangle(func));
}

function hasExportedSymbol(sym) {
  return WASM_EXPORTS.has(sym);
}

// JS API I64 param handling: if we have BigInt support, the ABI is simple,
// it is a BigInt. Otherwise, we legalize into pairs of i32s.
function defineI64Param(name) {
  if (WASM_BIGINT) {
    return `/** @type {!BigInt} */ ${name}`;
  }
  return `${name}_low, ${name}_high`;
}

function receiveI64ParamAsI32s(name) {
  if (WASM_BIGINT) {
    return `var ${name}_low = Number(${name} & 0xffffffffn) | 0, ${name}_high = Number(${name} >> 32n) | 0;`;
  }
  return '';
}

function receiveI64ParamAsI53(name, onError) {
  if (WASM_BIGINT) {
    // Just convert the bigint into a double.
    return `${name} = bigintToI53Checked(${name}); if (isNaN(${name})) return ${onError};`;
  }
  // Covert the high/low pair to a Number, checking for
  // overflow of the I53 range and returning onError in that case.
  return `var ${name} = convertI32PairToI53Checked(${name}_low, ${name}_high); if (isNaN(${name})) return ${onError};`;
}

function sendI64Argument(low, high) {
  if (WASM_BIGINT) {
    return 'BigInt(low) | (BigInt(high) << 32n)';
  }
  return low + ', ' + high;
}

// Any function called from wasm64 may have bigint args, this function takes
// a list of variable names to convert to number.
function from64(x) {
  if (!MEMORY64) {
    return '';
  }
  if (Array.isArray(x)) {
    let ret = '';
    for (e of x) ret += from64(e);
    return ret;
  }
  return `${x} = Number(${x});`;
}

function to64(x) {
  if (!MEMORY64) return x;
  return `BigInt(${x})`;
}

// Add assertions to catch common errors when using the Promise object we
// create on Module.ready() and return from MODULARIZE Module() invocations.
function addReadyPromiseAssertions(promise) {
  // Warn on someone doing
  //
  //  var instance = Module();
  //  ...
  //  instance._main();
  const properties = Array.from(EXPORTED_FUNCTIONS.values());
  // Also warn on onRuntimeInitialized which might be a common pattern with
  // older MODULARIZE-using codebases.
  properties.push('onRuntimeInitialized');
  const warningEnding = ' on the Promise object, instead of the instance. Use .then() to get called back with the instance, see the MODULARIZE docs in src/settings.js';
  const res = JSON.stringify(properties);
  return res + `.forEach((prop) => {
  if (!Object.getOwnPropertyDescriptor(${promise}, prop)) {
    Object.defineProperty(${promise}, prop, {
      get: () => abort('You are getting ' + prop + '${warningEnding}'),
      set: () => abort('You are setting ' + prop + '${warningEnding}'),
    });
  }
});`;
}

function makeMalloc(source, param) {
  if (hasExportedSymbol('malloc')) {
    return `_malloc(${param})`;
  }
  // It should be impossible to call some functions without malloc being
  // included, unless we have a deps_info.json bug. To let closure not error
  // on `_malloc` not being present, they don't call malloc and instead abort
  // with an error at runtime.
  // TODO: A more comprehensive deps system could catch this at compile time.
  if (!ASSERTIONS) {
    return 'abort();';
  }
  return `abort('malloc was not included, but is needed in ${source}. Adding "_malloc" to EXPORTED_FUNCTIONS should fix that. This may be a bug in the compiler, please file an issue.');`;
}

// Adds a call to runtimeKeepalivePush, if needed by the current build
// configuration.
// We skip this completely in MINIMAL_RUNTIME and also in builds that
// don't ever need to exit the runtime.
function runtimeKeepalivePush() {
  if (MINIMAL_RUNTIME || (EXIT_RUNTIME == 0 && USE_PTHREADS == 0)) return '';
  return 'runtimeKeepalivePush();';
}

// Adds a call to runtimeKeepalivePush, if needed by the current build
// configuration.
// We skip this completely in MINIMAL_RUNTIME and also in builds that
// don't ever need to exit the runtime.
function runtimeKeepalivePop() {
  if (MINIMAL_RUNTIME || (EXIT_RUNTIME == 0 && USE_PTHREADS == 0)) return '';
  return 'runtimeKeepalivePop();';
}

// Some web functions like TextDecoder.decode() may not work with a view of a
// SharedArrayBuffer, see https://github.com/whatwg/encoding/issues/172
// To avoid that, this function allows obtaining an unshared copy of an
// ArrayBuffer.
function getUnsharedTextDecoderView(heap, start, end) {
  const shared = `${heap}.slice(${start}, ${end})`;
  const unshared = `${heap}.subarray(${start}, ${end})`;

  // No need to worry about this in non-shared memory builds
  if (!SHARED_MEMORY) return unshared;

  // If asked to get an unshared view to what we know will be a shared view, or if in -Oz,
  // then unconditionally do a .slice() for smallest code size.
  if (SHRINK_LEVEL == 2 || heap == 'HEAPU8') return shared;

  // Otherwise, generate a runtime type check: must do a .slice() if looking at a SAB,
  // or can use .subarray() otherwise.
  return `${heap}.buffer instanceof SharedArrayBuffer ? ${shared} : ${unshared}`;
}
