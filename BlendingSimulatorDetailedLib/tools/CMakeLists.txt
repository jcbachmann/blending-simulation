cmake_minimum_required(VERSION 3.15)

# Development and calibration tools of the detailed simulation (BUILD_TOOLS)
add_executable(DemProbe src/DemProbe.cpp)
target_link_libraries(DemProbe PRIVATE BlendingSimulator::DetailedLib)

add_executable(DemCalibration src/DemCalibration.cpp)
target_link_libraries(DemCalibration PRIVATE BlendingSimulator::DetailedLib)
