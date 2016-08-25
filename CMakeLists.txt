cmake_minimum_required(VERSION 3.5)

project(BlendingSimulator)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(
		SOURCE_FILES
		ConfigHandler.cpp
		FlowLine.cpp
		main.cpp
		Processing.cpp
		Simulator.cpp
)
add_library(BlendingSimulator STATIC ${SOURCE_FILES})
