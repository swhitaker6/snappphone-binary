// Copyright 2021 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>

using namespace emscripten;
using namespace std;

void fail()
{
  cout << "fail\n";
}

void pass()
{
  cout << "pass\n";
}

void test(string message)
{
  cout << "test:\n" << message << "\n";
}

void ensure(bool value)
{
  if (value)
    pass();
  else
    fail();
}

void ensure_js(string js_code)
{
  js_code.append(";");
  const char* js_code_pointer = js_code.c_str();
  ensure(EM_ASM_INT({
    var js_code = UTF8ToString($0);
    return eval(js_code);
  }, js_code_pointer));
}

template <typename T>
string compare_a_64_js(T value) {
  stringstream ss;
  ss << "a === " << value << "n";
  return ss.str();
}

int main()
{
  const int64_t max_int64_t = numeric_limits<int64_t>::max();
  const int64_t min_int64_t = numeric_limits<int64_t>::min();
  const uint64_t max_uint64_t = numeric_limits<uint64_t>::max();

  printf("start\n");

  test("val(int64_t v)");
  val::global().set("a", val(int64_t(1234)));
  ensure_js("a === 1234n");

  val::global().set("a", val(int64_t(-4321)));
  ensure_js("a === -4321n");

  val::global().set("a", val(int64_t(0x12345678aabbccddL)));
  ensure_js("a === 1311768467732155613n");
  ensure(val::global()["a"].as<int64_t>() == 0x12345678aabbccddL);

  test("val(uint64_t v)");
  val::global().set("a", val(uint64_t(1234)));
  ensure_js("a === 1234n");

  val::global().set("a", val(max_uint64_t));
  ensure_js(compare_a_64_js(max_uint64_t));
  ensure(val::global()["a"].as<uint64_t>() == max_uint64_t);

  test("val(int64_t v)");
  val::global().set("a", val(max_int64_t));
  ensure_js(compare_a_64_js(max_int64_t));
  ensure(val::global()["a"].as<int64_t>() == max_int64_t);

  val::global().set("a", val(min_int64_t));
  ensure_js(compare_a_64_js(min_int64_t));
  ensure(val::global()["a"].as<int64_t>() == min_int64_t);

  printf("end\n");
  return 0;
}
