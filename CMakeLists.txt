cmake_minimum_required(VERSION 3.9)

include("cmake/HunterGate.cmake")
HunterGate(
	URL "https://github.com/cpp-pm/hunter/archive/v0.23.316.tar.gz"
	SHA1 "167a33c4cc398504826a1a3014ef66f9ccc35db3"
	LOCAL
)

option(BUILD_VISUALIZER "Build the visualization libary" ON)

project(BlendingSimulator)

enable_testing()

add_subdirectory(BlendingSimulatorLib)
add_subdirectory(BlendingSimulatorFastLib)
add_subdirectory(BlendingSimulatorDetailedLib)
add_subdirectory(BlendingSimulatorLibPython)
add_subdirectory(BlendingSimulatorCli)

if (BUILD_VISUALIZER)
	add_subdirectory(BlendingVisualizer)
endif ()
