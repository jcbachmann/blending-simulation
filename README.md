# blending-simulator
This software simulates the stacking and reclaiming process in longitudinal bulk material blending beds.

## Build
Use QtCreator to open Qbs project file and compile with c++11 (e.g. g++ 6.1.1).

## Usage
To execute the binary pass one of the output files generated by [blending-control](https://github.com/jcbachmann/blending-control) (stack or reclaim) as a single parameter.

In case of the stack file the stockpile is simulated and a reclaimer output file is generated. In case of the reclaim file the output format is adapted to match the reclaim output format of the simulator for easy comparison.

A configuration file `processing.config` is generated with default parameters on first execution and placed in the working directory. This file can be modified to match the real world model.

## Input-Files
* "\<stockpilename\>.stockpile \<timestamp\> stack.csv" -> Input for blending simulation
* "\<stockpilename\>.stockpile \<timestamp\> reclaim.csv" -> Format is adapted

## Output-Files
* "\<stockpilename\>.stockpile \<timestamp\> stack.csv.sim.pile.csv": output of simulation
* "\<stockpilename\>.stockpile \<timestamp\> stack.csv.sim.pile.plotdata": output of simulation
* "\<stockpilename\>.stockpile \<timestamp\> stack.csv.sim.slices.csv": output of simulation
* "\<stockpilename\>.stockpile \<timestamp\> reclaim.csv.slices.csv": format adapted
