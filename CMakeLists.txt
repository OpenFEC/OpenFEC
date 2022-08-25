cmake_minimum_required(VERSION 3.0.2)

##project
project(openfec C)
include(GNUInstallDirs)

ENABLE_TESTING()

if (PROFILING STREQUAL "ON")

else(PROFILING STREQUAL "ON")

endif(PROFILING STREQUAL "ON")

if (DEBUG STREQUAL "ON")
# Debug mode
ADD_DEFINITIONS(-DOF_DEBUG)
set(CMAKE_BUILD_TYPE Debug) 
message(STATUS "Debug mode ON" )

else(DEBUG STREQUAL "ON")
# Release mode
set(CMAKE_BUILD_TYPE Release)
message(STATUS "Debug mode OFF")

endif (DEBUG STREQUAL "ON")

add_subdirectory(src)
add_subdirectory(applis/eperftool)
add_subdirectory(applis/howto_examples/simple_client_server)
add_subdirectory(tools/descr_stats_v1.2)
add_subdirectory(tests)
