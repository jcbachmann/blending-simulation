#include "BlendingVisualizer.h"

#include <mutex>

#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreMeshManager.h>
#include <OgreMovablePlane.h>
#include <Terrain/OgreTerrainMaterialGeneratorA.h>
#include <Ogre.h>

#include "Particle.h"
#include "BlendingSimulator.h"
#include "ParameterCube.h"
#include "QualityColor.h"
#include "Noise.h"

template<typename Parameters>
BlendingVisualizer<Parameters>::BlendingVisualizer(BlendingSimulator<Parameters>* simulator, bool verbose, bool pretty)
	: Visualizer(verbose)
	, pretty(pretty)
	, mSimulationDetailsPanel(nullptr)
	, simulator(simulator)
	, mTerrainGroup(nullptr)
	, mTerrainGlobals(nullptr)
	, showParameterCubes(false)
	, heapMesh(nullptr)
{
}

template<typename Parameters>
BlendingVisualizer<Parameters>::~BlendingVisualizer()
{
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::createFrameListener()
{
	Visualizer::createFrameListener();

	// create a params panel for displaying simulation details
	Ogre::StringVector items;
	items.push_back("Active Particles");
	items.push_back("Frozen Particles");
	items.push_back("Simulation Status [P]");
	items.push_back("Heap Updates [U]");
	items.push_back("Parameter Cubes [C]");
	mSimulationDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_TOPLEFT, "SimulationDetails", 250, items);

	// display values of parameters which are only refreshed on action
	mSimulationDetailsPanel->setParamValue(0, "0");
	mSimulationDetailsPanel->setParamValue(1, "0");
	mSimulationDetailsPanel->setParamValue(2, simulator->isPaused() ? "Paused" : "Active");
	mSimulationDetailsPanel->setParamValue(3, "0");
	mSimulationDetailsPanel->setParamValue(4, showParameterCubes ? "yes" : "no");

	mSimulationDetailsPanel->show();
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::createScene()
{
	// Camera position and direction
	mCameraNode->setPosition(Ogre::Vector3(10, 50, -5));
	mCameraNode->lookAt(Ogre::Vector3(100, 30, 50), Ogre::Node::TS_PARENT);

	// Background color
	mWindow->getViewport(0)->setBackgroundColour(Ogre::ColourValue(0.1, 0.1, 0.1));

	// Fog
	mSceneMgr->setFog(Ogre::FOG_EXP2, Ogre::ColourValue(0.8, 0.8, 1.0), 0.0005);

	// Lighting and shadows
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
	mLight = mSceneMgr->createLight("MainLight");
	mLight->setType(Ogre::Light::LT_DIRECTIONAL);
	mLight->setDirection(Ogre::Vector3(.55f, -.3f, .75f).normalisedCopy());
	mLight->setDiffuseColour(Ogre::ColourValue(0.7, 0.7, 0.7));
	mLight->setSpecularColour(Ogre::ColourValue(0.7, 0.7, 0.7));

	// Skybox
	mSceneMgr->setSkyBox(true, "CloudyNoonSkyBox");

	// Ground / Terrain
	auto heapWorldSize = simulator->getHeapWorldSize();

	if (pretty) {
		addTerrain(heapWorldSize.first, heapWorldSize.second);
	} else {
		addGroundPlane();
	}

	// Heap
	addHeap(heapWorldSize.first, heapWorldSize.second);

	// Particles
	instanceManager = mSceneMgr->createInstanceManager("InstanceManager", "Particle.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
		Ogre::InstanceManager::ShaderBased, 1000, Ogre::IM_USEALL);
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::destroyScene()
{
	for (auto inactiveParticle : inactiveParticles) {
		Ogre::SceneNode* sceneNode = inactiveParticle->entity->getParentSceneNode();
		if (sceneNode) {
			sceneNode->detachAllObjects();
			sceneNode->getParentSceneNode()->removeAndDestroyChild(sceneNode->getName());
		}

		mSceneMgr->destroyInstancedEntity(inactiveParticle->entity);
		delete inactiveParticle;
	}
	inactiveParticles.clear();

	instanceManager->cleanupEmptyBatches();
	mSceneMgr->destroyInstanceManager(instanceManager);

	Ogre::MeshManager::getSingleton().remove("ground", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

	if (mTerrainGroup) {
		delete mTerrainGroup;
		mTerrainGroup = nullptr;
	}

	if (mTerrainGlobals) {
		delete mTerrainGlobals;
		mTerrainGlobals = nullptr;
	}

	if (heapMesh) {
		delete heapMesh;
		heapMesh = nullptr;
	}
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::frameRendered(const Ogre::FrameEvent& evt)
{
	Visualizer::frameRendered(evt);

	refreshParticles();

	if (showParameterCubes) {
		refreshParameterCubes();
	}
}

template<typename Parameters>
bool BlendingVisualizer<Parameters>::keyPressed(const OgreBites::KeyboardEvent& evt)
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
	} else if (key == SDLK_u) {
		refreshHeightMap();
	} else if (key == SDLK_c) {
		showParameterCubes = !showParameterCubes;
		if (!showParameterCubes) {
			for (auto& it : visualizationCubes) {
				mSceneMgr->getRootSceneNode()->removeChild(it.second->node);
				it.second->attached = false;
			}
		}
		mSimulationDetailsPanel->setParamValue(4, showParameterCubes ? "yes" : "no");
	}

	Visualizer::keyPressed(evt);
	return true;
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::addTerrain(float flatSizeX, float flatSizeZ)
{
	const uint16_t terrainSize = 2048 + 1;
	const float terrainExtend = 4;
	const float terrainWorldSize = std::max(flatSizeX, flatSizeZ) * terrainExtend;

	mTerrainGlobals = new Ogre::TerrainGlobalOptions();
	mTerrainGlobals->setMaxPixelError(2);
	mTerrainGlobals->setCompositeMapDistance(3000);
	mTerrainGlobals->setLightMapDirection(mLight->getDerivedDirection());
	mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
	mTerrainGlobals->setCompositeMapDiffuse(mLight->getDiffuseColour());
	mTerrainGlobals->setCastsDynamicShadows(true);

	mTerrainGroup = new Ogre::TerrainGroup(mSceneMgr, Ogre::Terrain::ALIGN_X_Z, (Ogre::uint16) terrainSize, terrainWorldSize);
	mTerrainGroup->setOrigin(Ogre::Vector3(flatSizeX / 2.0f, 0.0, flatSizeZ / 2.0f));
	Ogre::TerrainMaterialGeneratorA::SM2Profile* matProfile = dynamic_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(
		mTerrainGlobals->getDefaultMaterialGenerator()->getActiveProfile());
	matProfile->setReceiveDynamicShadowsEnabled(false);

	Ogre::Terrain::ImportData& importData = mTerrainGroup->getDefaultImportSettings();
	importData.terrainSize = (Ogre::uint16) terrainSize;
	importData.worldSize = terrainWorldSize;
	importData.inputScale = 1;
	importData.minBatchSize = 33;
	importData.maxBatchSize = 65;
	importData.layerList.resize(1);
	importData.layerList[0].worldSize = 10;
	importData.layerList[0].textureNames.push_back("Ground_diffusespecular.dds");
	importData.layerList[0].textureNames.push_back("Ground_normalheight.dds");

	mTerrainGroup->defineTerrain(0, 0, 0.0f);
	mTerrainGroup->loadAllTerrains(true);
	mTerrainGroup->freeTemporaryResources();

	Ogre::Terrain* terrain = mTerrainGroup->getTerrain(0, 0);
	uint16_t size = terrain->getSize();
	float* heightMap = terrain->getHeightData();
	Noise noise = Noise();
	constexpr const float pi = std::atan(1.0f) * 4.0f;
	constexpr const float leak = 5.0f;
	for (int z = 0; z < size; z++) {
		for (int x = 0; x < size; x++) {
			float xAbs = terrainWorldSize * float(x) / float(size) - terrainWorldSize / 2.0f + flatSizeX / 2.0f;
			float zAbs = terrainWorldSize * float(z) / float(size) - terrainWorldSize / 2.0f + flatSizeZ / 2.0f;
			float xDistance = 0;
			float zDistance = 0;
			if (xAbs < leak) {
				xDistance = leak - xAbs;
			} else if (xAbs > flatSizeX - leak) {
				xDistance = flatSizeX - leak - xAbs;
			}
			if (zAbs < leak) {
				zDistance = leak - zAbs;
			} else if (zAbs > flatSizeZ - leak) {
				zDistance = flatSizeZ - leak - zAbs;
			}
			float distance = hypotf(xDistance, zDistance);
			float maxRelevantDistance = 250.0f;
			float relativeDistance = std::min(maxRelevantDistance, distance) / maxRelevantDistance;
			float distanceFactor = 0.5f * (1.0f - std::cos(relativeDistance * pi));
			float sideHeapHeight = 5.0f * 0.5f * (1.0f - std::cos(std::min(distance / 10.0f, 1.0f) * 2.0f * pi));

			if (xAbs >= leak && xAbs < flatSizeX - leak && zAbs >= leak && zAbs < flatSizeZ - leak) {
				heightMap[(size - z - 1) * size + x] = 0.0f;
			} else {
				heightMap[(size - z - 1) * size + x] = sideHeapHeight + distanceFactor * 75.0f * (0.75f + noise.get(0.004 * Ogre::Vector2(x, z)));
			}
		}
	}

	terrain->dirty();
	mTerrainGroup->update();
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::addGroundPlane()
{
	Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
	const Ogre::MaterialPtr& groundMaterial = materialManager.create("Ground", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::TextureUnitState* groundTexture = groundMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("Ground_diffusespecular.dds");
	groundTexture->setTextureScale(0.03, 0.03);

	Ogre::MeshManager::getSingleton().createPlane("ground", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::Plane(Ogre::Vector3::UNIT_Y, -0.5f),
		10000, 10000, 20, 20, true, 1, 6, 6, Ogre::Vector3::UNIT_Z);
	Ogre::Entity* ground = mSceneMgr->createEntity("Ground", "ground");
	ground->setMaterial(groundMaterial);
	ground->setCastShadows(false);
	mSceneMgr->getRootSceneNode()->attachObject(ground);
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::addHeap(float heapWorldSizeX, float heapWorldSizeZ)
{
	Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
	const Ogre::MaterialPtr& groundMaterial = materialManager.create("HeapIn", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::TextureUnitState* groundTexture = groundMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("Heap.dds");
	float textureScale = 10.0f;
	groundTexture->setTextureScale(textureScale / heapWorldSizeX, textureScale / heapWorldSizeZ);

	std::pair<unsigned int, unsigned int> heapMapSize = simulator->getHeapMapSize();
	heapMesh = new HeapMesh("HeapMesh", heapMapSize.first, heapMapSize.second, heapWorldSizeX, heapWorldSizeZ, false);
	heapEntity = mSceneMgr->createEntity("HeapEntity", "HeapMesh");
	heapEntity->setCastShadows(true);
	heapEntity->setMaterial(groundMaterial);
	mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(heapEntity);
	heapMesh->updateMesh(simulator->getHeapMap());
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::refreshHeightMap()
{
	if (!heapMesh) {
		return;
	}

	static int heapUpdateCount = 0;
	mSimulationDetailsPanel->setParamValue(3, Ogre::StringConverter::toString(++heapUpdateCount));
	heapMesh->updateMesh(simulator->getHeapMap());
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::refreshParticles()
{
	bool doRefreshHeightMap = false;

	mSimulationDetailsPanel->setParamValue(2, simulator->isPaused() ? "Paused" : "Active");

	std::deque<VisualizationParticle*>::iterator cubePoolIterator = activeParticlePool.begin();

	{
		std::lock_guard<std::mutex> lock(simulator->outputParticlesMutex);

		// Display particle information
		mSimulationDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(simulator->activeOutputParticles.size()));
		mSimulationDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(simulator->inactiveOutputParticles.size()));

		static unsigned long lastParticlesSize = 0;
		if (simulator->inactiveOutputParticles.size() > lastParticlesSize + 100) {
			lastParticlesSize = simulator->inactiveOutputParticles.size();
			doRefreshHeightMap = true;
		}

		// Refresh active particles
		for (auto it = simulator->activeOutputParticles.begin(); it != simulator->activeOutputParticles.end(); it++) {
			Particle<Parameters>* particle = *it;

			// Acquire cube object
			VisualizationParticle* cube;
			if (cubePoolIterator == activeParticlePool.end()) {
				// Create cube
				cube = new VisualizationParticle();
				cube->entity = mSceneMgr->createEntity(std::string("CubePool") + std::to_string(activeParticlePool.size()), Ogre::SceneManager::PT_CUBE);
				cube->entity->setCastShadows(true);
				cube->node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
				cube->attached = true;
				cube->node->attachObject(cube->entity);
				activeParticlePool.push_back(cube);
				cubePoolIterator = activeParticlePool.end();
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
			std::string materialName = "";
			const Parameters& pp = particle->parameters;
			if (pp.getValue(0) > pp.getValue(1) && pp.getValue(0) > pp.getValue(2)) {
				materialName = "Particle_red";
			} else if (pp.getValue(1) > pp.getValue(2)) {
				materialName = "Particle_blue";
			} else {
				materialName = "Particle_yellow";
			}

			cube->entity->setMaterialName(materialName);
		}

		// Refresh inactive particles
		if (simulator->inactiveOutputParticles.size() > inactiveParticles.size()) {
			static int defragmentBatchesCounter = 0;
			defragmentBatchesCounter += simulator->inactiveOutputParticles.size() - inactiveParticles.size();

			// Add new inactive particles
			for (auto it = simulator->inactiveOutputParticles.begin() + inactiveParticles.size(); it != simulator->inactiveOutputParticles.end(); it++) {
				Particle<Parameters>* particle = *it;

				// Create cube
				VisualizationInstancedParticle* cube = new VisualizationInstancedParticle();

				// Set color
				std::string materialName = "";
				const Parameters& pp = particle->parameters;
				if (pp.getValue(0) > pp.getValue(1) && pp.getValue(0) > pp.getValue(2)) {
					materialName = "Particle_red";
				} else if (pp.getValue(1) > pp.getValue(2)) {
					materialName = "Particle_blue";
				} else {
					materialName = "Particle_yellow";
				}

				// Create entity
				cube->entity = instanceManager->createInstancedEntity(materialName);
				cube->entity->setCastShadows(true);

				// Set position, scale and orientation
				bs::Vector3 p = particle->position;
				bs::Quaternion o = particle->orientation;
				bs::Vector3 s = particle->size;
				cube->entity->setPosition(Ogre::Vector3(float(p.x), float(p.y), float(p.z)));
				cube->entity->setScale(Ogre::Vector3(float(s.x), float(s.y), float(s.z)));
				cube->entity->setOrientation(Ogre::Quaternion(float(o.w), float(o.x), float(o.y), float(o.z)));

				// Group all inactive particles
				inactiveParticles.push_back(cube);
			}

			if (defragmentBatchesCounter > 1000) {
				defragmentBatchesCounter = 0;
//				instanceManager->defragmentBatches(true); // Causes lag spikes
			}
		}
	}

	// Handle unused cubes in pool
	while (cubePoolIterator != activeParticlePool.end()) {
		mSceneMgr->getRootSceneNode()->removeChild((*cubePoolIterator)->node);
		(*cubePoolIterator)->attached = false;
		cubePoolIterator++;
	}

	// Refresh heap map
	if (doRefreshHeightMap) {
		refreshHeightMap();
	}
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::refreshParameterCubes()
{
	std::lock_guard<std::mutex> lock(simulator->parameterCubesMutex);

	for (auto it = simulator->parameterCubes.begin(); it != simulator->parameterCubes.end(); it++) {
		const ParameterCube<Parameters>* parameterCube = it->second;

		VisualizationCube* visualizationCube = nullptr;

		auto visIt = visualizationCubes.find(it->first);
		if (visIt != visualizationCubes.end()) {
			visualizationCube = visIt->second;
		}

		Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
		if (!visualizationCube) {
			std::string identifier =
				"_" + std::to_string(std::get<0>(it->first)) + "_" + std::to_string(std::get<1>(it->first)) + "_" + std::to_string(std::get<2>(it->first));

			visualizationCube = new VisualizationCube();
			Ogre::Entity* cubeEnt = mSceneMgr->createEntity(std::string("CubeMap") + identifier, Ogre::SceneManager::PT_CUBE);
			visualizationCube->material = materialManager.create(std::string("CubeMap") + identifier, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			visualizationCube->material->getTechnique(0)->getPass(0)->setPolygonMode(Ogre::PM_WIREFRAME);
			cubeEnt->setMaterial(visualizationCube->material);
			cubeEnt->setCastShadows(false);
			visualizationCube->node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
			visualizationCube->attached = true;

			bs::Vector3 p = parameterCube->getPosition();
			float s = 0.9f * parameterCube->getSize() / 100.0f;

			visualizationCube->node->setPosition(Ogre::Real(p.x), Ogre::Real(p.y), Ogre::Real(p.z));
			visualizationCube->node->setScale(s, s, s);
			visualizationCube->node->attachObject(cubeEnt);
			visualizationCubes[it->first] = visualizationCube;
		}

		if (!visualizationCube->attached) {
			mSceneMgr->getRootSceneNode()->addChild(visualizationCube->node);
			visualizationCube->attached = true;
		}

		const Parameters& pp = parameterCube->getParameters();
		std::tuple<double, double, double> c = qualityColor(pp.getValue(0), pp.getValue(1), pp.getValue(2));
		visualizationCube->material->getTechnique(0)->getPass(0)->setAmbient(0, 0, 0);
		visualizationCube->material->getTechnique(0)->getPass(0)->setDiffuse(0, 0, 0, 0);
		visualizationCube->material->getTechnique(0)->getPass(0)->setEmissive((float) std::get<0>(c), (float) std::get<1>(c), (float) std::get<2>(c));
	}
}