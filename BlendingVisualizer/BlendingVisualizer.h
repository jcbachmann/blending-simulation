#ifndef BLENDINGVISUALIZER_H
#define BLENDINGVISUALIZER_H

#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>

#include "Visualizer.h"

template<typename Parameters>
class BlendingSimulator;

struct VisualizationParticle
{
	VisualizationParticle()
		: attached(false)
		, node(nullptr)
	{
	}

	bool attached;
	Ogre::Entity* entity;
	Ogre::SceneNode* node;
	Ogre::MaterialPtr material;
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
		BlendingVisualizer(BlendingSimulator<Parameters>* simulator, bool verbose);
		virtual ~BlendingVisualizer();

	protected:
		virtual void createFrameListener() override;
		virtual void createScene() override;
		virtual void destroyScene() override;
		virtual void frameRendered(const Ogre::FrameEvent& evt) override;
		virtual bool keyPressed(const OgreBites::KeyboardEvent& evt) override;

	private:
		OgreBites::ParamsPanel* mSimulationDetailsPanel;
		Ogre::TerrainGroup* mTerrainGroup;
		Ogre::TerrainGlobalOptions* mTerrainGlobals;
		BlendingSimulator<Parameters>* simulator;
		std::deque<VisualizationParticle*> particlePool;
		std::map<std::tuple<int, int, int>, VisualizationCube*> visualizationCubes;
		bool showFrozen;
		bool showTemperature;
		bool showParameterCubes;

		void addTerrain(void);
		void addGroundPlane();
		void refreshHeightMap();
		void refreshParticles();
		void refreshParameterCubes();
};

#include "BlendingVisualizer.impl.h"

#endif
