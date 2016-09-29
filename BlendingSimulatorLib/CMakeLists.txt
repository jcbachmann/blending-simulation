cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(
		SOURCE_FILES
		Simulator.cpp
)

add_library(BlendingSimulatorLib STATIC ${SOURCE_FILES})

target_include_directories(BlendingSimulatorLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})