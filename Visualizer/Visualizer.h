#ifndef COALYARD_VISUALIZER_H
#define COALYARD_VISUALIZER_H

#include <OgreCamera.h>
#include <OgreEntity.h>
#include <OgreLogManager.h>
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

class Visualizer
	: public Ogre::FrameListener,
	  public Ogre::WindowEventListener,
	  public OgreBites::InputListener,
	  OgreBites::TrayListener
{
	public:
		Visualizer();
		virtual ~Visualizer();

		virtual void run(void);

	protected:
		void initApp();
		void closeApp();

		virtual void createCamera(void);
		virtual void createFrameListener(void);
		virtual void createScene(void) = 0;
		virtual void destroyScene(void) = 0;
		virtual void loadResources(void);

		// Ogre::FrameListener
		virtual bool frameStarted(const Ogre::FrameEvent& evt) override;
		virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt) override;
//		virtual bool frameEnded(const Ogre::FrameEvent& evt) override;

		// Ogre::WindowEventListener
//		virtual void windowMoved(Ogre::RenderWindow* rw) override;
		virtual void windowResized(Ogre::RenderWindow* rw) override;
//		virtual bool windowClosing(Ogre::RenderWindow* rw) override;
//		virtual void windowClosed(Ogre::RenderWindow* rw) override;
//		virtual void windowFocusChange(Ogre::RenderWindow* rw) override;

		// OgreBites::InputListener
		virtual void frameRendered(const Ogre::FrameEvent& evt) override;
		virtual bool keyPressed(const OgreBites::KeyboardEvent& evt) override;
		virtual bool keyReleased(const OgreBites::KeyboardEvent& evt) override;
//		virtual bool touchMoved(const OgreBites::TouchFingerEvent& evt) override;
//		virtual bool touchPressed(const OgreBites::TouchFingerEvent& evt) override;
//		virtual bool touchReleased(const OgreBites::TouchFingerEvent& evt) override;
		virtual bool mouseMoved(const OgreBites::MouseMotionEvent& evt) override;
//		virtual bool mouseWheelRolled(const OgreBites::MouseWheelEvent& evt) override;
		virtual bool mousePressed(const OgreBites::MouseButtonEvent& evt) override;
		virtual bool mouseReleased(const OgreBites::MouseButtonEvent& evt) override;

		// OgreBites::TrayListener
//		virtual void buttonHit(OgreBites::Button* button) override;
//		virtual void itemSelected(OgreBites::SelectMenu* menu) override;
//		virtual void labelHit(OgreBites::Label* label) override;
//		virtual void sliderMoved(OgreBites::Slider* slider) override;
//		virtual void checkBoxToggled(OgreBites::CheckBox* box) override;
//		virtual void okDialogClosed(const Ogre::DisplayString& message) override;
//		virtual void yesNoDialogClosed(const Ogre::DisplayString& question, bool yesHit) override;

		Ogre::RenderWindow* createWindow();
		bool acquireConfiguration();
		void locateResources();
		void setupInput(bool grab);
		void createDummyScene();
		void destroyDummyScene();
		void fireInputEvent(const OgreBites::Event& event);
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