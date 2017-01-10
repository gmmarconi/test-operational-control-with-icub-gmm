#ifndef _MYTEST_H_
#define _MYTEST_H_
#include <yarp/os/all.h>

class MyTest : public RTF::TestCase {
public:
    yarp::os::RpcClient portMIR, portiCubSim;
    yarp::os::ResourceFinder rf;

    MyTest();

    virtual ~MyTest();

    virtual bool setup(int argc, char** argv);

    virtual void tearDown();

    virtual void run();
};
#endif //_MYTEST_H_
