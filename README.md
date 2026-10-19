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
| `BlendingSimulatorCli`<br>*executable*                  | `BlendingSimulatorLib`<br>`BlendingSimulatorFastLib`<br>`BlendingSimulatorDetailedLib`<br>`BlendingSimulatorLatticeLib`<br>`BlendingSimulatorConeLib`<br>`BlendingSimulatorFormulaLib`<br>`BlendingVisualizer` | [CLI11](https://github.com/CLIUtils/CLI11) v2.6.2                                          | 
| `BlendingSimulatorLib`<br>*header-only library*         | *none*                                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFastLib`<br>*header-only library*     | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFastLib-test`<br>*executable*         | `BlendingSimulatorFastLib`                                                                                     | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorDetailedLib`<br>*static library*      | `BlendingSimulatorLib`                                                                                         | OpenMP (optional, `-DUSE_OPENMP=OFF` to disable)                                           |
| `BlendingSimulatorDetailedLib-test`<br>*executable*     | `BlendingSimulatorDetailedLib`                                                                                 | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorLatticeLib`<br>*header-only library*      | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorLatticeLib-test`<br>*executable*      | `BlendingSimulatorLatticeLib`                                                                                  | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorConeLib`<br>*header-only library*     | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorConeLib-test`<br>*executable*         | `BlendingSimulatorConeLib`                                                                                     | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingSimulatorFormulaLib`<br>*header-only library*  | `BlendingSimulatorLib`                                                                                         | *none*                                                                                     |
| `BlendingSimulatorFormulaLib-test`<br>*executable*      | `BlendingSimulatorFormulaLib`<br>`BlendingSimulatorConeLib` (optional)                                         | [Google Test](https://github.com/google/googletest) v1.17.0                                |
| `BlendingVisualizer`<br>*static library*                | `BlendingSimulatorLib`                                                                                         | [OGRE](https://github.com/OGRECave/ogre) v1.11.6<br>[SDL2](https://www.libsdl.org) v2.30.9 |

## Simulators

The particle simulators split the stacked material into particles of `1 / particlesPerCubicMeter` m³, track where each particle comes to
rest and reclaim the stockpile with a reclaimer at `reclaimAngle` moving along x; the cone and formula simulators work on the surface of the pile
instead (below). The parameters are in
[`SimulationParameters.h`](BlendingSimulatorLib/include/BlendingSimulator/SimulationParameters.h).
Cells and reclaim slices are one particle size wide, cell or slice i covering the positions from i to i + 1 particle sizes. A particle is
reclaimed where the reclaimer face meets its center, at x - y / tan(reclaimAngle), and its volume is spread over the slices the face cuts
it in.

| Simulator | CLI | Model | Specific parameters |
|---|---|---|---|
| `BlendingSimulatorFast` | default | Cubes in columns of a square height grid. A particle falls to a lower neighbor column (of 8, or of 4 with probability `1 - eightLikelihood`, which gives pyramids instead of cones) until no neighbor is lower. By default (since 2026.4) `eightLikelihood` is the value fitted to the detailed simulation for the particle size (`fittedEightLikelihood`: 0.60, 0.60, 0.70, 0.75, 0.80, 0.85 at 1, 4, 16, 64, 256, 1,024 particles per m³), with which the fast simulation agrees with the detailed one about equally well from 16 particles per m³ up; releases up to 2026.3 used 0.87 everywhere. A constant likelihood is tried at every step, and particles take more steps on finer grids and larger piles, so the piles steepen with `particlesPerCubicMeter` (a 500 m³ pile: about 42° at 16, 47° at 4,096 particles per m³); `eightHeightScale` 6.5 instead scales the likelihood with the height of the cell a particle is dropped on, `max(0, 1 - 6.5 / h)` for h particles, which keeps round 45° piles from 16 particles per m³ up at every resolution and pile size. | `eightLikelihood`, `eightHeightScale` (0: constant likelihood), `seed` |
| `BlendingSimulatorLattice` | `--lattice` | Spheres on a hexagonal close-packed lattice (port of the hexsim proof of concept). A particle rests once all three sites below it are filled. Piles are hexagonal pyramids whose volume equals that of a cone of 60.89° (faces 62.06°, edges 58.52°); the lattice is compressed vertically so that this equivalent cone reaches 45°, and the particles become spheroids of the same volume. Faces then stand at 46.4° and edges at 42.3°. Deterministic. No circular stockpiles. | *none* |
| `BlendingSimulatorCone` | `--cone` | No particles: every stacked portion (every `stack` call, whatever its volume) forms a cone at the angle of repose over the existing pile, the new surface is the higher of the old surface and the cone, and the apex height is solved so that the shell between them holds exactly the portion's volume, the surface a sandpile at its angle of repose reaches when material is poured at a point. Each grid column's part of the shell is reclaimed like a particle of its size. Height grid of about one particle size, `1 / particlesPerCubicMeter^(1/3)`, adjusted to tile the bed exactly; round piles at exactly the angle of repose (45°, a constructor argument). Deterministic, conserves the volume exactly, changes continuously with the drop positions. A moving stacker needs one `stack` call per grid cell it crosses (`ContinuousDrops.h`). | angle of repose (constructor) |
| `BlendingSimulatorFormula` | `--formula` | The cone simulation reduced exactly to the stacker line: when all drops lie on one line along the bed, every cross-section of the pile is the upper envelope of the hyperbolic sections of the drops' cones, two of which cross at most once, so each drop's apex, shell and reclaimed material follow from closed-form integrals and a one-dimensional root, without a grid across the bed or in height. Cells along the bed of about one particle size like the cone simulation; each shell is reclaimed in one or two height bands per envelope piece, so its slice volumes differ from the cone simulation's at the same cell size by about 1 % (rms). Deterministic, conserves the volume exactly, changes continuously with the drop positions. All drops on one line along the bed (lateral stacker motion is rejected), no circular stockpiles. | angle of repose (constructor) |
| `BlendingSimulatorDetailed` | `--detailed` | Discrete element method: soft spheres with Hertz-Mindlin contacts, Coulomb friction and rolling resistance, released from a luffing stacker discharge, colliding, sliding and rolling until they come to rest; the angle of repose (45° by default) follows from the rolling resistance. Seconds to minutes per run. See [its README](BlendingSimulatorDetailedLib/README.md). | `detailed` (`DetailedParameters`), `seed` |

Fast and lattice simulation cost about the same per particle (a chevron stockpile of 323,000 particles stacks and reclaims in about 0.2 s).
The cone simulation costs per portion and per cell it covers: 2,500 m³ in 50 portions on a 59 m by 20 m bed take about 0.5 ms with 1 m
cells, and with continuous flow (one portion per grid cell the stacker crosses, about 400 to 1,500 portions) 1 to 4 ms. The formula
simulation costs about as much (about 1.8 ms with 1 m cells and continuous flow, against 1.3 ms for the cone simulation on the same inputs).

`ContinuousDrops.h` turns a material flowing while the stacker moves along a piecewise linear path into such drops: the path is cut where
it crosses a cell boundary of the simulator's grid or a portion starts or ends, every piece gets the volume that flows during it, and the
pieces of one cell and portion become one drop. Each cell then gets the volume of the time the stacker spends over it (the first and last
cell of a pass only their overlap), which is all a simulator with that grid can resolve; one drop per particle gives the same result.
The drops follow the path continuously except where a turnaround crosses a cell boundary: the two sides of a turnaround inside one cell
are one drop, and become two once the tip reaches the next cell, a small jump of the results (0.1 % in F1 for a chevron with 1 m cells).
With `visualize` set, the simulators keep every particle (`inactiveOutputParticles`) for visualization and export.

The CLI reads the stacked material from stdin, one line per increment: `time x z volume parameter...` (whitespace separated); every line
is one drop at one position, so a continuous flow along a stacker path has to be cut into drops by the caller (for example with the
`continuous_drops` of the Python bindings, one drop per crossed cell), and it writes
the reclaimed material with `--reclaim <file|stdout>`, every particle size or every `--reclaimincrement` meters, and the height map with
`--heights <file>`:

```bash
BlendingSimulatorCli --lattice --length 60 --depth 20 --ppm3 8 --reclaim stdout < deposition.txt
```

## Visualizer

`BlendingVisualizer` shows the stockpile in 3D while it is stacked (OGRE and SDL2; built with `-DBUILD_VISUALIZER=ON`, started with
`--visualize`, nicer rendering with `--pretty`). It loads its runtime files from the working directory, and they are not part of this
repository:

- `plugins.cfg` (`plugins_d.cfg` for debug builds): the OGRE plugin folder and the plugins, at least the OpenGL render system
  (`RenderSystem_GL`), the scene managers and `Plugin_ParticleFX`.
- `resources.cfg` (`resources_d.cfg`): the resource locations, a `media` folder and the `SdkTrays.zip` of the OGRE samples.
- In the media: the mesh `Particle.mesh` with the materials `Particle_red`, `Particle_blue` and `Particle_yellow`, the textures
  `Heap.dds`, `Ground_diffusespecular.dds` and `Ground_normalheight.dds`, and the instancing and shadow shaders of the OGRE samples.

## Releasing

How versions are numbered and releases are tagged, with a complete example, is described in [RELEASING.md](RELEASING.md).
