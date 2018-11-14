cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	main.cpp
	Execution.cpp
	Execution.h
	ExecutionParameters.h
)

hunter_add_package(Boost COMPONENTS program_options)
find_package(Boost CONFIG REQUIRED program_options)

add_executable(BlendingSimulator ${SOURCE_FILES})

set_target_properties(
	BlendingSimulator PROPERTIES
	CXX_STANDARD_REQUIRED 14
)

target_link_libraries(
	BlendingSimulator
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	pthread
	Boost::program_options
)

if (BUILD_VISUALIZER)
	message(STATUS "Building BlendingSimulator with visualization")
	target_compile_definitions(BlendingSimulator PUBLIC VISUALIZER_AVAILABLE)
	target_link_libraries(BlendingSimulator BlendingVisualizer)
else ()
	message(STATUS "Building BlendingSimulator without visualization")
endif ()
