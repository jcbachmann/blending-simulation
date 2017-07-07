#include <mutex>

#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreMeshManager.h>
#include <OgreMovablePlane.h>
#include <Terrain/OgreTerrainMaterialGeneratorA.h>

#include "ParticleLite.h"
#include "BlendingSimulator.h"
#include "QualityCube.h"
#include "BlendingBedVisualizer.h"
#include "QualityColor.h"

template<typename Parameters>
BlendingBedVisualizer<Parameters>::BlendingBedVisualizer(BlendingSimulator<Parameters>* simulator)
	: mSimulationDetailsPanel(nullptr)
	, simulator(simulator)
	, mTerrainGroup(nullptr)
	, mTerrainGlobals(nullptr)
	, showFrozen(false)
	, showTemperature(false)
	, showQualityCubes(false)
{
}

template<typename Parameters>
BlendingBedVisualizer<Parameters>::~BlendingBedVisualizer(void)
{
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::createFrameListener(void)
{
	Visualizer::createFrameListener();

	// create a params panel for displaying simulation details
	Ogre::StringVector items;
	items.push_back("Active Particles");
	items.push_back("Frozen Particles [K]");
	items.push_back("Simulation Status [P]");
	items.push_back("Heap Update");
	items.push_back("Quality Cubes [C]");
	items.push_back("Simulation Temperature [T]");
	mSimulationDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_TOPLEFT, "SimulationDetails", 250, items);
	mSimulationDetailsPanel->show();
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::createScene(void)
{
	// Camera position and direction
	mCameraNode->setPosition(Ogre::Vector3(10, 50, -5));
	mCameraNode->lookAt(Ogre::Vector3(100, 30, 50), Ogre::Node::TS_PARENT);

	// Background color
	mWindow->getViewport(0)->setBackgroundColour(Ogre::ColourValue(0.0, 0.0, 0.0));

	// Lighting and shadows
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.7, 0.7, 0.7));
	mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
	mLight = mSceneMgr->createLight("TestLight");
	mLight->setType(Ogre::Light::LT_DIRECTIONAL);
	mLight->setDirection(Ogre::Vector3(.55f, -.3f, .75f).normalisedCopy());
	mLight->setDiffuseColour(Ogre::ColourValue(1.0, 1.0, 1.0));
	mLight->setSpecularColour(Ogre::ColourValue(1.0, 1.0, 1.0));

	// TODO terrain -> cloth (higher refresh rate / dynamic build)
	// Terrain
	addTerrain();

	// Ground plane
