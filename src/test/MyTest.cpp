#include <rtf/TestAssert.h>
#include <rtf/dll/Plugin.h>
#include "MyTest.h"
#include <yarp/os/ConstString.h>

using namespace RTF;
using namespace yarp::os;
PREPARE_PLUGIN(MyTest)

MyTest::MyTest() : TestCase("MyTest") { }

MyTest::~MyTest() { }

bool MyTest::setup(int argc, char** argv) {
    RTF_TEST_REPORT("running MIRTest::setup...");
    RTF_ASSERT_ERROR_IF(!yarp.checkNetwork(),
                    Asserter::format("Unable to find network"));

    rf.configure(argc, argv);
    rf.setVerbose(true);
    yarp::os::ConstString name, server_name;
    name = rf.check("name", Value("/MIRTest/rpc"), "Getting tester port name").asString();
    server_name = rf.check("server_name", Value("/service"), "Getting program port name").asString();
    portMIR.open(name);
    RTF_ASSERT_ERROR_IF(!yarp.connect(name, server_name),
                        Asserter::format("Failed to connect to tested program port"));

    portiCubSim.open("/MIRTest/icubSIM/world");
    RTF_ASSERT_ERROR_IF(!yarp.connect("/MIRTest/icubSIM/world", "/icubSim/world"),
                        Asserter::format("Failed to connect to iCubSim World port"));
    return true;
}

void MyTest::tearDown() {
    RTF_TEST_REPORT("Teardown: closing RPC port");
    if (portMIR.asPort().isOpen())
        portMIR.close();
    if (portiCubSim.asPort().isOpen())
        portiCubSim.asPort().isOpen();
}

void MyTest::run() {
    Bottle msg, response;

    RTF_TEST_REPORT("Sending message 'look_down'");
    msg.clear();
    msg.addString("look_down");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "iCubSim couldn't look down");

    RTF_TEST_REPORT("Retrieving ball position");
    msg.clear();
    response.clear();
    msg.addString("world get ball");
    RTF_ASSERT_ERROR_IF(portiCubSim.write(msg, response), "Couldn't retrieve ball position");
    std::printf("%s\n", response.toString().c_str());

    RTF_TEST_REPORT("Sending message 'roll'");
    msg.clear();
    msg.addString("roll");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "iCubSim couldn't make it roll");

    RTF_TEST_REPORT("Retrieving ball position");
    msg.clear();
    response.clear();
    msg.addString("world get ball");
    RTF_ASSERT_ERROR_IF(portiCubSim.write(msg, response), "Couldn't retrieve ball position");
    std::printf("%s\n", response.toString().c_str());

    RTF_TEST_REPORT("Sending message 'home'");
    msg.clear();
    msg.addString("home");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "iCubSim couldn't go home");

    RTF_TEST_REPORT("Sending message 'close'");
    msg.clear();
    msg.addString("close");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "program couldn't be closed");

}
