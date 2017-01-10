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

    rf.configure(argc, argv);
    rf.setVerbose(true);
    yarp::os::ConstString name, server_name;
    name = rf.check("name", Value("/MIRTest/rpc"), "Getting tester port name").asString();
    server_name = rf.check("server_name", Value("/service"), "Getting program port name").asString();
    portMIR.open(name);
    RTF_ASSERT_ERROR_IF(Network::connect(name,server_name,"tcp"),
                    Asserter::format("Failed to connect to %s!",server_name.c_str()));

    portiCubSim.open("/MIRTest/icubSIM/world");
    RTF_ASSERT_ERROR_IF(Network::connect("/MIRTest/icubSIM/world","/icubSim/world","tcp"),
                    Asserter::format("Failed to connect to /icubSim/world!"));

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
    int ball_y = response.get(1).asInt();

    RTF_TEST_REPORT("Sending message 'roll'");
    msg.clear();
    msg.addString("roll");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "iCubSim couldn't make it roll");

    Time::delay(1.5);
    RTF_TEST_REPORT("Retrieving ball position");
    msg.clear();
    response.clear();
    msg.addString("world get ball");
    RTF_ASSERT_ERROR_IF(portiCubSim.write(msg, response), "Couldn't retrieve ball position");
    RTF_TEST_FAIL_IF( (ball_y - response.get(1).asInt()) < 0.1, "The ball did not fall off the table");

    RTF_TEST_REPORT("Sending message 'home'");
    msg.clear();
    msg.addString("home");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "iCubSim couldn't go home");

    RTF_TEST_REPORT("Sending message 'quit'");
    msg.clear();
    msg.addString("quit");
    RTF_TEST_FAIL_IF(portMIR.write(msg, response), "program couldn't be closed");

}
