cmake_minimum_required(VERSION 2.8.9)
project(make-it-roll)

find_package(YARP REQUIRED)
include_directories(${YARP_INCLUDE_DIRS})

set(folder_source ${folder_source} main.cpp)
# set up our program
add_executable(${PROJECT_NAME} ${folder_source})
# link with YARP libraries
target_link_libraries(${PROJECT_NAME} ${YARP_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
        RUNTIME
        DESTINATION bin)
