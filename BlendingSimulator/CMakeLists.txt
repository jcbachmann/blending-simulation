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

target_link_libraries(
	BlendingSimulator
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	BlendingVisualizer
	pthread
	Boost::program_options
)
