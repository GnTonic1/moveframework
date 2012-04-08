#include "BallManager.h"
#include "MoveDevice.h"
#include "MoveExceptions.h"

namespace Move
{

	BallManager::BallManager(int numMoves, EyeImage* img)
	{
		this->numMoves=numMoves;
		this->img=img;
		offset=Vec3::ZERO;

		balls.resize(numMoves);
		for (int i=0; i<numMoves; i++)
		{
			balls[i].mask = new unsigned short[img->w*img->h*2];
		}
		filter.resize(numMoves);

		colorManager = new BallColorManager(img);
		contourFinder = new ContourFinder(img);
		ballFitAlgorithm = new BallFitAlgorithm(img);
	}


	BallManager::~BallManager()
	{
		for (int i=0; i<numMoves; i++)
		{
			delete[] balls[i].mask;
		}
		delete colorManager;
		delete contourFinder;
		delete ballFitAlgorithm;
	}

	std::vector<Vec3> BallManager::findBalls()
	{
		std::vector<Vec3> positions;
		colorManager->calculateColors(balls);

		contourFinder->findBalls(balls, numMoves);

		for (int i=0; i<numMoves; i++)
		{
			Vec3 pos;
			if (balls[i].ballFound)
			{
				ballFitAlgorithm->fitCircle(&(balls[i]));

				//TODO: ha a lentit megcsin�lom, nem kell a try
				if (balls[i].ballSize>15) //TODO: && pixelMatches(balls[i].position, balls[i].ballPerceptedColor))
				{
					try
					{
						balls[i].ballPerceptedColor=ColorHsv(img->getPixel(balls[i].position));
					}
					catch(MoveOutOfImageRangeException){}
				}

				pos=calculateRealWorldPosition(balls[i], filter[i]);
			}
			positions.push_back(pos);

			MoveDevice::SetMoveColour(i,balls[i].ballOutColor.r,balls[i].ballOutColor.g,balls[i].ballOutColor.b);
		}
		return positions;
	}

	Vec3 BallManager::calculateRealWorldPosition(MoveBall& ball, Kalman& filter)
	{
		
		if (ball.ballFound)
		{
			float x,y,size;
			x=ball.position.x;
			y=ball.position.y;
			size=ball.ballSize;

			img->drawCircle(Vec2((int)x,(int)y),(int)(size/2),ColorRgb(255,0,255));

			Vec3 position;

			position.z=filter.update(2500/size);
			//position.z=2500/size;
			position.x=(x-((float)img->w/2))*position.z/400;
			position.y=(y-((float)img->h/2))*position.z/-400;

			position.x -= offset.x;
			position.y -= offset.y;
			position.z -= offset.z;
			return position;
		}
		else
			return Vec3::ZERO;

	}

	unsigned char* BallManager::getMaskBuffer(int moveId)
	{
		if (balls.size()<=moveId)
			return 0;
		return (unsigned char*)(balls[moveId].mask);
	}

}