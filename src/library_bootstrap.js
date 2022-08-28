/**
 * @license
 * Copyright 2015 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

// When bootstrapping struct info, we can't use the full library because
// it itself depends on the struct info information.

#if !BOOTSTRAPPING_STRUCT_INFO
assert(false, "library_bootstrap.js only designed for use with BOOTSTRAPPING_STRUCT_INFO")
#endif

assert(Object.keys(LibraryManager.library).length === 0);
mergeInto(LibraryManager.library, {
  $callRuntimeCallbacks: function() {},

  $ExitStatus__docs: '/** @constructor */',
  $ExitStatus: function(status) {
    this.name = 'ExitStatus';
    this.message = 'Program terminated with exit(' + status + ')';
    this.status = status;
  },

  $exitJS__deps: ['$ExitStatus'],
  $exitJS: function(code) {
    quit_(code, new ExitStatus(code));
  },

  $handleException: function(e) {
    if (e !== 'unwind') {
      throw e;
    }
  },

  // printf/puts implementations for when musl is not pulled in - very
  // partial, but enough for bootstrapping structInfo
  printf__deps: ['$formatString', '$intArrayToString'],
  printf__sig: 'ipp',
  printf: function(format, varargs) {
    // int printf(const char *restrict format, ...);
    // http://pubs.opengroup.org/onlinepubs/000095399/functions/printf.html
    // extra effort to support printf, even without a filesystem. very partial, very hackish
    var result = formatString(format, varargs);
    var string = intArrayToString(result);
    if (string[string.length-1] === '\n') string = string.substr(0, string.length-1); // remove a final \n, as Module.print will do that
    out(string);
    return result.length;
  },

  puts__sig: 'ip',
  puts: function(s) {
    // extra effort to support puts, even without a filesystem. very partial, very hackish
    var result = UTF8ToString(s);
    var string = result.substr(0);
    if (string[string.length-1] === '\n') string = string.substr(0, string.length-1); // remove a final \n, as Module.print will do that
    out(string);
    return result.length;
  },
});
