# Blending Simulator

[![Build Status](https://github.com/jcbachmann/blending-simulation/actions/workflows/build.yml/badge.svg)](https://github.com/jcbachmann/blending-simulation/actions/workflows/build.yml)
[![Test Status](https://github.com/jcbachmann/blending-simulation/actions/workflows/test.yml/badge.svg)](https://github.com/jcbachmann/blending-simulation/actions/workflows/test.yml)
[![Coverage](https://codecov.io/gh/jcbachmann/blending-simulation/branch/master/graph/badge.svg)](https://codecov.io/gh/jcbachmann/blending-simulation)

This software package contains libraries and programs for the simulation of stacking and reclaiming in bulk material blending beds.

## Dependencies

The following table lists all internal and external dependencies for the libraries and executables in this repository.

External dependencies are automatically downloaded by CMake via `FetchContent`.

| Target                                                  | Internal Dependencies                                                                                          | External Dependencies                                                                      |
|---------------------------------------------------------|----------------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------|
| `BlendingSimulatorCli`<br>*executable*                  | `BlendingSimulatorLib`<br>`BlendingSimulatorFastLib`<br>`BlendingSimulatorDetailedLib`<br>`BlendingSimulatorLatticeLib`<br>`BlendingVisualizer` | [CLI11](https://github.com/CLIUtils/CLI11) v2.6.2                                          | 
| `BlendingSimulatorLib`<br>*header-only library*         | *none*                                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFastLib`<br>*header-only library*     | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFastLib-test`<br>*executable*         | `BlendingSimulatorFastLib`                                                                                     | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorDetailedLib`<br>*header-only library* | `BlendingSimulatorLib`                                                                                         | [Bullet Physics](https://github.com/bulletphysics/bullet3) v2.87                           |
| `BlendingSimulatorDetailedLib-test`<br>*executable*     | `BlendingSimulatorDetailedLib`                                                                                 | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorLatticeLib`<br>*header-only library*      | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorLatticeLib-test`<br>*executable*      | `BlendingSimulatorLatticeLib`                                                                                  | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingVisualizer`<br>*static library*                | `BlendingSimulatorLib`                                                                                         | [OGRE](https://github.com/OGRECave/ogre) v1.11.6<br>[SDL2](https://www.libsdl.org) v2.30.9 |

## Simulators

All simulators split the stacked material into particles of `1 / particlesPerCubicMeter` m³, track where each particle comes to rest
and reclaim the stockpile with a reclaimer at `reclaimAngle` moving along x. The parameters are in
[`SimulationParameters.h`](BlendingSimulatorLib/include/BlendingSimulator/SimulationParameters.h).

| Simulator | CLI | Model | Specific parameters |
|---|---|---|---|
| `BlendingSimulatorFast` | default | Cubes in columns of a square height grid. A particle falls to a lower neighbor column (of 8, or of 4 with probability `1 - eightLikelihood`, which gives pyramids instead of cones) until no neighbor is lower; cones reach about 44°. | `eightLikelihood`, `seed` |
| `BlendingSimulatorLattice` | `--lattice` | Spheres on a hexagonal close-packed lattice (port of the hexsim proof of concept). A particle rests once all three sites below it are filled. Piles are hexagonal pyramids whose volume equals that of a cone of 60.89° (faces 62.06°, edges 58.52°); the lattice is compressed vertically so that this equivalent cone reaches `latticeAngleOfRepose`, and the particles become spheroids of the same volume. Faces then stand about 1.4° steeper and edges 2.7° flatter (at 45°). Deterministic. No circular stockpiles. | `latticeAngleOfRepose` (`--latticeangle`) |
| `BlendingSimulatorDetailed` | `--detailed` | Rigid-body cubes in Bullet physics, thrown from the stacker belt; the angle of repose follows from the friction. Seconds to hours per run. | `dropHeight`, `bulkDensityFactor`, `seed` |

Fast and lattice simulation cost about the same per particle (a chevron stockpile of 323,000 particles stacks and reclaims in about 0.2 s).
With `visualize` set, the simulators keep every particle (`inactiveOutputParticles`) for visualization and export.

The CLI reads the stacked material from stdin, one line per increment: `time x z volume parameter...` (whitespace separated), and writes
the reclaimed slices with `--reclaim <file|stdout>` and the height map with `--heights <file>`:

```bash
BlendingSimulatorCli --lattice --latticeangle 45 --length 60 --depth 20 --ppm3 8 --reclaim stdout < deposition.txt
```
