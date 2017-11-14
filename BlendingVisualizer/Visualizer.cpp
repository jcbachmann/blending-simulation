#include "Visualizer.h"

#include <SDL_syswm.h>
#include <OgreLogManager.h>
#include <OgreMaterialManager.h>
#include <OgreTextureManager.h>
#include <Bites/OgreBitesConfigDialog.h>

Visualizer::Visualizer(bool verbose)
	: mRoot(nullptr)
	, mCamera(nullptr)
	, mSceneMgr(nullptr)
	, mWindow(nullptr)
	, mSDLWindow(nullptr)
	, mResourcesCfg(Ogre::BLANKSTRING)
	, mPluginsCfg(Ogre::BLANKSTRING)
	, mTrayMgr(nullptr)
	, mCameraMan(nullptr)
	, mDetailsPanel(nullptr)
	, grabbed(false)
{
	Ogre::LogManager* lm = new Ogre::LogManager();
	lm->createLog("ogre.log", true, verbose, false);

	mFSLayer = new Ogre::FileSystemLayer("Visualizer ... again");
}

Visualizer::~Visualizer()
{
	delete mFSLayer;
	mFSLayer = nullptr;
}

void Visualizer::initApp()
{
	mRoot = new Ogre::Root(mPluginsCfg);
	mRoot->addFrameListener(this);

	mOverlaySystem = new Ogre::OverlaySystem();

	if (!acquireConfiguration()) {
		// User cancelled config dialog
		return;
	}

	mWindow = createWindow();
	Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

	setupInput(false);

	locateResources();
	loadResources();

	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);
	mSceneMgr->addRenderQueueListener(mOverlaySystem);

	Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
	materialManager.setDefaultTextureFiltering(Ogre::TFO_ANISOTROPIC);
	materialManager.setDefaultAnisotropy(8);
	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

	createCamera();
	createScene();
	createFrameListener();
}

void Visualizer::closeApp()
{
	destroyScene();

	mRoot->saveConfig();

	if (mWindow) {
		Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
		mRoot->destroyRenderTarget(mWindow);
		mWindow = nullptr;
	}

	if (mSDLWindow) {
		SDL_DestroyWindow(mSDLWindow);
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		mSDLWindow = nullptr;
	}

	if (mTrayMgr) {
		delete mTrayMgr;
		mTrayMgr = nullptr;
	}

	if (mCameraMan) {
		delete mCameraMan;
		mCameraMan = nullptr;
	}

	if (mOverlaySystem) {
		delete mOverlaySystem;
		mOverlaySystem = nullptr;
	}

	if (mRoot) {
		delete mRoot;
		mRoot = nullptr;
	}
}

Ogre::RenderWindow* Visualizer::createWindow()
{
	const char* appName = "Visualizer";
	mRoot->initialise(false, appName);
	Ogre::NameValuePairList miscParams;
	Ogre::ConfigOptionMap& ropts = mRoot->getRenderSystem()->getConfigOptions();

	std::istringstream mode(ropts["Video Mode"].currentValue);
	size_t width;
	Ogre::String token;
	size_t height;
	mode >> width; // width
	mode >> token; // 'x' as separator between width and height
	mode >> height; // height

	miscParams["FSAA"] = ropts["FSAA"].currentValue;
	miscParams["vsync"] = ropts["VSync"].currentValue;

	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}

	mSDLWindow = SDL_CreateWindow(appName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int) width, (int) height,
		SDL_WINDOW_RESIZABLE);

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(mSDLWindow, &wmInfo);

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	miscParams["parentWindowHandle"] = Ogre::StringConverter::toString(size_t(wmInfo.info.x11.window));
#elif OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	miscParams["externalWindowHandle"] = Ogre::StringConverter::toString(size_t(wmInfo.info.win.window));
#endif
	return mRoot->createRenderWindow(appName, (unsigned int) width, (unsigned int) height, false, &miscParams);
}

void Visualizer::setupInput(bool grab)
{
	if (!mSDLWindow) {
		OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "you must create a SDL window first",
			"SampleContext::setupInput");
	}

	SDL_ShowCursor(grab ? SDL_FALSE : SDL_TRUE);
	SDL_bool sdl_grab = SDL_bool(grab);
	SDL_SetWindowGrab(mSDLWindow, sdl_grab);
	SDL_SetRelativeMouseMode(sdl_grab);
}

void Visualizer::locateResources()
{
	Ogre::ConfigFile cf;
	cf.load(mFSLayer->getConfigFilePath(mResourcesCfg));

	// load resource paths from config file
	Ogre::String sec, type, arch;
	// go through all specified resource groups
	Ogre::ConfigFile::SettingsBySection_::const_iterator seci;
	for (seci = cf.getSettingsBySection().begin(); seci != cf.getSettingsBySection().end(); ++seci) {
		sec = seci->first;
		const Ogre::ConfigFile::SettingsMultiMap& settings = seci->second;
		Ogre::ConfigFile::SettingsMultiMap::const_iterator i;

		// go through all resource paths
		for (i = settings.begin(); i != settings.end(); i++) {
			type = i->first;
			arch = Ogre::FileSystemLayer::resolveBundlePath(i->second);

			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(arch, type, sec);
		}
	}
}

