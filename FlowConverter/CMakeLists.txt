cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(
		SOURCE_FILES
		FlowLine.cpp
		main.cpp
		Processing.cpp
)

find_package(Boost 1.61 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(FlowConverter ${SOURCE_FILES})

target_link_libraries(FlowConverter ${Boost_LIBRARIES})