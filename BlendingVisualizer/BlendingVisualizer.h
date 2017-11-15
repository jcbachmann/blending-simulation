#ifndef BLENDINGVISUALIZER_H
#define BLENDINGVISUALIZER_H

#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>

#include "Visualizer.h"
#include "HeapMesh.h"

template<typename Parameters>
class BlendingSimulator;

struct VisualizationInstancedParticle
{
	VisualizationInstancedParticle()
		: entity(nullptr)
	{
	}

	Ogre::InstancedEntity* entity;
};

struct VisualizationParticle
{
	VisualizationParticle()
		: attached(false)
		, entity(nullptr)
		, node(nullptr)
	{
	}

	bool attached;
	Ogre::Entity* entity;
	Ogre::SceneNode* node;
};

struct VisualizationCube
{
	VisualizationCube()
		: attached(false)
		, node(nullptr)
	{
	}

	bool attached;
	Ogre::SceneNode* node;
	Ogre::MaterialPtr material;
};

template<typename Parameters>
class BlendingVisualizer : public Visualizer
{
	public:
		BlendingVisualizer(BlendingSimulator<Parameters>* simulator, bool verbose, bool pretty);
		virtual ~BlendingVisualizer();

	protected:
		virtual void createFrameListener() override;
		virtual void createScene() override;
		virtual void destroyScene() override;
		virtual void frameRendered(const Ogre::FrameEvent& evt) override;
		virtual bool keyPressed(const OgreBites::KeyboardEvent& evt) override;
		virtual void checkBoxToggled(OgreBites::CheckBox* box) override;

	private:
		bool pretty;
		OgreBites::ParamsPanel* mSimulationPanel;
		OgreBites::ParamsPanel* mGraphicsPanel;
		Ogre::TerrainGroup* mTerrainGroup;
		Ogre::TerrainGlobalOptions* mTerrainGlobals;
		BlendingSimulator<Parameters>* simulator;
		std::deque<VisualizationParticle*> activeParticlePool;
		std::deque<VisualizationInstancedParticle*> inactiveParticles;
		std::map<std::tuple<int, int, int>, VisualizationCube*> visualizationCubes;
		bool showInactiveParticles;
		bool showHeapMap;
		Ogre::InstanceManager* instanceManager = nullptr;
		HeapMesh* heapMesh;
		Ogre::Entity* heapEntity;

		void addTerrain(float flatSizeX, float flatSizeZ);
		void addGroundPlane();
		void addHeap(float heapWorldSizeX, float heapWorldSizeZ);
		void refreshHeightMap();
		void refreshParticles();
};

#include "BlendingVisualizer.impl.h"

#endif
