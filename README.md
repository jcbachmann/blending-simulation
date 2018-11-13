# Blending Simulator
This software package contains libraries and executables for the simulation of stacking and reclaiming in longitudinal bulk material blending beds.

[![Build Status](https://travis-ci.com/jcbachmann/blending-simulation.svg?branch=master)](https://travis-ci.com/jcbachmann/blending-simulation)

## Dependencies
Here are all dependencies for the libraries and executables in this repository.
External dependencies are marked with System (indicating that installation is required at system level) or Hunter (indicating automatic installation using [Hunter](https://docs.hunter.sh/en/latest/)).

### By module
#### BlendingSimulator (executable)
Internal:
* BlendingSimulatorLib
* BlendingSimulatorFastLib
* BlendingSimulatorDetailedLib
* BlendingVisualizer

External:
* Boost (Hunter)

#### BlendingSimulatorLib (header-only library)
Internal: none
External: none

#### BlendingSimulatorFastLib (header-only library)
Internal:
* BlendingSimulatorLib

External: none

#### BlendingSimulatorFastLib-test (executable)
Internal:
* BlendingSimulatorFastLib

External:
* Google Test (Hunter)

#### BlendingSimulatorDetailedLib (header-only library)
Internal:
* BlendingSimulatorLib

External:
* Bullet Physics (Hunter)

#### BlendingSimulatorDetailedLib-test (executable)
Internal:
* BlendingSimulatorDetailedLib

External:
* Google Test (Hunter)

#### BlendingVisualizer (static library)
Internal:
* BlendingSimulatorLib

External:
* OGRE (System)
* SDL (Hunter)

#### BlendingSimulatorLibPython (shared library)
Internal:
* BlendingSimulatorLib
* BlendingSimulatorFastLib
* BlendingSimulatorDetailedLib

External:
* pybind11 (Hunter)

### All external dependencies
* Boost (Hunter)
* Bullet Physics (Hunter)
* Google Test (Hunter)
* OGRE (System)
* pybind11 (Hunter)
* SDL (Hunter)
