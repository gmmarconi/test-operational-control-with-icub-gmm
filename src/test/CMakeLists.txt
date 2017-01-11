cmake_minimum_required(VERSION 2.8.9)
project(MIRTest)

find_package(RTF)
find_package(RTF COMPONENTS DLL)
find_package(YARP)

include_directories(${CMAKE_SOURCE_DIR}
    ${RTF_INCLUDE_DIRS}
    ${YARP_INCLUDE_DIRS}
    )


link_libraries(${RTF_LIBRARIES}
    ${YARP_LIBRARIES})

add_library(${PROJECT_NAME} MODULE MyTest.cpp MyTest.h)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}
        COMPONENT runtime
        LIBRARY DESTINATION lib)

