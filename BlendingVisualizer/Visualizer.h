#ifndef BLENDING_VISUALIZER_H
#define BLENDING_VISUALIZER_H

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreRoot.h>
#include <OgreViewport.h>
#include <OgreSceneManager.h>
#include <OgreRenderWindow.h>
#include <OgreConfigFile.h>
#include <OgreWindowEventUtilities.h>
#include <OgreFileSystemLayer.h>
#include <Bites/OgreInput.h>
#include <Bites/OgreTrays.h>
#include <Bites/OgreCameraMan.h>
#include <SDL2/SDL.h>

class Visualizer : public Ogre::FrameListener, public OgreBites::WindowEventListener, public OgreBites::InputListener, OgreBites::TrayListener
{
	public:
		explicit Visualizer(bool verbose);
		~Visualizer() override;

		virtual void run();

	protected:
		void initApp();
		void closeApp();

		virtual void createCamera();
		virtual void createFrameListener();
		virtual void createScene() = 0;
		virtual void destroyScene() = 0;
		virtual void loadResources();

		// Ogre::FrameListener
		bool frameStarted(const Ogre::FrameEvent& evt) override;
		bool frameRenderingQueued(const Ogre::FrameEvent& evt) override;
//		bool frameEnded(const Ogre::FrameEvent& evt) override;

		// OgreBites::WindowEventListener
//		void windowMoved(Ogre::RenderWindow* rw) override;
		void windowResized(Ogre::RenderWindow* rw) override;
//		bool windowClosing(Ogre::RenderWindow* rw) override;
//		void windowClosed(Ogre::RenderWindow* rw) override;
//		void windowFocusChange(Ogre::RenderWindow* rw) override;

		// OgreBites::InputListener
		void frameRendered(const Ogre::FrameEvent& evt) override;
		bool keyPressed(const OgreBites::KeyboardEvent& evt) override;
		bool keyReleased(const OgreBites::KeyboardEvent& evt) override;
//		bool touchMoved(const OgreBites::TouchFingerEvent& evt) override;
//		bool touchPressed(const OgreBites::TouchFingerEvent& evt) override;
//		bool touchReleased(const OgreBites::TouchFingerEvent& evt) override;
		bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override;
//		bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) override;
		bool mousePressed(const OgreBites::MouseButtonEvent& evt) override;
		bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override;

		// OgreBites::TrayListener
//		void buttonHit(OgreBites::Button* button) override;
//		void itemSelected(OgreBites::SelectMenu* menu) override;
//		void labelHit(OgreBites::Label* label) override;
//		void sliderMoved(OgreBites::Slider* slider) override;
//		void checkBoxToggled(OgreBites::CheckBox* box) override;
//		void okDialogClosed(const Ogre::DisplayString& message) override;
//		void yesNoDialogClosed(const Ogre::DisplayString& question, bool yesHit) override;

		Ogre::RenderWindow* createWindow();
		bool acquireConfiguration();
		void locateResources();
		void setupInput(bool grab);
		void createDummyScene();
		void destroyDummyScene();
		void fireInputEvent(const SDL_Event& event);
		void pollEvents();
		void grab();
		void ungrab();

		bool grabbed;
		Ogre::FileSystemLayer* mFSLayer;
		Ogre::Viewport* mViewport;
		Ogre::Root* mRoot;
		Ogre::Camera* mCamera;
		Ogre::SceneNode* mCameraNode;
		Ogre::Light* mLight;
		Ogre::SceneManager* mSceneMgr;
		Ogre::OverlaySystem* mOverlaySystem;
		Ogre::RenderWindow* mWindow;
		SDL_Window* mSDLWindow;
		Ogre::String mResourcesCfg;
		Ogre::String mPluginsCfg;

		// OgreBites
		OgreBites::TrayManager* mTrayMgr;
		OgreBites::CameraMan* mCameraMan;
		OgreBites::ParamsPanel* mDetailsPanel;
};

#endif
