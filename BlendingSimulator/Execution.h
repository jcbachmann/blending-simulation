#ifndef BLENDINGSIMULATOR_EXECUTION_H
#define BLENDINGSIMULATOR_EXECUTION_H

#include "ExecutionParameters.h"

void executeFastSimulationCounted(ExecutionParameters parameters);
void executeFastSimulationAveraged(ExecutionParameters parameters);
void executeDetailedSimulationCounted(ExecutionParameters parameters);
void executeDetailedSimulationAveraged(ExecutionParameters parameters);

#endif
