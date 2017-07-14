#include <mutex>

#include <OgreMaterial.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreMeshManager.h>
#include <OgreMovablePlane.h>
#include <Terrain/OgreTerrainMaterialGeneratorA.h>

#include "ParticleLite.h"
#include "BlendingSimulator.h"
#include "ParameterCube.h"
#include "BlendingVisualizer.h"
#include "QualityColor.h"

template<typename Parameters>
BlendingVisualizer<Parameters>::BlendingVisualizer(BlendingSimulator<Parameters>* simulator)
	: mSimulationDetailsPanel(nullptr)
	, simulator(simulator)
	, mTerrainGroup(nullptr)
	, mTerrainGlobals(nullptr)
	, showFrozen(false)
	, showTemperature(false)
	, showParameterCubes(false)
{
}

template<typename Parameters>
BlendingVisualizer<Parameters>::~BlendingVisualizer(void)
{
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::createFrameListener(void)
{
	Visualizer::createFrameListener();

	// create a params panel for displaying simulation details
	Ogre::StringVector items;
	items.push_back("Active Particles");
	items.push_back("Frozen Particles [K]");
	items.push_back("Simulation Status [P]");
	items.push_back("Heap Update");
	items.push_back("Parameter Cubes [C]");
	items.push_back("Simulation Temperature [T]");
	mSimulationDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_TOPLEFT, "SimulationDetails", 250, items);
	mSimulationDetailsPanel->show();
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::createScene(void)
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
void BlendingVisualizer<Parameters>::destroyScene(void)
{
	delete mTerrainGroup;
	delete mTerrainGlobals;
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
	} else if (key == SDLK_k) {
		showFrozen = !showFrozen;
	} else if (key == SDLK_t) {
		showTemperature = !showTemperature;
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
	}

	Visualizer::keyPressed(evt);
	return true;
}

template<typename Parameters>
void BlendingVisualizer<Parameters>::addTerrain(void)
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
void BlendingVisualizer<Parameters>::addGroundPlane(void)
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
void BlendingVisualizer<Parameters>::refreshHeightMap(void)
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
void BlendingVisualizer<Parameters>::refreshParticles(void)
{
	bool doRefreshHeightMap = false;

	mSimulationDetailsPanel->setParamValue(2, simulator->isPaused() ? "Paused" : "Active");

	{ // Render particles
		std::deque<VisualizationParticle*>::iterator cubePoolIterator = particlePool.begin();
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
			if (cubePoolIterator == particlePool.end()) {
				// Create cube
				cube = new VisualizationParticle();
				cube->entity = mSceneMgr->createEntity(
					std::string("CubePool") + std::to_string(particlePool.size()),
					Ogre::SceneManager::PT_CUBE
				);
				cube->node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
				cube->attached = true;
				cube->node->attachObject(cube->entity);
				particlePool.push_back(cube);
				cubePoolIterator = particlePool.end();

				// Create material
				Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
				cube->material = materialManager.create(
					std::string("CubePool") + std::to_string(particlePool.size()),
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
		while (cubePoolIterator != particlePool.end()) {
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
void BlendingVisualizer<Parameters>::refreshParameterCubes(void)
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
			std::string identifier = "_" + std::to_string(std::get<0>(it->first))
									 + "_" + std::to_string(std::get<1>(it->first))
									 + "_" + std::to_string(std::get<2>(it->first));

			visualizationCube = new VisualizationCube();
			Ogre::Entity* cubeEnt = mSceneMgr->createEntity(
				std::string("CubeMap") + identifier,
				Ogre::SceneManager::PT_CUBE
			);
			visualizationCube->material = materialManager.create(
				std::string("CubeMap") + identifier,
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			);
			visualizationCube->material->getTechnique(0)->getPass(0)->setPolygonMode(Ogre::PM_WIREFRAME);
			cubeEnt->setMaterial(visualizationCube->material);
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
		std::tuple<float, float, float> c = hsvToRgb(qualityHue(pp.get(0), pp.get(1), pp.get(2)), 1.0, 1.0);
		visualizationCube->material->getTechnique(0)->getPass(0)->setAmbient(std::get<0>(c), std::get<1>(c), std::get<2>(c));
		visualizationCube->material->getTechnique(0)->getPass(0)->setDiffuse(std::get<0>(c), std::get<1>(c), std::get<2>(c), 1.0f);
	}
}