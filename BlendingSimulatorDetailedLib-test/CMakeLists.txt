cmake_minimum_required(VERSION 3.5)

set(
	SOURCE_FILES
	BlendingSimulatorDetailed-test.cpp
)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

add_executable(BlendingSimulatorDetailedLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorDetailedLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 14
)

target_link_libraries(
	BlendingSimulatorDetailedLib-test
	BlendingSimulatorDetailedLib
	GTest::main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorDetailedLib-test)
