#include "MovePrecompiled.h"
#include "EyeInterface.h"

namespace Eye
{

	EyeInterface::EyeInterface(int numMoves)
	{
		this->numMoves=numMoves;
		img=0;
		pCapBuffer=0;
		_cam=0;
		balls[0].ballOutColor=ColorRgb(0,0,255);
		balls[1].ballOutColor=ColorRgb(0,255,0);
	}
	
	bool EyeInterface::initCamera()
	{
		GUID _cameraGUID;

		_cameraGUID = CLEyeGetCameraUUID(0);
		if (_cameraGUID==GUID_NULL)
			return false;
		_cam = CLEyeCreateCamera(_cameraGUID, CLEYE_COLOR_PROCESSED, CLEYE_VGA, 75);
		if (_cam==0)
			return false;
		CLEyeCameraGetFrameDimensions(_cam, width, height);

		CLEyeSetCameraParameter(_cam, CLEYE_GAIN, 0);
		CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true);
		CLEyeSetCameraParameter(_cam, CLEYE_AUTO_WHITEBALANCE, true);
		//CLEyeSetCameraParameter(_cam, CLEYE_WHITEBALANCE_RED, 255);
		//CLEyeSetCameraParameter(_cam, CLEYE_WHITEBALANCE_GREEN, 255);
		//CLEyeSetCameraParameter(_cam, CLEYE_WHITEBALANCE_BLUE, 255);
		CLEyeSetCameraParameter(_cam, CLEYE_HFLIP, true);
		//CLEyeSetCameraParameter(_cam, CLEYE_VFLIP, true);

		_hThread = CreateThread(NULL, 0, &EyeInterface::CaptureThread, this, 0, 0);
		SetPriorityClass(_hThread,REALTIME_PRIORITY_CLASS);
		SetThreadPriority(_hThread,THREAD_PRIORITY_HIGHEST);
		return true;
	}

	EyeInterface::~EyeInterface(void)
	{
		TerminateThread(_hThread,0);

		if (img)
			delete(img);

		if (_cam)
		{
			CLEyeCameraStop(_cam);
			CLEyeDestroyCamera(_cam);
		}
		if (pCapBuffer)
			delete[] pCapBuffer;
	}

	void EyeInterface::Run()
	{
		CLEyeCameraStart(_cam);
		Sleep(100);

		pCapBuffer=new BYTE[width*height*4];
		img = new EyeImage(width,height,pCapBuffer);

		while(true)
		{
			bool ret = CLEyeCameraGetFrame(_cam, pCapBuffer);

			float x,y,size;
			img->findBalls(balls, numMoves);
			for (int i=0; i<numMoves; i++)
			{
				if (balls[i].ballFound)
				{
					x=balls[i].positionX;
					y=balls[i].positionY;
					size=balls[i].ballSize;

					img->drawCircle(Vector2((int)x,(int)y),(int)(size/2),ColorRgb(255,0,255));

					balls[i].ballZ=kballz[i].update(2500/size);
					balls[i].ballX=(x-(width/2))*balls[i].ballZ/400;
					balls[i].ballY=(y-(height/2))*balls[i].ballZ/400;
				}
				balls[i].ballFoundOut=balls[i].ballFound;
			}
		}
	}

	DWORD WINAPI EyeInterface::CaptureThread(LPVOID instance)
	{
		EyeInterface *pThis = (EyeInterface *)instance;
		pThis->Run();
		return 0;
	}
}
