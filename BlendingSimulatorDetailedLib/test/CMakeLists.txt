cmake_minimum_required(VERSION 3.15)

set(
	SOURCE_FILES
	src/BlendingSimulatorDetailed-test.cpp
)

add_executable(BlendingSimulatorDetailedLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorDetailedLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_link_libraries(
	BlendingSimulatorDetailedLib-test
	BlendingSimulator::DetailedLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorDetailedLib-test)
