#pragma once
#include "BaseApplication.h"
#include "IMoveManager.h"
#include "IMoveObserver.h"
#include "IMoveCalibrationObserver.h"
#include "MoveFactory.h"
#include "MoveData.h"

#include "stdafx.h"

class Game : public BaseApplication, Move::IMoveObserver, Move::IMoveCalibrationObserver
{
	struct GameState
	{
		enum Enum
		{
			Normal,
			WaitForXBeforeCalibration,
			Calibrating,
			Calculating
		};
		GameState(Enum _value = Normal) : value(_value) { }
		friend bool operator == (GameState const& a, GameState const& b) { return a.value == b.value; }
		friend bool operator != (GameState const& a, GameState const& b) { return a.value != b.value; }
	private:
		Enum value;
	};

	Ogre::SceneNode* moveNode[2];

	Ogre::TexturePtr camImage;

	int calibratingMove;

	Move::IMoveManager* move;
	GameState state;
	int numMoves;
	bool useMagnetometers;

	bool guiInitialized;

public:
    Game(void);
    virtual ~Game(void);

protected:
    virtual void createScene(void);
	virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
	virtual bool configure();
	bool keyPressed( const OIS::KeyEvent &arg );

	bool initMove();
	void copyCameraImageToTexture(Ogre::TexturePtr texture);

private:
	void initGui();
	void guiExitClick(MyGUI::Widget* _widget);
	void guiReadCalibrationClick(MyGUI::Widget* _widget);
	void guiCalibrateClick(MyGUI::Widget* _widget);

public:
	void calibrationDone(int moveId);

	void moveKeyPressed(int moveId, int keyCode);
	void moveKeyReleased(int moveId, int keyCode);
	void moveUpdated(int moveId, Move::MoveData data);
};


