cmake_minimum_required(VERSION 3.9)

project(BlendingSimulatorLibPython VERSION 1.0.0 DESCRIPTION "Blending Simulator Lib for Python")

FIND_PACKAGE(PythonInterp)

FIND_PACKAGE(Boost COMPONENTS python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR} REQUIRED)
FIND_PACKAGE(PythonInterp 3 REQUIRED)
FIND_PACKAGE(PythonLibs 3 REQUIRED)

message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}")

include_directories(${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

python_add_module(blending_simulator_lib BlendingSimulatorLibPython.cpp)

target_link_libraries(
	blending_simulator_lib
	BlendingSimulatorLib
	BlendingSimulatorFastLib
	BlendingSimulatorDetailedLib
	BlendingVisualizer
	pthread
)
