cmake_minimum_required(VERSION 3.15)

set(
	SOURCE_FILES
	src/BlendingSimulatorLattice-test.cpp
)

add_executable(BlendingSimulatorLatticeLib-test ${SOURCE_FILES})

set_target_properties(
	BlendingSimulatorLatticeLib-test PROPERTIES
	CXX_STANDARD_REQUIRED 17
)

target_link_libraries(
	BlendingSimulatorLatticeLib-test
	BlendingSimulator::LatticeLib
	GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET BlendingSimulatorLatticeLib-test)
