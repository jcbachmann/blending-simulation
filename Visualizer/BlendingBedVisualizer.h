#ifndef COALYARD_BLENDINGBEDVISUALIZER_H
#define COALYARD_BLENDINGBEDVISUALIZER_H

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
class BlendingBedVisualizer : public Visualizer
{
	public:
		BlendingBedVisualizer(BlendingSimulator<Parameters>* simulator);
		virtual ~BlendingBedVisualizer(void);

	protected:
		virtual void createFrameListener(void) override;
		virtual void createScene(void) override;
		virtual void destroyScene(void) override;
		virtual void frameRendered(const Ogre::FrameEvent& evt) override;
		virtual bool keyPressed(const OgreBites::KeyboardEvent& evt) override;

	private:
		OgreBites::ParamsPanel* mSimulationDetailsPanel;
		Ogre::TerrainGroup* mTerrainGroup;
		Ogre::TerrainGlobalOptions* mTerrainGlobals;
		BlendingSimulator<Parameters>* simulator;
		std::list<VisualizationParticle*> cubePool;
		std::map<int, std::map<int, VisualizationCube*>> qualityCubeMap;
		bool showFrozen;
		bool showQualityCubes;

		void addTerrain(void);
		void addGroundPlane(void);
		void refreshHeightMap(void);
		void refreshParticles(void);
		void refreshQualityCubes(void);
};

#include "BlendingBedVisualizer.impl.h"

#endif
