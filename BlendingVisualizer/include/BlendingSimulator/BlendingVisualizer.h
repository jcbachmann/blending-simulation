#ifndef BLENDINGVISUALIZER_H
#define BLENDINGVISUALIZER_H

#include "detail/Visualizer.h"

// Forward declarations
namespace Ogre
{
class TerrainGroup;

class TerrainGlobalOptions;
}

class HeapMesh;

struct VisualizationParticle;
struct VisualizationInstancedParticle;

template<typename Parameters>
class BlendingSimulator;

template<typename Parameters>
class BlendingVisualizer : public Visualizer
{
	public:
		BlendingVisualizer(BlendingSimulator<Parameters>* simulator, bool verbose, bool pretty);
		~BlendingVisualizer() override = default;

	protected:
		void createFrameListener() override;
		void createScene() override;
		void destroyScene() override;
		void frameRendered(const Ogre::FrameEvent& evt) override;
		bool keyPressed(const OgreBites::KeyboardEvent& evt) override;
		void checkBoxToggled(OgreBites::CheckBox* box) override;

	private:
		bool pretty;
		OgreBites::ParamsPanel* mSimulationPanel;
		OgreBites::ParamsPanel* mGraphicsPanel;
		Ogre::TerrainGroup* mTerrainGroup;
		Ogre::TerrainGlobalOptions* mTerrainGlobals;
		BlendingSimulator<Parameters>* simulator;
		std::deque<VisualizationParticle*> activeParticlePool;
		std::deque<VisualizationInstancedParticle*> inactiveParticles;
		bool showInactiveParticles;
		bool showHeapMap;
		Ogre::InstanceManager* instanceManager;
		HeapMesh* heapMesh;
		Ogre::Entity* heapEntity;

		void addTerrain(float flatSizeX, float flatSizeZ);
		void addGroundPlane();
		void addHeap(float heapWorldSizeX, float heapWorldSizeZ);
		void refreshHeightMap();
		void refreshParticles();
};

#include "detail/BlendingVisualizer.impl.h"

#endif
