cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(
		SOURCE_FILES
		ConfigHandler.cpp
		FlowLine.cpp
		main.cpp
		Processing.cpp
)

add_executable(FlowConverter ${SOURCE_FILES})