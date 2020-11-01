#include <jerryscript.h>
#include <iostream>

#include "x.h"


static jerry_value_t _rtjs_TestClass_static_test_handler(
  const jerry_value_t function_obj,
  const jerry_value_t this_val,
  const jerry_value_t args[],
  const jerry_length_t argc)
{
  std::cout << "_rtjs_TestClass_static_test_handler called" << std::endl;

  if (argc != 0)
    throw std::string("_rtjs_TestClass_static_test_handler called with invalid argument count ") + std::to_string(0) + std::string(" (must be 0)");

  return jerry_create_undefined();
}

static jerry_value_t _rtjs_TestClass_test_handler(
  const jerry_value_t function_obj,
  const jerry_value_t this_val,
  const jerry_value_t args[],
  const jerry_length_t argc)
{
  std::cout << "_rtjs_TestClass_test_handler called" << std::endl;

  if (argc != 0)
    throw std::string("_rtjs_TestClass_test_handler called with invalid argument count ") + std::to_string(0) + std::string(" (must be 0)");

  return jerry_create_undefined();
}

jerry_value_t _rtjs_create_TestClass_object(TestClass *class_ptr)
{
  auto classObj = jerry_create_object();
  jerry_set_object_native_pointer(classObj, (void *)class_ptr, nullptr);

  // function
  {
    jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"test");
    jerry_value_t func_val = jerry_create_external_function(_rtjs_TestClass_test_handler);
    jerry_release_value(jerry_set_property(classObj, prop_name, func_val));
    jerry_release_value(prop_name);
    jerry_release_value(func_val);
  }

  return classObj;
}

static jerry_value_t _rtjs_TestClass_ctor0_handler(
  const jerry_value_t function_obj,
  const jerry_value_t this_val,
  const jerry_value_t args[],
  const jerry_length_t argc)
{
  std::cout << "_rtjs_TestClass_ctor0_handler called" << std::endl;

  if (argc != 0)
    throw std::string("_rtjs_TestClass_ctor0_handler called with invalid argument count ") + std::to_string(0) + std::string(" (must be 0)");

  auto *class_ptr = new TestClass;
  auto classObj = _rtjs_create_TestClass_object(class_ptr);
  jerry_set_object_native_pointer(classObj, (void *)class_ptr, nullptr);

  // function
  {
    jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"test");
    jerry_value_t func_val = jerry_create_external_function(_rtjs_TestClass_test_handler);
    jerry_release_value(jerry_set_property(classObj, prop_name, func_val));
    jerry_release_value(prop_name);
    jerry_release_value(func_val);
  }

  return classObj;
}

static jerry_value_t _rtjs_x_handler(
  const jerry_value_t function_obj,
  const jerry_value_t this_val,
  const jerry_value_t args[],
  const jerry_length_t argc)
{
  std::cout << "_rtjs_x_handler called" << std::endl;

  if (argc != 1)
    throw std::string("_rtjs_x_handler called with invalid argument count ") + std::to_string(1) + std::string(" (must be 1)");

  auto _param0 = jerry_value_to_boolean(args[0]);
  auto param0 = _param0;
  auto ret = x(param0);
  std::cerr << "ret = " << ret << std::endl;
  return jerry_create_boolean(ret);

}

static jerry_value_t _rtjs_x2_handler(
  const jerry_value_t function_obj,
  const jerry_value_t this_val,
  const jerry_value_t args[],
  const jerry_length_t argc)
{
  std::cout << "_rtjs_x2_handler called" << std::endl;

  if (argc != 1)
    throw std::string("_rtjs_x2_handler called with invalid argument count ") + std::to_string(1) + std::string(" (must be 1)");

  auto jsParam0 = args[0];
  void *param0Ptr = nullptr;
  if (!jerry_get_object_native_pointer(jsParam0, &param0Ptr, nullptr))
    throw std::string("some handler called with no raw pointer behind it!");
  auto *_param0 = static_cast<bool *>(param0Ptr);
  auto param0 = _param0;
  auto ret = x2(param0);
  std::cerr << "ret = " << ret << std::endl;
  jerry_value_t retObj = jerry_create_object();
  jerry_set_object_native_pointer(retObj, (void *)ret, nullptr);
  return retObj;

}

void __rtjs_init_TestTarget()
{
  jerry_init(JERRY_INIT_EMPTY);

  jerry_value_t glob_obj = jerry_get_global_object();

  
  // function
  {
    jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"x");
    jerry_value_t func_val = jerry_create_external_function(_rtjs_x_handler);
    jerry_release_value(jerry_set_property(glob_obj, prop_name, func_val));
    jerry_release_value(prop_name);
    jerry_release_value(func_val);
  }

  // function
  {
    jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"x2");
    jerry_value_t func_val = jerry_create_external_function(_rtjs_x2_handler);
    jerry_release_value(jerry_set_property(glob_obj, prop_name, func_val));
    jerry_release_value(prop_name);
    jerry_release_value(func_val);
  }

  // class
  {
    auto classObj = jerry_create_object();

    {
      auto ctor = jerry_create_external_function(_rtjs_TestClass_ctor0_handler);
      auto ctorName = jerry_create_string((const jerry_char_t *)"ctor0");
      auto prop = jerry_set_property(classObj, ctorName, ctor);
    }
    {
      auto staticFunction = jerry_create_external_function(_rtjs_TestClass_static_test_handler);
      auto staticFunctionName = jerry_create_string((const jerry_char_t *)"static_test");
      auto prop = jerry_set_property(classObj, staticFunctionName, staticFunction);
    }

    auto classObjName = jerry_create_string((const jerry_char_t *)"TestClass");
    jerry_set_property(glob_obj, classObjName, classObj);
  }

}
