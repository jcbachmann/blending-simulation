cmake_minimum_required(VERSION 3.9)

set(
	SOURCE_FILES
	src/main.cpp
	src/Execution.cpp
)

add_executable(BlendingSimulatorCli ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorCli PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

hunter_add_package(CLI11)
find_package(CLI11 CONFIG REQUIRED)

target_link_libraries(
	BlendingSimulatorCli
	PRIVATE
	BlendingSimulatorLib
	pthread
	CLI11::CLI11
)

if (BUILD_FAST_SIMULATOR)
	target_link_libraries(BlendingSimulatorCli PRIVATE BlendingSimulatorFastLib)
	target_compile_definitions(BlendingSimulatorCli PRIVATE FAST_SIMULATOR_AVAILABLE)
endif ()

if (BUILD_DETAILED_SIMULATOR)
	target_link_libraries(BlendingSimulatorCli PRIVATE BlendingSimulatorDetailedLib)
	target_compile_definitions(BlendingSimulatorCli PRIVATE DETAILED_SIMULATOR_AVAILABLE)
endif ()

if (BUILD_VISUALIZER)
	message(STATUS "Building BlendingSimulatorCli with visualization")
	target_compile_definitions(BlendingSimulatorCli PUBLIC VISUALIZER_AVAILABLE)
	target_link_libraries(BlendingSimulatorCli PRIVATE BlendingVisualizer)
else ()
	message(STATUS "Building BlendingSimulatorCli without visualization")
endif ()
