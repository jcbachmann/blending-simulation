cmake_minimum_required(VERSION 3.9)

project(BlendingSimulatorLibPython VERSION 1.0.0 DESCRIPTION "Blending Simulator Lib for Python")

pybind11_add_module(
	blending_simulator_lib
	src/BlendingSimulatorLibPython.cpp
)

set_target_properties(
	blending_simulator_lib PROPERTIES
	CXX_STANDARD_REQUIRED 17
	POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
	blending_simulator_lib PRIVATE
	BlendingSimulator::Lib
	Threads::Threads
	pybind11::pybind11
	pybind11::embed
	pybind11::module
)

if (BUILD_FAST_SIMULATOR)
	target_link_libraries(blending_simulator_lib PRIVATE BlendingSimulator::FastLib)
	target_compile_definitions(blending_simulator_lib PRIVATE FAST_SIMULATOR_AVAILABLE)
endif ()

if (BUILD_DETAILED_SIMULATOR)
	target_link_libraries(blending_simulator_lib PRIVATE BlendingSimulator::DetailedLib)
	target_compile_definitions(blending_simulator_lib PRIVATE DETAILED_SIMULATOR_AVAILABLE)
endif ()
