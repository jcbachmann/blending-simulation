cmake_minimum_required(VERSION 3.15)

set(
	SOURCE_FILES
	src/BlendingSimulatorCone-test.cpp
)

add_executable(BlendingSimulatorConeLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorConeLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_link_libraries(
	BlendingSimulatorConeLib-test
	BlendingSimulator::ConeLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorConeLib-test)
