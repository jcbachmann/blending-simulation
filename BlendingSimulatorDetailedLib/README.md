# Detailed simulation: discrete element method

`BlendingSimulatorDetailed` simulates stacking with the discrete element method (DEM): every particle is a sphere that leaves the stacker,
falls, collides, slides and rolls on the pile and comes to rest; reclaiming then cuts the settled pile at the reclaim angle. It replaces the
rigid-body simulation with Bullet (until 2026.2), whose piles depended on timed freezing of particles, a fixed throw from a fixed point
and a heap map of particle centers. The engine (`dem::World`, `dem::ContactModel`) is part of this library and has no dependencies; with
OpenMP it uses several threads, and its results do not depend on their number.

## Model

**Particles.** Each particle stands for `1 / particlesPerCubicMeter` m³ of bulk material (a coarse-grained parcel, as is usual for DEM at
the scale of stockpiles). Radii are uniform within `sizeSpread` (10 %) around a mean chosen so that the solid volume of the particles is
`packingFraction` (0.6) times the stacked bulk volume: the settled pile then takes up the stacked volume (measured within 1 to 2 % from
0.5 m particles on, see below).

**Contacts** (`ContactModel`), the standard soft-sphere model of DEM codes such as EDEM, LIGGGHTS and MercuryDPM:

* normal: Hertz, with viscous damping proportional to `sqrt(stiffness * mass)` (Tsuji et al. 1992), so the coefficient of restitution does
  not depend on the impact speed. No attraction. The damping is solved from the normalized impact equation so that a head-on impact has
  exactly the given restitution; the usual closed form, derived for linear springs, gives 0.25 instead of 0.1 once attraction is excluded.
* tangential: Mindlin's no-slip stiffness with damping, an elastic shear displacement over the lifetime of the contact, and Coulomb's
  limit. A static pile needs this elastic part; purely viscous friction cannot hold a slope.
* rolling and twisting: the elastic-plastic spring-dashpot (EPSD) model of Ai et al. (2011, Powder Technology 206), which lets spheres
  behave like irregular particles. The torque grows elastically with the relative rotation, is limited to `rollingFriction` times the
  effective radius times the normal force, and is damped below that limit. It acts on twisting about the contact normal too, which nothing
  else would resist (spheres resting on the ground otherwise spin forever).