//	addGroundPlane();
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::destroyScene(void)
{
	delete mTerrainGroup;
	delete mTerrainGlobals;
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::frameRendered(const Ogre::FrameEvent& evt)
{
	Visualizer::frameRendered(evt);

	refreshParticles();

	if (showQualityCubes) {
		refreshQualityCubes();
	}
}

template<typename Parameters>
bool BlendingBedVisualizer<Parameters>::keyPressed(const OgreBites::KeyboardEvent& evt)
{
	if (mTrayMgr->isDialogVisible()) {
		return true;
	}

	int key = evt.keysym.sym;
	if (key == SDLK_p) {
		if (simulator->isPaused()) {
			simulator->resume();
		} else {
			simulator->pause();
		}
	} else if (key == SDLK_k) {
		showFrozen = !showFrozen;
	} else if (key == SDLK_t) {
		showTemperature = !showTemperature;
	} else if (key == SDLK_u) {
		refreshHeightMap();
	} else if (key == SDLK_c) {
		showQualityCubes = !showQualityCubes;
		if (!showQualityCubes) {
			for (auto it : qualityCubeMap) {
				for (auto it2 : it.second) {
					mSceneMgr->getRootSceneNode()->removeChild(it2.second->node);
					it2.second->attached = false;
				}
			}
		}
	}

	Visualizer::keyPressed(evt);
	return true;
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::addTerrain(void)
{
	mTerrainGlobals = new Ogre::TerrainGlobalOptions();
	mTerrainGlobals->setMaxPixelError(5);
	mTerrainGlobals->setCompositeMapDistance(3000);
	mTerrainGlobals->setLightMapDirection(mLight->getDerivedDirection());
	mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
	mTerrainGlobals->setCompositeMapDiffuse(mLight->getDiffuseColour());
	mTerrainGlobals->setCastsDynamicShadows(true);

	mTerrainGroup = new Ogre::TerrainGroup(mSceneMgr, Ogre::Terrain::ALIGN_X_Z, (Ogre::uint16) simulator->heapMapRes, float(simulator->heapMapRes) - 1.0f);
	mTerrainGroup->setOrigin(Ogre::Vector3((float(simulator->heapMapRes) - 1.0f) / 2.0f, 0.0, (float(simulator->heapMapRes) - 1.0f) / 2.0f));
	Ogre::TerrainMaterialGeneratorA::SM2Profile* matProfile = static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(mTerrainGlobals->getDefaultMaterialGenerator()
		->getActiveProfile());
	matProfile->setReceiveDynamicShadowsEnabled(true);

	Ogre::Terrain::ImportData& importData = mTerrainGroup->getDefaultImportSettings();
	importData.terrainSize = (Ogre::uint16) simulator->heapMapRes;
	importData.worldSize = float(simulator->heapMapRes) - 1.0f;
	importData.inputScale = 1;
	importData.minBatchSize = 33;
	importData.maxBatchSize = 65;
	importData.layerList.resize(1);
	importData.layerList[0].worldSize = 10;
	importData.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
	importData.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");

	mTerrainGroup->defineTerrain(0, 0, 0.0f);
	mTerrainGroup->loadAllTerrains(true);
	mTerrainGroup->freeTemporaryResources();
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::addGroundPlane(void)
{
	Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
	const Ogre::MaterialPtr& groundMaterial = materialManager.create("Ground", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::TextureUnitState* groundTexture = groundMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("dirt_grayrocky_diffusespecular.dds");
	groundTexture->setTextureScale(0.01, 0.01);
	Ogre::MovablePlane* mPlane = new Ogre::MovablePlane("Plane");
	mPlane->d = 0;
	mPlane->normal = Ogre::Vector3::UNIT_Y;
	Ogre::MeshManager::getSingleton().createPlane(
		"PlaneMesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		*mPlane, 2048, 2048, 1, 1, true, 1, 1, 1, Ogre::Vector3::UNIT_Z
	);
	Ogre::Entity* mPlaneEntity = mSceneMgr->createEntity("PlaneMesh");
	mPlaneEntity->setMaterialName("Ground");
	Ogre::SceneNode* mPlaneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mPlaneNode->attachObject(mPlaneEntity);
	mPlaneNode->setPosition(512, 1, 512);
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::refreshHeightMap(void)
{
	Ogre::Terrain* terrain = mTerrainGroup->getTerrain(0, 0);
	uint16_t size = terrain->getSize();
	float* heightMap = terrain->getHeightData();

	if (heightMap) {
		float* heapMap = simulator->getHeapMap();
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				// match simulation height map to terrain height map
				heightMap[(size - i - 1) * size + j] = heapMap[i * size + j];
			}
		}

		terrain->dirty();
		mTerrainGroup->update();
	}
	static int heapUpdateCount = 0;
	mSimulationDetailsPanel->setParamValue(3, Ogre::StringConverter::toString(++heapUpdateCount));
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::refreshParticles(void)
{
	bool doRefreshHeightMap = false;

	mSimulationDetailsPanel->setParamValue(2, simulator->isPaused() ? "Paused" : "Active");

	{ // Render particles
		std::list<VisualizationParticle*>::iterator cubePoolIterator = cubePool.begin();
		std::lock_guard<std::mutex> lock(simulator->outputParticlesMutex);
		const std::list<ParticleLite<Parameters>*>& particles = simulator->outputParticles;
		mSimulationDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(
			simulator->activeOutputParticles.size()));
		mSimulationDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(
			simulator->outputParticles.size() - simulator->activeOutputParticles.size()));

		static unsigned long lastParticlesSize = 0;
		if (particles.size() > lastParticlesSize + 1000) {
			lastParticlesSize = particles.size();
			doRefreshHeightMap = true;
		}

		for (auto it = particles.begin(); it != particles.end(); it++) {
			ParticleLite<Parameters>* particle = *it;

			// Do not display frozen particles
			if (!showFrozen && particle->frozen) {
				continue;
			}

			// Acquire cube object
			VisualizationParticle* cube;
			if (cubePoolIterator == cubePool.end()) {
				// Create cube
				cube = new VisualizationParticle();
				cube->entity = mSceneMgr->createEntity(
					std::string("CubePool") + std::to_string(cubePool.size()),
					Ogre::SceneManager::PT_CUBE
				);
				cube->node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
				cube->attached = true;
				cube->node->attachObject(cube->entity);
				cubePool.push_back(cube);
				cubePoolIterator = cubePool.end();

				// Create material
				Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
				cube->material = materialManager.create(
					std::string("CubePool") + std::to_string(cubePool.size()),
					Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
				cube->entity->setMaterial(cube->material);
			} else {
				// Reuse cube from pool
				cube = *cubePoolIterator++;

				if (!cube->attached) {
					cube->attached = true;
					mSceneMgr->getRootSceneNode()->addChild(cube->node);
				}
			}

			// Set position, scale and orientation
			bs::Vector3 p = particle->position;
			bs::Quaternion o = particle->orientation;
			bs::Vector3 s = particle->size;
			cube->node->setPosition(float(p.x), float(p.y), float(p.z));
			cube->node->setScale(float(s.x) / 100.0f, float(s.y) / 100.0f, float(s.z) / 100.0f);
			cube->node->setOrientation(float(o.w), float(o.x), float(o.y), float(o.z));

			// Set color
			std::tuple<float, float, float> c;
			if (showTemperature) {
				c = std::make_tuple(particle->temperature, particle->frozen ? 0.0 : (1.0 - particle->temperature),
					1.0 - particle->temperature);
			} else {
				const Parameters& pp = particle->parameters;
				c = hsvToRgb(qualityHue(pp.get(0), pp.get(1), pp.get(2)), 1.0, 1.0);
			}
			cube->material->getTechnique(0)->getPass(0)->setAmbient(std::get<0>(c), std::get<1>(c), std::get<2>(c));
			cube->material->getTechnique(0)->getPass(0)->setDiffuse(std::get<0>(c), std::get<1>(c), std::get<2>(c), 1.0f);
			cube->material->compile();
		}

		// Handle unused cubes in pool
		while (cubePoolIterator != cubePool.end()) {
			mSceneMgr->getRootSceneNode()->removeChild((*cubePoolIterator)->node);
			(*cubePoolIterator)->attached = false;
			cubePoolIterator++;
		}
	}

	// Refresh heap map
	if (doRefreshHeightMap) {
		refreshHeightMap();
	}
}

template<typename Parameters>
void BlendingBedVisualizer<Parameters>::refreshQualityCubes(void)
{
	BlendingSimulatorDetailed<Parameters>* detailedSimulator = dynamic_cast<BlendingSimulatorDetailed<Parameters>*>(simulator);
	if (!detailedSimulator) {
		// Does not apply for other simulator types
		return;
	}

	std::lock_guard<std::mutex> lock2(detailedSimulator->qualityGridMutex);
	const std::map<int, std::map<int, QualityCube<Parameters>>>& qualityGrid = detailedSimulator->qualityGrid;

	for (auto it = qualityGrid.begin(); it != qualityGrid.end(); it++) {
		for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
			const QualityCube<Parameters>& qualityCube = it2->second;

			VisualizationCube* visualizationCube = nullptr;

			auto one = qualityCubeMap.find(it->first);
			if (one != qualityCubeMap.end()) {
				auto two = one->second.find(it2->first);
				if (two != one->second.end()) {
					visualizationCube = two->second;
				}
			}

			Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
			if (!visualizationCube) {
				visualizationCube = new VisualizationCube();
				Ogre::Entity* cubeEnt = mSceneMgr->createEntity(
					std::string("CubeMap") + std::to_string(it->first) + std::to_string(it2->first),
					Ogre::SceneManager::PT_CUBE
				);
				visualizationCube->material = materialManager.create(
					std::string("CubeMap") + std::to_string(it->first) + std::to_string(it2->first),
					Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
				);
				visualizationCube->material->getTechnique(0)->getPass(0)->setPolygonMode(Ogre::PM_WIREFRAME);
				cubeEnt->setMaterial(visualizationCube->material);
				visualizationCube->node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
				visualizationCube->attached = true;

				btVector3 p = qualityCube.position;
				btVector3 s = 0.9 * qualityCube.size / 100.0;

				visualizationCube->node->setPosition(p.x(), p.y(), p.z());
				visualizationCube->node->setScale(s.x(), s.y(), s.z());
				visualizationCube->node->attachObject(cubeEnt);
				qualityCubeMap[it->first][it2->first] = visualizationCube;
			}

			if (!visualizationCube->attached) {
				mSceneMgr->getRootSceneNode()->addChild(visualizationCube->node);
				visualizationCube->attached = true;
			}

			const Parameters& pp = qualityCube.getAverage();
			std::tuple<float, float, float> c = hsvToRgb(qualityHue(pp.get(0), pp.get(1), pp.get(2)), 1.0, 1.0);
			visualizationCube->material->getTechnique(0)->getPass(0)->setAmbient(std::get<0>(c), std::get<1>(c), std::get<2>(c));
			visualizationCube->material->getTechnique(0)->getPass(0)->setDiffuse(std::get<0>(c), std::get<1>(c), std::get<2>(c), 1.0f);
		}
	}
}