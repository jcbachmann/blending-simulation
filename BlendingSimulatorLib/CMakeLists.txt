cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_library(BlendingSimulatorLib INTERFACE)

target_include_directories(BlendingSimulatorLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})