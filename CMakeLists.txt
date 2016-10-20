cmake_minimum_required(VERSION 3.5)

project(BlendingSimulator)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}-build)

add_subdirectory(BlendingSimulatorLib)
add_subdirectory(BlendingSimulator)
add_subdirectory(FlowConverter)