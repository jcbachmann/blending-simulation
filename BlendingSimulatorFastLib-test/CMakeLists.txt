cmake_minimum_required(VERSION 3.5)

set(
	SOURCE_FILES
	BlendingSimulatorFast-test.cpp
)

hunter_add_package(GTest)
include(GoogleTest)

add_executable(BlendingSimulatorFastLib-test ${SOURCE_FILES})

target_link_libraries(
	BlendingSimulatorFastLib-test
	BlendingSimulatorFastLib
	gtest
	gtest_main
)

gtest_add_tests(TARGET BlendingSimulatorFastLib-test)
