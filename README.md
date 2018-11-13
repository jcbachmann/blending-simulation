# Blending Simulator
This software package contains libraries and executables for the simulation of stacking and reclaiming in longitudinal bulk material blending beds.

[![Build Status](https://travis-ci.com/jcbachmann/blending-simulation.svg?branch=master)](https://travis-ci.com/jcbachmann/blending-simulation)

## Dependencies
Here are all dependencies for the libraries and executables in this repository.

External dependencies are marked with System (indicating that installation is required at system level) or Hunter (indicating automatic installation using [Hunter](https://docs.hunter.sh/en/latest/)).

### By module
| Target | Internal Dependencies | External Dependencies |
| ------ | --------------------- | --------------------- |
| BlendingSimulator (executable) | <ul><li>BlendingSimulatorLib</li><li>BlendingSimulatorFastLib</li><li>BlendingSimulatorDetailedLib</li><li>BlendingVisualizer</li></ul> | <ul><li>Boost (Hunter)</li></ul> | 
| BlendingSimulatorLib (header-only library) | none | none |
| BlendingSimulatorFastLib (header-only library) | <ul><li>BlendingSimulatorLib</li></ul> | none |
| BlendingSimulatorFastLib-test (executable) | <ul><li>BlendingSimulatorFastLib</li></ul> | <ul><li>Google Test (Hunter)</li></ul> |
| BlendingSimulatorDetailedLib (header-only library) | <ul><li>BlendingSimulatorLib</li></ul> | <ul><li>Bullet Physics (Hunter)</li></ul> |
| BlendingSimulatorDetailedLib-test (executable) | <ul><li>BlendingSimulatorDetailedLib</li></ul> | <ul><li>Google Test (Hunter)</li></ul> |
| BlendingVisualizer (static library) | <ul><li>BlendingSimulatorLib</li></ul> | <ul><li>OGRE (System)</li><li>SDL (Hunter)</li></ul> |
| BlendingSimulatorLibPython (shared library) | <ul><li>BlendingSimulatorLib</li><li>BlendingSimulatorFastLib</li><li>BlendingSimulatorDetailedLib</li></ul> | <ul><li>pybind11 (Hunter)</li></ul> |
