# Blending Simulator

[![Build Status](https://github.com/jcbachmann/blending-simulation/actions/workflows/build.yml/badge.svg)](https://github.com/jcbachmann/blending-simulation/actions/workflows/build.yml)
[![Test Status](https://github.com/jcbachmann/blending-simulation/actions/workflows/test.yml/badge.svg)](https://github.com/jcbachmann/blending-simulation/actions/workflows/test.yml)
[![Coverage](https://img.shields.io/badge/coverage-view--artifact-blue)](https://github.com/jcbachmann/blending-simulation/actions/workflows/test.yml)

This software package contains libraries and programs for the simulation of stacking and reclaiming in bulk material blending beds.

## Dependencies

The following table lists all internal and external dependencies for the libraries and executables in this repository.

Extenral dependencies are automatically downloaded by CMake via `FetchContent`.

| Target                                                  | Internal Dependencies                                                                                          | External Dependencies                                                                      |
|---------------------------------------------------------|----------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------|
| `BlendingSimulatorCli`<br>*executable*                  | `BlendingSimulatorLib`<br>`BlendingSimulatorFastLib`<br>`BlendingSimulatorDetailedLib`<br>`BlendingVisualizer` | [CLI11](https://github.com/CLIUtils/CLI11) v2.6.2                                          | 
| `BlendingSimulatorLib`<br>*header-only library*         | *none*                                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFastLib`<br>*header-only library*     | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFastLib-test`<br>*executable*         | `BlendingSimulatorFastLib`                                                                                     | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorDetailedLib`<br>*header-only library* | `BlendingSimulatorLib`                                                                                         | [Bullet Physics](https://github.com/bulletphysics/bullet3) v2.87                           |
| `BlendingSimulatorDetailedLib-test`<br>*executable*     | `BlendingSimulatorDetailedLib`                                                                                 | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingVisualizer`<br>*static library*                | `BlendingSimulatorLib`                                                                                         | [OGRE](https://github.com/OGRECave/ogre) v1.11.6<br>[SDL2](https://www.libsdl.org) v2.30.9 |