* ground: the plane y = 0 with the same model (the particle's radius and mass as effective values).

Both particles of a contact compute exactly opposite forces (the computation is antisymmetric to the last bit), so momentum is conserved.

**Stacker.** The material of each `stack()` call is released at `flowRate` (bulk m³/s) from a discharge `dischargeHeight` above the pile
surface at the stacker position (the boom luffs with the pile). It leaves the belt with `beltSpeed` along +z, over `beltWidth` in x, and
enters the simulation 0.25 m below the head pulley, moving down at the speed of that drop: each particle is placed at a free point of a
staggered lattice in that horizontal plane, spaced so that the largest particles do not touch, with twice as many points as the flow needs
in the time a released particle takes to leave its point. The stream thus delivers the flow exactly at any particle size and is about the
same at all sizes (about 1 m along z, over the belt width in x), and it is aimed so that its center lands at the stacker position. A
horizontal speed carries material in the direction of the throw: 1 m/s moves a small pile by 0.4 m in z (which does not change F1 or F2,
both depend on x only).

**Numerics.** Semi-implicit Euler with a fixed time step of `timeStepFactor` (0.15) times the Rayleigh time of the smallest particle.
Pairs closer than their radii plus a skin are kept in a list; a particle's pairs are searched again once it moved by a quarter of the skin
or changed its state, and contact histories carry over. Particles slower than `restVelocity` for `restTime` fall asleep: they still support
and touch their neighbors but are not moved, until a particle faster than twice `restVelocity` touches them. Sleeping particles buried
three diameters below the surface around them are fixed for good. The computing time thus goes to the material in motion: a few hundred
to a few thousand particles near the stream, out of tens of thousands.

**Reclaiming** follows the other simulators: each particle is reclaimed where the face at `reclaimAngle` meets its center, spread over the
slices the face cuts it in (a box of its diameter), with its exact share of the stacked volume.

## Parameters

`SimulationParameters::detailed` (`DetailedParameters`); CLI options in parentheses; Python: `detailed_parameters={...}` with the
snake_case names.

| Parameter | Default | Meaning |
|---|---|---|
| `flowRate` (`--flow`) | 1 m³/s | Stacker flow; sets the simulated time (2,500 m³ take 2,500 s) |
| `dischargeHeight` (`--dischargeheight`) | 1.5 m | Height of the discharge above the pile |
| `beltSpeed` (`--beltspeed`) | 1 m/s | Horizontal speed of the discharged material along +z |
| `beltWidth` (`--beltwidth`) | 1.5 m | Width of the stream in x |
| `packingFraction` (`--packing`) | 0.6 | Solid fraction of the settled pile, sizes the particles |
| `sizeSpread` (`--sizespread`) | 0.1 | Relative spread of the radii |
| `density` (`--density`) | 2,600 kg/m³ | Density of the particle solid |
| `youngsModulus` (`--youngsmodulus`) | 2e8 Pa | Reduced stiffness: overlaps of 1 to 2 % at the bottom of a 10 m pile |
| `poissonRatio` (`--poisson`) | 0.25 | |
| `restitution` (`--restitution`) | 0.3 | |
| `friction` (`--friction`) | 0.6 | Sliding friction, particles and ground |
| `rollingFriction` (`--rollingfriction`) | 0.4 | Rolling resistance, sets the angle of repose (45°) |
| `rollingDamping` (`--rollingdamping`) | 0.3 | Damping ratio of the rolling resistance |
| `timeStepFactor` (`--timestepfactor`) | 0.15 | Time step per Rayleigh time |
| `restVelocity`, `restTime` (`--restvelocity`, `--resttime`) | 0.05 m/s, 0.5 s | Sleeping |
| `skinFactor` (`--skinfactor`) | 0.4 | Skin of the pair list per largest radius (0.2 to 0.8 within 3 % in time) |
| `threads` (`--threads`) | 1 | Threads, 0 for all cores |

`bulkDensityFactor` and `dropHeight` of the rigid-body simulation are no longer used.

## Calibration and checks

Measured with `tools/DemCalibration` (built with `-DBUILD_TOOLS=ON`), `tools/scripts/summarize.py` and the unit tests; seed 1, on an
i9-9900K with 4 threads while other jobs used the remaining cores, so times are upper bounds.

**Contacts** (unit tests): Hertz overlap of a resting sphere within 1 %; restitution 0.1 / 0.3 / 0.7 within 0.01 at half the default
time step (at the default step within about 10 % for impacts of 6 m/s); momentum conserved to 1e-12; identical results with 1 and 4
threads; a rolling sphere stops; piles are centered at the stacker (a vertical discharge; 8 seeds: 10.008, 10.011).

**Angle of repose** of a 500 m³ cone with 0.5 m particles (4,000), measured like the simulator comparison (radius of the area above a
height against the height, 20 to 80 % of the peak; seed 1, with the earlier discharge, which at this size differs within the noise):

| `rollingFriction` | 0.1 | 0.2 | 0.3 | 0.5 |
|---|---|---|---|---|
| angle | 36.2° | 38.6° | 42.5° | 46.8° |
| packing | 0.628 | 0.618 | 0.608 | 0.598 |

Sliding friction matters little (0.4 / 0.6 / 0.8: 41.8° / 42.5° / 42.6° at `rollingFriction` 0.3), as in calibrated DEM studies of
coarse materials. The default 0.4 gives 45.7° ± 0.9° at 0.5 m particles; the angle depends on the particle size, see below.

**Numerics** (the 2,500 m³ bed of bmh_ml, 59 m x 20 m, 19 Chevron passes with a continuously moving stacker, 1 m particles; seed to seed F1
varies by about 4 % and F2 by 3 %, at 0.5 m particles by 3 % and 0.5 %, see below): `timeStepFactor` 0.1, 0.15 and 0.2 agree, 0.25 and 0.3
are unstable (the Hertz contacts stiffen under the weight of the pile, and a sphere with a dozen contacts is stiffer in rotation than in
translation; the margin does not depend on the particle size). A Young's modulus of 4e8 gives the same results as 2e8, while 1e8 and 5e7 are
unstable at the same factor (softer contacts overlap more and stiffen relatively more). A flow of 2 m³/s and a `restVelocity` of 0.1 m/s
stay within the noise and save 35 % and 15 % of the time.

**Particle size** (default parameters; mean ± standard deviation over seeds, runs of 2026-10-17; Chevron: the bed above, F1 of a
linear input and F2 against bmh's ideal stockpile; for comparison the fast simulator on the same input, 8 seeds: L1 0.0536 / 10.62, L3
0.0508 / 10.61, L5 0.0494 / 11.25, L6 0.0488 / 11.55):

| Particle size | 1 m | 0.5 m | 0.33 m | 0.25 m |
|---|---|---|---|---|
| Chevron F1 | 0.0519 ± 0.0024 (3 seeds) | 0.0491 ± 0.0014 (4) | 0.0499 (0.0498, 0.0499) | 0.0499 (1 seed, *) |
| Chevron F2 | 10.87 ± 0.31 | 11.51 ± 0.06 | 11.54 (11.61, 11.46) | 11.54 (*) |
| Chevron bulk volume (2,500 m³ stacked), packing | 2,418 m³, 0.620 | 2,530 m³, 0.592 | 2,586 m³, 0.580 | 2,607 m³, 0.576 (*) |
| 500 m³ cone: angle of repose | 41.6° (43.2°, 39.9°) | 45.7° ± 0.9° (3) | 46.1° (*, 2 seeds) | 46.5° |
| 500 m³ cone: bulk volume, packing | 473 m³, 0.638 | 498 m³, 0.602 | 510 m³, 0.589 (*) | 519 m³, 0.578 |
| Time (4 runs side by side on 2 threads each, machine shared with other jobs) | Chevron 50 to 67 s (1 thread) | Chevron 8 to 10 min | Chevron 37 min | Cone 35 min |

(*) With the earlier discharge, which could not keep up with the flow at these sizes (stacking took 20 to 70 % longer); the 0.25 m
cone with the earlier discharge gave 47.1°, the current one 46.5°.

From 0.5 m particles on, F1 and F2 agree with the finest levels of the fast simulator within the noise, and the angle of repose within a
degree. Coarse particles pack more densely and form flatter, noisier slopes: a 500 m³ cone is only 8 diameters of 1 m particles high, and
F2 at 1 m is 5 % low. The packing keeps falling slightly with the particle size (0.60 at 0.5 m, 0.58 from 0.33 m on), so the bulk volume
of fine piles exceeds the stacked volume by up to 4 %; set `packingFraction` to 0.58 for particles of 0.33 m and below, and calibrate
`rollingFriction` for the particle size used when the angle of repose matters to a degree.

**Time per run** (Ryzen 7 5825U, 8 cores, idle; measured with the earlier discharge, the current one runs as fast): the 500 m³ cone
with 0.5 m particles takes 106 s on one thread and 36 s on 8 (16 threads: 55 s, so use one thread per physical core); the bed above 27 s on
8 threads with 1 m particles (43 s on one) and 160 s with 0.5 m particles. The time grows a little faster than the particle count (from 0.5
m to 0.33 m particles 3.4 times as many, 4.2 times the time): finer particles need proportionally shorter time steps, and the number of
particles in motion grows with the pile surface. On one thread 83 % of the time goes to the contacts, 8 % to checking the pair list and 5 %
to indexing it; on 8 threads the contacts take 18 s and the serial pair-list bookkeeping 14 s, which limits the speedup of a single run to
about 3 (more runs side by side scale fully). `World::getProfile()` returns these times. The rigid-body simulation needed 4 minutes for a 1
m chevron of 5,000 m³.

## Tools

* `tools/DemCalibration cone <m³> <ppm3> [key=value ...]` and `... chevron <passes> <ppm3> [key=value ...]`: one JSON line with time,
  particle counts, angle of repose, bulk volume and packing, reclaimed volume, F1 of a linear input and the slice volumes;
  `tools/scripts/summarize.py` adds F2 and prints a table.
* `tools/DemProbe restitution|settling|centering|chevron`: small experiments used during development.
