cmake_minimum_required(VERSION 3.15)

add_library(BlendingSimulatorConeLib INTERFACE)
add_library(BlendingSimulator::ConeLib ALIAS BlendingSimulatorConeLib)

target_include_directories(
	BlendingSimulatorConeLib
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(BlendingSimulatorConeLib INTERFACE cxx_std_17)

target_link_libraries(
	BlendingSimulatorConeLib
	INTERFACE BlendingSimulator::Lib
)

if (BUILD_TESTS)
	add_subdirectory(test)
endif ()
