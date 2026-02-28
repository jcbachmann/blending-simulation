cmake_minimum_required(VERSION 3.15)

option(BUILD_VISUALIZER "Build the visualization libary" OFF)
option(BUILD_TESTS "Build the tests" OFF)
option(BUILD_FAST_SIMULATOR "Build fast simulator" OFF)
option(BUILD_DETAILED_SIMULATOR "Build detailed simulator" OFF)
option(BUILD_CLI "Build simulator CLI" OFF)

project(BlendingSimulator)

include(cmake/Dependencies.cmake)

if (BUILD_TESTS)
	enable_testing()
endif ()

add_subdirectory(BlendingSimulatorLib)

if (BUILD_FAST_SIMULATOR)
	add_subdirectory(BlendingSimulatorFastLib)
endif ()

if (BUILD_DETAILED_SIMULATOR)
	add_subdirectory(BlendingSimulatorDetailedLib)
endif ()

if (BUILD_CLI)
	add_subdirectory(BlendingSimulatorCli)
endif ()

if (BUILD_VISUALIZER)
	add_subdirectory(BlendingVisualizer)
endif ()
