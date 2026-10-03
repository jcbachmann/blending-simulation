cmake_minimum_required(VERSION 3.15)

set(
	SOURCE_FILES
	src/HeightMap-test.cpp
	../src/HeightMap.cpp
)

add_executable(BlendingSimulatorCli-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorCli-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_include_directories(BlendingSimulatorCli-test PRIVATE ../src)

target_link_libraries(
	BlendingSimulatorCli-test
	BlendingSimulator::FastLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorCli-test)
