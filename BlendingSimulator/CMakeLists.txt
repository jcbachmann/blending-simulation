cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	main.cpp
	Execution.cpp
	Execution.h
	ExecutionParameters.h
)

find_package(Boost 1.61 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(BlendingSimulator ${SOURCE_FILES})

target_link_libraries(
	BlendingSimulator
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	BlendingVisualizer
	pthread
	${Boost_LIBRARIES}
)
