#include <rtf/TestAssert.h>
#include <rtf/dll/Plugin.h>
#include "MyTest.h"
#include <iostream>
#include <limits>

using namespace RTF;
PREPARE_PLUGIN(MyTest)

void PressEnterToContinue()
  {
  std::cout << "Press ENTER to continue... " << std::flush;
  std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
  }

MyTest::MyTest() : TestCase("MyTest") { }

MyTest::~MyTest() { }

bool MyTest::setup(int argc, char** argv) {
    RTF_TEST_REPORT("running MyTest::setup...");
    PressEnterToContinue();
    return true;
}

void MyTest::tearDown() {
    RTF_TEST_REPORT("running MyTest::teardown...");
    // assert an arbitray error for example.
    RTF_ASSERT_ERROR("this is just for example!");
}

void MyTest::run() {
    RTF_TEST_REPORT("testing integers");
    RTF_TEST_FAIL_IF(2<3, "is not smaller");
    int a = 5;
    int b = 3;
    RTF_TEST_FAIL_IF(a<b, Asserter::format("%d is not smaller than %d.", a, b));
}