void Visualizer::loadResources()
{
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Essential");
	mTrayMgr = new OgreBites::TrayManager("Tray Manager", mWindow, this);
	mTrayMgr->showBackdrop("SdkTrays/Bands");
	mTrayMgr->getTrayContainer(OgreBites::TL_NONE)->hide();

	createDummyScene();
	mTrayMgr->showLoadingBar(1, 0);
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("General");
	mTrayMgr->hideLoadingBar();

	mTrayMgr->hideBackdrop();
	destroyDummyScene();
}

void Visualizer::createDummyScene()
{
	mWindow->removeAllViewports();
	Ogre::SceneManager* sm = mRoot->createSceneManager(Ogre::ST_GENERIC, "DummyScene");
	sm->addRenderQueueListener(mOverlaySystem);
	Ogre::Camera* cam = sm->createCamera("DummyCamera");
	mWindow->addViewport(cam);
}

void Visualizer::destroyDummyScene()
{
	if (!mRoot->hasSceneManager("DummyScene")) {
		return;
	}

	Ogre::SceneManager* dummyScene = mRoot->getSceneManager("DummyScene");
	dummyScene->removeRenderQueueListener(mOverlaySystem);
	mWindow->removeAllViewports();
	mRoot->destroySceneManager(dummyScene);
}

void Visualizer::createCamera()
{
	mCamera = mSceneMgr->createCamera("Camera");
	mCameraNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mCameraNode->attachObject(mCamera);
	mCameraNode->setFixedYawAxis(true, Ogre::Vector3::UNIT_Y);
	mViewport = mWindow->addViewport(mCamera);
	mViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));
	mCamera->setAspectRatio(Ogre::Real(mViewport->getActualWidth()) / Ogre::Real(mViewport->getActualHeight()));
	mCamera->setAutoAspectRatio(true);
	mCamera->setNearClipDistance(.1);
	bool infiniteClip = mRoot->getRenderSystem()->getCapabilities()->hasCapability(Ogre::RSC_INFINITE_FAR_PLANE);
	if (infiniteClip) {
		mCamera->setFarClipDistance(0);
	} else {
		mCamera->setFarClipDistance(50000);
	}
	mCameraMan = new OgreBites::CameraMan(mCameraNode);
	mCameraMan->setTopSpeed(10);
	mCameraMan->setStyle(OgreBites::CS_MANUAL);
}

void Visualizer::createFrameListener()
{
	windowResized(mWindow);
	// Don't display OGRE cursor for the moment
	mTrayMgr->hideCursor();

	// create a params panel for displaying sample details
	Ogre::StringVector items;
	items.push_back("cam.pX");
	items.push_back("cam.pY");
	items.push_back("cam.pZ");
	items.push_back("");
	items.push_back("cam.oW");
	items.push_back("cam.oX");
	items.push_back("cam.oY");
	items.push_back("cam.oZ");
	items.push_back("");
	items.push_back("Poly Mode");

	mDetailsPanel = mTrayMgr->createParamsPanel(OgreBites::TL_NONE, "DetailsPanel2", 200, items);
	mDetailsPanel->setParamValue(9, "Solid");
	mDetailsPanel->hide();
}

bool Visualizer::acquireConfiguration()
{
	if (!mRoot->restoreConfig()) {
		return mRoot->showConfigDialog(OgreBites::getNativeConfigDialog());
	}
	return true;
}

void Visualizer::run()
{
#ifdef _DEBUG
	mResourcesCfg = "resources_d.cfg";
	mPluginsCfg = "plugins_d.cfg";
#else
	mResourcesCfg = "resources.cfg";
	mPluginsCfg = "plugins.cfg";
#endif

	initApp();

	if (mRoot->getRenderSystem()) {
		mRoot->startRendering();
	}

	closeApp();
}

bool Visualizer::frameStarted(const Ogre::FrameEvent& evt)
{
	pollEvents();
	return true;
}

bool Visualizer::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	frameRendered(evt);
	return true;
}

