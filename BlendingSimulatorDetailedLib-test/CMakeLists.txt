cmake_minimum_required(VERSION 3.5)

set(
	SOURCE_FILES
	BlendingSimulatorDetailed-test.cpp
)

hunter_add_package(GTest)
include(GoogleTest)

add_executable(BlendingSimulatorDetailedLib-test ${SOURCE_FILES})

target_link_libraries(
	BlendingSimulatorDetailedLib-test
	BlendingSimulatorDetailedLib
	gtest
	gtest_main
)

gtest_add_tests(TARGET BlendingSimulatorDetailedLib-test)
