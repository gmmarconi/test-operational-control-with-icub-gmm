#include <stdio.h>
#include <rtf/dll/Plugin.h>
#include "MyFixManager.h"
#include <yarp/os/all.h>

//#include <iostream>
//#include <limits>

using namespace yarp::os;
using namespace RTF;

PREPARE_FIXTURE_PLUGIN(MyFixManager)

bool MyFixManager::setup(int argc, char** argv) {
    printf("Called from fixture plugin: setupping fixture...\n");
    if (!yarp.checkNetwork())
    {
        printf("Yarp network not available\n");
        return 1;
    }
    //std::cout << "Press ENTER to continue... " << std::flush;
    //std::cin.ignore( std::numeric_limits <std::streamsize> ::max(), '\n' );
    // do the setup here
    // ...
    return true;
}

void MyFixManager::tearDown() {
    printf("Called from fixture plugin: tearing down the fixture...\n");

    // do the tear down here
    // ...
}
