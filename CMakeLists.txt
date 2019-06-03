cmake_minimum_required(VERSION 3.9)

include("cmake/HunterGate.cmake")
HunterGate(
	URL "https://github.com/ruslo/hunter/archive/v0.23.51.tar.gz"
	SHA1 "d6f582a4d127af6ac01d9be842636da3935043d4"
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
