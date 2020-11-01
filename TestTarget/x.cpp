#include "x.h"

#include <iostream> // .......


bool x(bool y)
{
  std::cerr << "x called" << std::endl;
  return y;
}


bool *x2(bool *y)
{
  std::cerr << "x2 called" << std::endl;
  return y;
}


void TestClass::test()
{
  std::cerr << "TestClass::test called" << std::endl;
}


void TestClass::static_test()
{
  std::cerr << "TestClass::static_test called" << std::endl;
}
