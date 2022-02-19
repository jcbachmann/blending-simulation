cmake_minimum_required(VERSION 3.9)

include("cmake/HunterGate.cmake")
HunterGate(
	URL "https://github.com/cpp-pm/hunter/archive/v0.23.316.tar.gz"
	SHA1 "167a33c4cc398504826a1a3014ef66f9ccc35db3"
	LOCAL
)

option(BUILD_VISUALIZER "Build the visualization libary" OFF)
option(BUILD_TESTS "Build the tests" OFF)
option(BUILD_FAST_SIMULATOR "Build fast simulator" OFF)
option(BUILD_DETAILED_SIMULATOR "Build detailed simulator" OFF)
option(BUILD_PYTHON_LIB "Build python lib" OFF)
option(BUILD_CLI "Build simulator CLI" OFF)

project(BlendingSimulator)

if (BUILD_TESTS)
	enable_testing()
endif ()

add_subdirectory(BlendingSimulatorLib)

if (BUILD_FAST_SIMULATOR)
	add_subdirectory(BlendingSimulatorFastLib)
endif ()

if (BUILD_DETAILED)
	add_subdirectory(BlendingSimulatorDetailedLib)
endif ()

if (BUILD_PYTHON_LIB)
	add_subdirectory(BlendingSimulatorLibPython)
endif ()

if (BUILD_CLI)
	add_subdirectory(BlendingSimulatorCli)
endif ()

if (BUILD_VISUALIZER)
	add_subdirectory(BlendingVisualizer)
endif ()
