cmake_minimum_required(VERSION 3.15)

set(
	SOURCE_FILES
	src/BlendingSimulatorFormula-test.cpp
)

add_executable(BlendingSimulatorFormulaLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorFormulaLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_link_libraries(
	BlendingSimulatorFormulaLib-test
	BlendingSimulator::FormulaLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorFormulaLib-test)

# The comparison with the cone simulation, which the formula reduces
if (TARGET BlendingSimulator::ConeLib)
	target_link_libraries(BlendingSimulatorFormulaLib-test BlendingSimulator::ConeLib)
	target_compile_definitions(BlendingSimulatorFormulaLib-test PRIVATE CONE_SIMULATOR_AVAILABLE)
endif ()
