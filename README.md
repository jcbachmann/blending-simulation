# Blending Simulator
This software package contains libraries and programs for the simulation of stacking and reclaiming in bulk material blending beds.

[![Build Status](https://travis-ci.com/jcbachmann/blending-simulation.svg?branch=master)](https://travis-ci.com/jcbachmann/blending-simulation)

## Dependencies
The following table lists all dependencies for the libraries and executables in this repository.

External dependencies are marked with System (indicating that installation is required at system level) or Hunter (indicating automatic installation using [Hunter](https://docs.hunter.sh/en/latest/)).

| Target | Internal Dependencies | External Dependencies |
| ------ | --------------------- | --------------------- |
| `BlendingSimulatorCli`<br>*executable* | `BlendingSimulatorLib`<br>`BlendingSimulatorFastLib`<br>`BlendingSimulatorDetailedLib`<br>`BlendingVisualizer` | [CLI11](https://github.com/CLIUtils/CLI11) ([Hunter](https://docs.hunter.sh/en/latest/packages/pkg/CLI11.html)) | 
| `BlendingSimulatorLib`<br>*header-only library* | *none* | *none* |
| `BlendingSimulatorFastLib`<br>*header-only library* | `BlendingSimulatorLib` | *none* |
| `BlendingSimulatorFastLib-test`<br>*executable* | `BlendingSimulatorFastLib` | [Google Test](https://github.com/google/googletest) ([Hunter](https://docs.hunter.sh/en/latest/packages/pkg/GTest.html)) |
| `BlendingSimulatorDetailedLib`<br>*header-only library* | `BlendingSimulatorLib` | [Bullet Physics](https://github.com/bulletphysics/bullet3) ([Hunter](https://docs.hunter.sh/en/latest/packages/pkg/bullet.html)) |
| `BlendingSimulatorDetailedLib-test`<br>*executable* | `BlendingSimulatorDetailedLib` | [Google Test](https://github.com/google/googletest) ([Hunter](https://docs.hunter.sh/en/latest/packages/pkg/GTest.html)) |
| `BlendingVisualizer`<br>*static library* | `BlendingSimulatorLib` | [OGRE](https://github.com/OGRECave/ogre) (System)<br>[SDL2](https://www.libsdl.org) ([Hunter](https://docs.hunter.sh/en/latest/packages/pkg/SDL2.html)) |
| `BlendingSimulatorLibPython`<br>*shared library* | `BlendingSimulatorLib`<br>`BlendingSimulatorFastLib`<br>`BlendingSimulatorDetailedLib` | [pybind11](https://github.com/pybind/pybind11) ([Hunter](https://docs.hunter.sh/en/latest/packages/pkg/pybind11.html)) |

When building `BlendingSimulator` the build of `BlendingVisualizer` can be turned off setting the CMake option `BUILD_VISUALIZER` to `OFF`. This allows compilation of all targets when the OGRE dependency can not be satisfied. 
