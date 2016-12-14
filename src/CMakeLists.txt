cmake_minimum_required(VERSION 2.8.9)
project(make-it-roll)
set(PROJECT_NAME make-it-roll)
find_package(YARP REQUIRED)
include_directories(${YARP_INCLUDE_DIRS})

set(folder_source ${folder_source}    main.cpp)
source_group("Source Files" FILES ${folder_source})
# set up our program
add_executable(${PROJECT_NAME} ${folder_source} ${folder_header})
# link with YARP libraries
target_link_libraries(${PROJECT_NAME} ${YARP_LIBRARIES})
