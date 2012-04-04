#pragma once

#include "MoveCalibration.h"
#include "MadgwickAHRS.h"
#include "MoveIncludes.h"

namespace Move
{
	class MoveCalibration;

	class MoveOrientation
	{
		Quat orientation;
		Vec3 angularVel;
		Vec3 angularAcc;

		MoveCalibration* calibration;

		bool useMagnetometer;

		Madgwick::AHRS ahrs;

		CRITICAL_SECTION criticalSection;

		float AEq_1, AEq_2, AEq_3, AEq_4;  // Quat orientation of earth frame relative to auxiliary frame

	public:
		MoveOrientation(MoveCalibration* calib);
		~MoveOrientation(void);
		void Update(Vec3 acc, Vec3 gyro, Vec3 mag, float deltat);
		Quat GetOrientation();
		Vec3 GetAngularVelocity();
		Vec3 GetAngularAcceleration();
		void UseMagnetometer(bool value);
		void Reset();
	};
}
