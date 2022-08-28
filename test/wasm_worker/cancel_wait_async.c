#include <emscripten.h>
#include <emscripten/wasm_worker.h>
#include <emscripten/threading.h>
#include <assert.h>

// Test emscripten_cancel_wait_async() function.

EM_JS(void, console_log, (char* str), {
  console.log(UTF8ToString(str));
});

volatile int32_t addr = 1;

EM_BOOL testSucceeded = 1;

void asyncWaitFinishedShouldNotBeCalled(int32_t *ptr, uint32_t val, ATOMICS_WAIT_RESULT_T waitResult, void *userData)
{
  console_log("asyncWaitFinishedShouldNotBeCalled");
  testSucceeded = 0;
  assert(0); // We should not reach here
}

void asyncWaitFinishedShouldBeCalled(int32_t *ptr, uint32_t val, ATOMICS_WAIT_RESULT_T waitResult, void *userData)
{
  console_log("asyncWaitFinishedShouldBeCalled");
#ifdef REPORT_RESULT
  REPORT_RESULT(testSucceeded);
#endif
}

int main()
{
  console_log("Async waiting on address should give a wait token");
  ATOMICS_WAIT_TOKEN_T ret = emscripten_atomic_wait_async((int32_t*)&addr, 1, asyncWaitFinishedShouldNotBeCalled, (void*)42, EMSCRIPTEN_WAIT_ASYNC_INFINITY);
  assert(EMSCRIPTEN_IS_VALID_WAIT_TOKEN(ret));

  console_log("Canceling an async wait should succeed");
  EMSCRIPTEN_RESULT r = emscripten_atomic_cancel_wait_async(ret);
  assert(r == EMSCRIPTEN_RESULT_SUCCESS);

  console_log("Canceling an async wait a second time should give invalid param");
  r = emscripten_atomic_cancel_wait_async(ret);
  assert(r == EMSCRIPTEN_RESULT_INVALID_PARAM);

  console_log("Notifying an async wait should not trigger the callback function");
  int64_t numWoken = emscripten_wasm_notify((int32_t*)&addr, EMSCRIPTEN_NOTIFY_ALL_WAITERS);

  console_log("Notifying an async wait should return 0 threads woken");
  assert(numWoken == 0);

  addr = 2;
  console_log("Notifying an async wait even after changed value should not trigger the callback function");
  numWoken = emscripten_wasm_notify((int32_t*)&addr, EMSCRIPTEN_NOTIFY_ALL_WAITERS);

  console_log("Notifying an async wait should return 0 threads woken");
  assert(numWoken == 0);

  console_log("Async waiting on address should give a wait token");
  ret = emscripten_atomic_wait_async((int32_t*)&addr, 2, asyncWaitFinishedShouldBeCalled, (void*)42, EMSCRIPTEN_WAIT_ASYNC_INFINITY);
  assert(EMSCRIPTEN_IS_VALID_WAIT_TOKEN(ret));

#if 0
  console_log("Notifying an async wait without value changing should still trigger the callback");
  numWoken = emscripten_wasm_notify((int32_t*)&addr, EMSCRIPTEN_NOTIFY_ALL_WAITERS);
  assert(numWoken == 1);
#else
  // TODO: Switch to the above test instead after the Atomics.waitAsync() polyfill is dropped.
  addr = 3;
  console_log("Notifying an async wait after value changing should trigger the callback");
  numWoken = emscripten_wasm_notify((int32_t*)&addr, EMSCRIPTEN_NOTIFY_ALL_WAITERS);
#endif
}
