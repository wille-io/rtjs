
  // => create class
  jerry_value_t x_obj = jerry_create_object();
  jerry_value_t x_obj_name = jerry_create_string((const jerry_char_t *)"%1");
  jerry_release_value(jerry_set_property(glob_obj, x_obj_name, x_obj));
  jerry_release_value(x_obj_name);
  jerry_release_value(x_obj);

  %2

  jerry_release_value(glob_obj);
