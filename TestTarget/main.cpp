#include "x.h"

#include <cstring>
#include <iostream>

#include <jerryscript.h>


using namespace std;


static jerry_value_t boolptrHandler(
  const jerry_value_t ,
  const jerry_value_t ,
  const jerry_value_t [],
  const jerry_length_t )
{
  bool *b = (bool *)malloc(sizeof(bool));

  auto ret = jerry_create_object();
  jerry_set_object_native_pointer(ret, b, nullptr);

  return ret;
}


void print_unhandled_exception(jerry_value_t error_value, uint8_t *buffer)
{
  #define SYNTAX_ERROR_CONTEXT_SIZE 2

  if (!(!jerry_value_is_error(error_value)))
  {
    printf("assert(!jerry_value_is_error(error_value)\n");
    return;
  }

  jerry_char_t err_str_buf[256];

  if (jerry_value_is_object(error_value))
  {
    jerry_value_t stack_str = jerry_create_string((const jerry_char_t *)"stack");
    jerry_value_t backtrace_val = jerry_get_property(error_value, stack_str);
    jerry_release_value(stack_str);

    if (!jerry_value_is_error(backtrace_val) && jerry_value_is_array(backtrace_val))
    {
      printf("Exception backtrace:\n");

      uint32_t length = jerry_get_array_length(backtrace_val);

      /* This length should be enough. */
      if (length > 32)
      {
        length = 32;
      }

      for (uint32_t i = 0; i < length; i++)
      {
        jerry_value_t item_val = jerry_get_property_by_index(backtrace_val, i);

        if (/*!jerry_value_is_error (item_val) && */jerry_value_is_string (item_val))
        {
          jerry_size_t str_size = jerry_get_string_size(item_val);

          if (str_size >= 256)
          {
            printf ("%3u: [Backtrace string too long]\n", i);
          }
          else
          {
            jerry_size_t string_end = jerry_string_to_char_buffer(item_val, err_str_buf, str_size);
            if (!(string_end == str_size))
            {
              printf("assert (string_end == str_size)\n");
              return;
            }

            err_str_buf[string_end] = 0;

            printf ("%3u: %s\n", i, err_str_buf);
          }
        }

        jerry_release_value (item_val);
      }
    }

    jerry_release_value(backtrace_val);
  }

  jerry_value_t err_str_val = jerry_value_to_string(error_value);
  jerry_size_t err_str_size = jerry_get_string_size(err_str_val);

  if (err_str_size >= 256)
  {
    const char msg[] = "[Error message too long]";
    err_str_size = sizeof (msg) / sizeof (char) - 1;
    memcpy (err_str_buf, msg, err_str_size + 1);
  }
  else
  {
    jerry_size_t string_end = jerry_string_to_char_buffer(err_str_val, err_str_buf, err_str_size);
    if (!(string_end == err_str_size))
    {
      printf("assert (string_end (%d) == err_str_size (%d))\n", string_end, err_str_size);
      return;
    }
    err_str_buf[string_end] = 0;

    if (jerry_is_feature_enabled(JERRY_FEATURE_ERROR_MESSAGES) && jerry_get_error_type(error_value) == JERRY_ERROR_SYNTAX)
    {
      unsigned int err_line = 0;
      unsigned int err_col = 0;

      /* 1. parse column and line information */
      for (jerry_size_t i = 0; i < string_end; i++)
      {
        if (!strncmp ((char *) (err_str_buf + i), "[line: ", 7))
        {
          i += 7;

          char num_str[8];
          unsigned int j = 0;

          while (i < string_end && err_str_buf[i] != ',')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_line = (unsigned int) strtol (num_str, NULL, 10);

          if (strncmp ((char *) (err_str_buf + i), ", column: ", 10))
          {
            break; /* wrong position info format */
          }

          i += 10;
          j = 0;

          while (i < string_end && err_str_buf[i] != ']')
          {
            num_str[j] = (char) err_str_buf[i];
            j++;
            i++;
          }
          num_str[j] = '\0';

          err_col = (unsigned int) strtol (num_str, NULL, 10);
          break;
        }
      } /* for */

      if (err_line != 0 && err_col != 0)
      {
        unsigned int curr_line = 1;

        bool is_printing_context = false;
        unsigned int pos = 0;

        /* 2. seek and print */
        while (buffer[pos] != '\0')
        {
          if (buffer[pos] == '\n')
          {
            curr_line++;
          }

          if (err_line < SYNTAX_ERROR_CONTEXT_SIZE
              || (err_line >= curr_line
                  && (err_line - curr_line) <= SYNTAX_ERROR_CONTEXT_SIZE))
          {
            /* context must be printed */
            is_printing_context = true;
          }

          if (curr_line > err_line)
          {
            break;
          }

          if (is_printing_context)
          {
            printf("%c", buffer[pos]);
          }

          pos++;
        }

        printf("\n");

        while (--err_col)
        {
          printf("~");
        }

        printf("^\n");
      }
    }
  }

  printf("Script Error: %s\n", err_str_buf);
  jerry_release_value(err_str_val);
} /* print_unhandled_exception */


