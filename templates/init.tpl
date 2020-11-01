void __rtjs_init_%1()
{
  jerry_init(JERRY_INIT_EMPTY);

  jerry_value_t glob_obj = jerry_get_global_object();

  %2
}
