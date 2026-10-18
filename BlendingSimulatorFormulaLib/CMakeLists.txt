cmake_minimum_required(VERSION 3.15)

add_library(BlendingSimulatorFormulaLib INTERFACE)
add_library(BlendingSimulator::FormulaLib ALIAS BlendingSimulatorFormulaLib)

target_include_directories(
	BlendingSimulatorFormulaLib
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(BlendingSimulatorFormulaLib INTERFACE cxx_std_17)

target_link_libraries(
	BlendingSimulatorFormulaLib
	INTERFACE BlendingSimulator::Lib
)

if (BUILD_TESTS)
	add_subdirectory(test)
endif ()
