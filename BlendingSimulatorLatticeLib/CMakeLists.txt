cmake_minimum_required(VERSION 3.15)

add_library(BlendingSimulatorLatticeLib INTERFACE)
add_library(BlendingSimulator::LatticeLib ALIAS BlendingSimulatorLatticeLib)

target_include_directories(
	BlendingSimulatorLatticeLib
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(BlendingSimulatorLatticeLib INTERFACE cxx_std_17)

target_link_libraries(
	BlendingSimulatorLatticeLib
	INTERFACE BlendingSimulator::Lib
)

if (BUILD_TESTS)
	add_subdirectory(test)
endif ()
