cmake_minimum_required(VERSION 3.15)

set(
	SOURCE_FILES
	src/BlendingSimulatorFast-test.cpp
)

add_executable(BlendingSimulatorFastLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorFastLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_link_libraries(
	BlendingSimulatorFastLib-test
	BlendingSimulator::FastLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorFastLib-test)
