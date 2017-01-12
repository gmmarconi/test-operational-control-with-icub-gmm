cmake_minimum_required(VERSION 2.8.9)
project(make-it-roll-gianma)

find_package(YARP REQUIRED)
find_package(ICUBcontrib)

list(APPEND CMAKE_MODULE_PATH ${ICUBCONTRIB_MODULE_PATH})

include(ICUBcontribHelpers)
include(ICUBcontribOptions)
icubcontrib_set_default_prefix()
include_directories(${YARP_INCLUDE_DIRS}
		    ${ICUB_INCLUDE_DIRS}
			)

set(folder_source ${folder_source} main.cpp)
# set up our program
add_executable(${PROJECT_NAME} ${folder_source})
# link with YARP libraries
target_link_libraries(${PROJECT_NAME} ${YARP_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
        RUNTIME
        DESTINATION bin)
icubcontrib_add_uninstall_target()
