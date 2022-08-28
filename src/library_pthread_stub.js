/**
 * @license
 * Copyright 2015 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

#if USE_PTHREADS
#error "Internal error! USE_PTHREADS should not be enabled when including library_pthread_stub.js."
#endif
#if STANDALONE_WASM && SHARED_MEMORY
#error "STANDALONE_WASM does not support shared memories yet"
#endif

var LibraryPThreadStub = {
  // ===================================================================================
  // Stub implementation for pthread.h when not compiling with pthreads support enabled.
  // ===================================================================================

  emscripten_is_main_browser_thread: function() {
#if MINIMAL_RUNTIME
    return typeof importScripts == 'undefined';
#else
    return !ENVIRONMENT_IS_WORKER;
#endif
  },
};

mergeInto(LibraryManager.library, LibraryPThreadStub);
