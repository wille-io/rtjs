
  // function
  {
    jerry_value_t prop_name = jerry_create_string((const jerry_char_t *)"%1");
    jerry_value_t func_val = jerry_create_external_function(_rtjs_%2_handler);
    jerry_release_value(jerry_set_property(%3, prop_name, func_val));
    jerry_release_value(prop_name);
    jerry_release_value(func_val);
  }
