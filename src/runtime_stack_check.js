/**
 * @license
 * Copyright 2019 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

#if STACK_OVERFLOW_CHECK
// Initializes the stack cookie. Called at the startup of main and at the startup of each thread in pthreads mode.
function writeStackCookie() {
  var max = _emscripten_stack_get_end();
#if RUNTIME_DEBUG
  err('writeStackCookie: ' + max.toString(16));
#endif
#if ASSERTIONS
  assert((max & 3) == 0);
#endif
  // The stack grow downwards towards _emscripten_stack_get_end.
  // We write cookies to the final two words in the stack and detect if they are
  // ever overwritten.
  {{{ makeSetValue('max', 0, '0x2135467', 'i32' ) }}};
  {{{ makeSetValue('max', 4, '0x89BACDFE', 'i32' ) }}};
#if !USE_ASAN && !SAFE_HEAP // ASan and SAFE_HEAP check address 0 themselves
  // Also test the global address 0 for integrity.
  HEAPU32[0] = 0x63736d65; /* 'emsc' */
#endif
}

function checkStackCookie() {
#if !MINIMAL_RUNTIME
  if (ABORT) return;
#endif
  var max = _emscripten_stack_get_end();
#if RUNTIME_DEBUG
  err('checkStackCookie: ' + max.toString(16));
#endif
  var cookie1 = {{{ makeGetValue('max', 0, 'u32') }}};
  var cookie2 = {{{ makeGetValue('max', 4, 'u32') }}};
  if (cookie1 != 0x2135467 || cookie2 != 0x89BACDFE) {
    abort('Stack overflow! Stack cookie has been overwritten at 0x' + max.toString(16) + ', expected hex dwords 0x89BACDFE and 0x2135467, but received 0x' + cookie2.toString(16) + ' 0x' + cookie1.toString(16));
  }
#if !USE_ASAN && !SAFE_HEAP // ASan and SAFE_HEAP check address 0 themselves
  // Also test the global address 0 for integrity.
  if (HEAPU32[0] !== 0x63736d65 /* 'emsc' */) abort('Runtime error: The application has corrupted its heap memory area (address zero)!');
#endif
}
#endif
