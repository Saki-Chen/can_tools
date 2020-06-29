cmake_minimum_required(VERSION 3.0.0)
project(esd_can_tools)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(OpenCV)
find_package(Eigen3)

include_directories(
)
 

add_library(can_tools SHARED src/can_tools.cpp)
target_link_libraries(can_tools -lntcan)

add_executable(test_code src/test_code.cpp )
target_link_libraries(test_code can_tools)

add_executable(echo_can_msg src/echo_can_msg.cpp)
target_link_libraries(echo_can_msg can_tools)