void Visualizer::frameRendered(const Ogre::FrameEvent& evt)
{
	if (mWindow->isClosed()) {
		return;
	}

	mTrayMgr->frameRendered(evt);

	if (!mTrayMgr->isDialogVisible()) {
		mCameraMan->frameRendered(evt);

		if (mDetailsPanel->isVisible()) {
			mDetailsPanel->setParamValue(0, Ogre::StringConverter::toString(mCamera->getDerivedPosition().x));
			mDetailsPanel->setParamValue(1, Ogre::StringConverter::toString(mCamera->getDerivedPosition().y));
			mDetailsPanel->setParamValue(2, Ogre::StringConverter::toString(mCamera->getDerivedPosition().z));
			mDetailsPanel->setParamValue(4, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().w));
			mDetailsPanel->setParamValue(5, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().x));
			mDetailsPanel->setParamValue(6, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().y));
			mDetailsPanel->setParamValue(7, Ogre::StringConverter::toString(mCamera->getDerivedOrientation().z));
		}
	}
}

bool Visualizer::keyPressed(const OgreBites::KeyboardEvent& evt)
{
	if (mTrayMgr->isDialogVisible()) {
		return true;
	}

	int key = evt.keysym.sym;
	if (key == SDLK_f) {
		if (mTrayMgr->areFrameStatsVisible()) {
			mTrayMgr->hideFrameStats();
		} else {
			mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
		}
	} else if (key == SDLK_g) {
		if (mDetailsPanel->getTrayLocation() == OgreBites::TL_NONE) {
			mTrayMgr->moveWidgetToTray(mDetailsPanel, OgreBites::TL_TOPRIGHT, 0);
			mDetailsPanel->show();
		} else {
			mTrayMgr->removeWidgetFromTray(mDetailsPanel);
			mDetailsPanel->hide();
		}
	} else if (key == SDLK_r) {
		Ogre::String newVal;
		Ogre::PolygonMode pm;

		switch (mCamera->getPolygonMode()) {
			case Ogre::PM_SOLID:
				newVal = "Wireframe";
				pm = Ogre::PM_WIREFRAME;
				break;
			default:
				newVal = "Solid";
				pm = Ogre::PM_SOLID;
				break;
		}

		mCamera->setPolygonMode(pm);
		mDetailsPanel->setParamValue(9, newVal);
	} else if (key == SDLK_ESCAPE) {
		if (grabbed) {
			ungrab();
		} else {
			mRoot->queueEndRendering();
		}
	}

	mCameraMan->keyPressed(evt);
	return true;
}

bool Visualizer::keyReleased(const OgreBites::KeyboardEvent& evt)
{
	mCameraMan->keyReleased(evt);
	return true;
}

bool Visualizer::mouseMoved(const OgreBites::MouseMotionEvent& evt)
{
	if (mTrayMgr->mouseMoved(evt)) {
		return true;
	}
	mCameraMan->mouseMoved(evt);
	return true;
}

bool Visualizer::mousePressed(const OgreBites::MouseButtonEvent& evt)
{
	if (mTrayMgr->mousePressed(evt)) {
		return true;
	}

	if (evt.button == OgreBites::BUTTON_LEFT && !grabbed) {
		grab();
	}

	mCameraMan->mousePressed(evt);
	return true;
}

bool Visualizer::mouseReleased(const OgreBites::MouseButtonEvent& evt)
{
	if (mTrayMgr->mouseReleased(evt)) {
		return true;
	}

	mCameraMan->mouseReleased(evt);
	return true;
}

void Visualizer::windowResized(Ogre::RenderWindow* rw)
{
	// Adjust mouse clipping area
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);

	mCamera->setAspectRatio((Ogre::Real) mViewport->getActualWidth() / (Ogre::Real) mViewport->getActualHeight());
}

void Visualizer::fireInputEvent(const OgreBites::Event& event)
{
	switch (event.type) {
		case SDL_KEYDOWN:
			// Ignore repeated signals from key being held down.
			if (event.key.repeat) {
				break;
			}
			keyPressed(event.key);
			break;
		case SDL_KEYUP:
			keyReleased(event.key);
			break;
		case SDL_MOUSEBUTTONDOWN:
			mousePressed(event.button);
			break;
		case SDL_MOUSEBUTTONUP:
			mouseReleased(event.button);
			break;
		case SDL_MOUSEWHEEL:
			mouseWheelRolled(event.wheel);
			break;
		case SDL_MOUSEMOTION:
			mouseMoved(event.motion);
			break;
		default:
			break;
	}
}

void Visualizer::pollEvents()
{
	if (!mSDLWindow) {
		// SDL events not initialized
		return;
	}

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				mRoot->queueEndRendering();
				break;
			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					mWindow->resize((unsigned int) event.window.data1, (unsigned int) event.window.data2);
					windowResized(mWindow);
				}
				break;
			default:
				fireInputEvent(event);
				break;
		}
	}
}

void Visualizer::grab()
{
	grabbed = true;
	setupInput(true);
	mCameraMan->setStyle(OgreBites::CS_FREELOOK);
}

void Visualizer::ungrab()
{
	grabbed = false;
	setupInput(false);
	mCameraMan->setStyle(OgreBites::CS_MANUAL);
}