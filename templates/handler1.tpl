static jerry_value_t _rtjs_%1_handler(
  const jerry_value_t function_obj,
  const jerry_value_t this_val,
  const jerry_value_t args[],
  const jerry_length_t argc)
{
  std::cout << "_rtjs_%1_handler called" << std::endl;

  if (argc != %2)
    throw std::string("_rtjs_%1_handler called with invalid argument count ") + std::to_string(%2) + std::string(" (must be %2)");

