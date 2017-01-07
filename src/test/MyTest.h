#ifndef _MYTEST_H_
#define _MYTEST_H_
#include <rtf/TestCase.h>

class MyTest : public RTF::TestCase {
public:

    MyTest();

    virtual ~MyTest();

    virtual bool setup(int argc, char** argv);

    virtual void tearDown();

    virtual void run();
};
#endif //_MYTEST_H_
