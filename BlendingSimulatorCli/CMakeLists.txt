cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	src/main.cpp
	src/Execution.cpp
)

hunter_add_package(Boost COMPONENTS program_options)
find_package(Boost CONFIG REQUIRED program_options)

add_executable(BlendingSimulatorCli ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorCli PROPERTIES
	CXX_STANDARD_REQUIRED 14
)

target_link_libraries(
	BlendingSimulatorCli
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	pthread
	Boost::program_options
)

if (BUILD_VISUALIZER)
	message(STATUS "Building BlendingSimulatorCli with visualization")
	target_compile_definitions(BlendingSimulatorCli PUBLIC VISUALIZER_AVAILABLE)
	target_link_libraries(BlendingSimulatorCli BlendingVisualizer)
else ()
	message(STATUS "Building BlendingSimulatorCli without visualization")
endif ()
