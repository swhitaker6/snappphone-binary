/**
 * @license
 * Copyright 2010 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

// "use strict";

// General JS utilities - things that might be useful in any JS project.
// Nothing specific to Emscripten appears here.

function safeQuote(x) {
  return x.replace(/"/g, '\\"').replace(/'/g, "\\'");
}

function dump(item) {
  let funcData;
  try {
    if (typeof item == 'object' && item != null && item.funcData) {
      funcData = item.funcData;
      item.funcData = null;
    }
    return '// ' + JSON.stringify(item, null, '  ').replace(/\n/g, '\n// ');
  } catch (e) {
    const ret = [];
    for (const i in item) {
      if (Object.prototype.hasOwnProperty.call(item, i)) {
        const j = item[i];
        if (typeof j == 'string' || typeof j == 'number') {
          ret.push(i + ': ' + j);
        } else {
          ret.push(i + ': [?]');
        }
      }
    }
    return ret.join(',\n');
  } finally {
    if (funcData) item.funcData = funcData;
  }
}

global.warnings = false;

function warn(a, msg) {
  global.warnings = true;
  if (!msg) {
    msg = a;
    a = false;
  }
  if (!a) {
    printErr('warning: ' + msg);
  }
}

function warnOnce(a, msg) {
  if (!msg) {
    msg = a;
    a = false;
  }
  if (!a) {
    if (!warnOnce.msgs) warnOnce.msgs = {};
    if (msg in warnOnce.msgs) return;
    warnOnce.msgs[msg] = true;
    warn(msg);
  }
}

global.abortExecution = false;

function error(msg) {
  abortExecution = true;
  printErr('error: ' + msg);
}

function range(size) {
  return Array.from(Array(size).keys());
}

function bind(self, func) {
  return function(...args) {
    func.apply(self, args);
  };
}

function sum(x) {
  return x.reduce((a, b) => a + b, 0);
}

// options is optional input object containing mergeInto params
// currently, it can contain
//
// key: noOverride, value: true
// if it is set, it prevents symbol redefinition and shows error
// in case of redefinition
function mergeInto(obj, other, options = null) {
  // check for unintended symbol redefinition
  if (options && options.noOverride) {
    for (const key of Object.keys(other)) {
      if (obj.hasOwnProperty(key)) {
        error('Symbol re-definition in JavaScript library: ' + key + '. Do not use noOverride if this is intended');
        return;
      }
    }
  }

  return Object.assign(obj, other);
}

function isNumber(x) {
  // XXX this does not handle 0xabc123 etc. We should likely also do x == parseInt(x) (which handles that), and remove hack |// handle 0x... as well|
  return x == parseFloat(x) || (typeof x == 'string' && x.match(/^-?\d+$/)) || x == 'NaN';
}

function isJsLibraryConfigIdentifier(ident) {
  suffixes = [
    '__sig',
    '__proxy',
    '__asm',
    '__inline',
    '__deps',
    '__postset',
    '__docs',
    '__nothrow',
    '__noleakcheck',
    '__internal',
    '__user',
  ];
  return suffixes.some((suffix) => ident.endsWith(suffix));
}

function isPowerOfTwo(x) {
  return x > 0 && ((x & (x - 1)) == 0);
}

/** @constructor */
function Benchmarker() {
  const totals = {};
  const ids = [];
  const lastTime = 0;
  this.start = function(id) {
    const now = Date.now();
    if (ids.length > 0) {
      totals[ids[ids.length - 1]] += now - lastTime;
    }
    lastTime = now;
    ids.push(id);
    totals[id] = totals[id] || 0;
  };
  this.stop = function(id) {
    const now = Date.now();
    assert(id === ids[ids.length - 1]);
    totals[id] += now - lastTime;
    lastTime = now;
    ids.pop();
  };
  this.print = function(text) {
    const ids = Object.keys(totals);
    if (ids.length > 0) {
      ids.sort((a, b) => totals[b] - totals[a]);
      printErr(text + ' times: \n' + ids.map((id) => id + ' : ' + totals[id] + ' ms').join('\n'));
    }
  };
}
