#ifndef _MYFIXMANAGER_H_
#define _MYFIXMANAGER_H_

#include <rtf/FixtureManager.h>
#include <yarp/os/all.h>

class MyFixManager : public RTF::FixtureManager {
public:

    yarp::os::Network yarp;

    virtual bool setup(int argc, char** argv);

    virtual void tearDown();
};

#endif //_MYFIXMANAGER_H_
