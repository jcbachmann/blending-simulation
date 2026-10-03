#ifndef BLENDINGSIMULATOR_HEIGHTMAP_H
#define BLENDINGSIMULATOR_HEIGHTMAP_H

#include <ostream>

#include "BlendingSimulator/BlendingSimulator.h"
#include "BlendingSimulator/ParticleParameters.h"

// Write the heap map as tab separated values, one line per z row with one height per x position
void writeHeightMap(std::ostream& out, blendingsimulator::BlendingSimulator<blendingsimulator::AveragedParameters>& simulator);

#endif
