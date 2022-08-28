#!/usr/bin/env node
// Copyright 2018 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.
//
// Preprocessor tool.  This is a wrapper for the 'preprocess' function which
// allows it to be called as a standalone tool.
//
// Parameters:
//    setting file.  Can specify 'settings.js' here, alternatively create a temp
//                   file with modified settings and supply the filename here.
//    shell file     This is the file that will be processed by the preprocessor

'use strict';

const fs = require('fs');
const path = require('path');

const arguments_ = process['argv'].slice(2);
const debug = false;

global.print = function(x) {
  process['stdout'].write(x + '\n');
};
global.printErr = function(x) {
  process['stderr'].write(x + '\n');
};

global.assert = require('assert');

function find(filename) {
  const prefixes = [process.cwd(), path.join(__dirname, '..', 'src')];
  for (let i = 0; i < prefixes.length; ++i) {
    const combined = path.join(prefixes[i], filename);
    if (fs.existsSync(combined)) {
      return combined;
    }
  }
  return filename;
}

global.read = function(filename) {
  const absolute = find(filename);
  return fs.readFileSync(absolute).toString();
};

global.load = function(f) {
  eval.call(null, read(f));
};

const settingsFile = arguments_[0];
const shellFile = arguments_[1];
const expandMacros = arguments_.includes('--expandMacros');

load(settingsFile);
load('utility.js');
load('modules.js');
load('parseTools.js');

const fromHTML = read(shellFile);
const toHTML = expandMacros ? processMacros(preprocess(fromHTML, shellFile)) : preprocess(fromHTML, shellFile);

print(toHTML);
