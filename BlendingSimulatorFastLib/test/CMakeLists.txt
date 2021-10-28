cmake_minimum_required(VERSION 3.5)

set(
	SOURCE_FILES
	src/BlendingSimulatorFast-test.cpp
)

add_executable(BlendingSimulatorFastLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorFastLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

target_link_libraries(
	BlendingSimulatorFastLib-test
	BlendingSimulatorFastLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorFastLib-test)
