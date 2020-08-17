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
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	pthread
	CLI11::CLI11
)

if (BUILD_VISUALIZER)
	message(STATUS "Building BlendingSimulatorCli with visualization")
	target_compile_definitions(BlendingSimulatorCli PUBLIC VISUALIZER_AVAILABLE)
	target_link_libraries(BlendingSimulatorCli BlendingVisualizer)
else ()
	message(STATUS "Building BlendingSimulatorCli without visualization")
endif ()