extern void RTJS_INIT();


int main()
{
  std::cerr << "main" << std::endl;

  RTJS_INIT();

  auto globObj = jerry_get_global_object();
  jerry_value_t boolptrfn = jerry_create_external_function(boolptrHandler);
  jerry_value_t boolptrfnName= jerry_create_string((const jerry_char_t *)"createBoolPointer");
  jerry_set_property(globObj, boolptrfnName, boolptrfn);


  {
    const char *test = "x(false);";
    jerry_value_t eval = jerry_eval((const jerry_char_t *)test, std::strlen(test), 0);

    jerry_error_t err = jerry_get_error_type(eval);
    if (err != JERRY_ERROR_NONE)
    {
      std::cerr << ":( #1" << std::endl;
      return -1;
    }

    std::cerr << ":) #1" << std::endl;
  }


  {
    const char *test = "var boolptr = createBoolPointer(); x2(boolptr);";
    jerry_value_t eval = jerry_eval((const jerry_char_t *)test, std::strlen(test), 0);

    jerry_error_t err = jerry_get_error_type(eval);
    if (err != JERRY_ERROR_NONE)
    {
      std::cerr << ":( #2" << std::endl;
      return -1;
    }

    std::cerr << ":) #2" << std::endl;
  }


  cerr << "TestTarget console" << endl << endl;


  while (true)
  {
    cerr << "> ";

    std::string x;
    std::getline(std::cin, x);

    if (x == "exit")
      return 0;

    jerry_value_t r;

    try
    {
      r = jerry_eval((const jerry_char_t *)x.c_str(), x.length(), JERRY_PARSE_STRICT_MODE);
    }
    catch (const std::string &msg)
    {
      cerr << "JS exception: " << msg << endl;
    }

    auto et = jerry_get_error_type(r);

    if (et != JERRY_ERROR_NONE)
    {
      auto y = jerry_get_value_from_error(r, true);
      print_unhandled_exception(y, (uint8_t *)x.c_str());
      continue;
    }


    auto rt = jerry_value_get_type(r);

    std::string result;
    switch (rt)
    {
      case JERRY_TYPE_NONE:
      {
        result = "(NONE)";
        break;
      }

      case JERRY_TYPE_UNDEFINED:
      {
        result = "(undefined)";
        break;
      }

      case JERRY_TYPE_NULL:
      {
        result = "(null)";
        break;
      }

      case JERRY_TYPE_BOOLEAN:
      {
        result = jerry_get_boolean_value(r) ? "true" : "false";
        break;
      }

      case JERRY_TYPE_NUMBER:
      {
        result = to_string(jerry_get_number_value(r));
        break;
      }

      case JERRY_TYPE_STRING:
      {
        auto strsize = jerry_get_utf8_string_size(r);
        jerry_char_t *buf = (jerry_char_t *)malloc(strsize);
        jerry_string_to_utf8_char_buffer(r, buf, strsize);
        free(buf);

        result = std::string((const char *)buf);
        break;
      }

      case JERRY_TYPE_OBJECT:
      {
        result = "[object]";
        break;
      }

      case JERRY_TYPE_FUNCTION:
      {
        result = "[f]";
        break;
      }

      case JERRY_TYPE_ERROR:
      {
        result = "(error)";
        break;
      }

      case JERRY_TYPE_SYMBOL:
      {
        result = "(symbol)";
        break;
      }
    }

    cerr << result << endl;
  }

  return 0;
}





























//
