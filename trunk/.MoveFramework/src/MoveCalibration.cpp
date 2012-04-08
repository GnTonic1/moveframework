#include "MoveCalibration.h"
#include "simplex.h"
#include "IniFile.h"

using namespace std;

namespace Move
{
	MoveCalibration* MoveCalibration::instance = 0;

	MoveCalibration::MoveCalibration(int id, MoveManager* manager)
	{
		this->id=id;
		isCalibrating=false;
		this->manager=manager;

		char fileName[]="settings.cfg";

		MoveDevice::TMoveBluetooth bt;
		MoveDevice::ReadMoveBluetoothSettings(id,&bt);
		//device name
		sprintf_s(deviceName,"Move_%s",bt.MoveBtMacString);

		try
		{
			data.gyroGain=IniFile::GetMat3("gyroGain", deviceName, fileName);
			data.accBias=IniFile::GetVec3("accBias", deviceName, fileName);
			data.accGain=IniFile::GetMat3("accGain", deviceName, fileName);
			data.magBias=IniFile::GetVec3("magBias", deviceName, fileName);
			data.magGain=IniFile::GetVec3("magGain", deviceName, fileName);
			calibrated=true;
		}
		catch(MoveConfigFileRecordNotFoundException)
		{
			calibrated=false;
		}
		magBuf=0;
		rawData=0;
	}

	MoveCalibration::~MoveCalibration(void)
	{
	}

	void MoveCalibration::Update(Vec3 acc, Vec3 gyro, Vec3 mag, float deltat)
	{
		// if there is no phase running
		if (!isCalibrating) return;
		
		if (bufLength<2000 && (skipData--)<=0)
		{
			magBuf[bufLength]=mag;
			bufLength++;
			skipData=SKIPDATA;
		}
	}

	MoveCalibrationData MoveCalibration::getCalibrationData()
	{
		return data;
	}

	//move calibration
	bool MoveCalibration::startCalibration()
	{
		MoveDevice::TMoveCalib calib;
		if (!MoveDevice::ReadMoveCalibration(id,&calib))
			return false;
		rawData=new MoveRawCalibration(calib);

		isCalibrating=true;

		magBuf=new Vec3[2000];

		bufLength=0;

		return true;
	}

	double MoveCalibration::integrateGyroError(std::vector<double> x)
	{
		double error=0.0;
		MoveRawCalibration* rawData=MoveCalibration::instance->rawData;

		Vec3 point;

		Vec3 objective[3];
		objective[0]=Vec3( 2*PI, 0, 0);
		objective[1]=Vec3( 0, 2*PI, 0 );
		objective[2]=Vec3( 0, 0, 2*PI );

		Mat3 gain=Mat3(x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8]);

		for (int i=0; i<3; i++)
		{
			//remove bias
			point=(rawData->GyroVectors[i]-(rawData->GyroBiasVectors[1]*rawData->UnknownVectors[1]));
			//convert from 80 rpm to 1 rotate per second
			point*=0.75f;
			//calculate error vector
			point=point*gain-objective[i];
			//integrate the error
			error+=fabs((double)point.length2());
		}

		return error;
	}

	double MoveCalibration::integrateMagError(std::vector<double> x)
	{
		double error=0.0f;
		Vec3 point;

		Vec3 bias=Vec3(x[0],x[1],x[2]);
		Vec3 gain=Vec3(x[3],x[4],x[5]);

		for (int i=1; i<MoveCalibration::instance->bufLength; i++)
		{
			//calculate calibrated point
			point=(MoveCalibration::instance->magBuf[i]-bias)*gain;
			//integrate the error
			error+=fabs(1.0-(double)point.length2());
		}
		return error/(double)MoveCalibration::instance->bufLength;
	}

	double MoveCalibration::integrateAccError(std::vector<double> x)
	{
		double error=0.0f;
		MoveRawCalibration* rawData=MoveCalibration::instance->rawData;

		Vec3 point;

		Vec3 objective[6];

		objective[0]=Vec3( -9.81, 0, 0 );
		objective[1]=Vec3( 9.81, 0, 0 );
		objective[2]=Vec3( 0, -9.81, 0 );
		objective[3]=Vec3( 0, 9.81, 0 );
		objective[4]=Vec3( 0, 0, -9.81 );
		objective[5]=Vec3( 0, 0, 9.81);

		Mat3 gain=Mat3(x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8]);
		Vec3 bias=Vec3(x[9],x[10],x[11]);

		for (int i=0; i<6; i++)
		{
			//calculate error vector
			point=(rawData->AccVectors[i]-bias)*gain-objective[i];
			//integrate the error
			error+=fabs((double)point.length2());
		}
		return error;
	}


	void MoveCalibration::endCalibration()
	{
		isCalibrating=false;

		MoveCalibration::instance=this;

		std::vector<double> init;
		std::vector<double> result;

		double error1, error2;

		//MAGNETOMETER
		//initialize the algorithm
		Vec3 min, max, bias, gain;
		min=magBuf[0];
		max=magBuf[0];
		for (int i=1; i<bufLength; i++)
		{
			SWAP_MIN(min.x,magBuf[i].x);
			SWAP_MIN(min.y,magBuf[i].y);
			SWAP_MIN(min.z,magBuf[i].z);
			SWAP_MAX(max.x,magBuf[i].x);
			SWAP_MAX(max.y,magBuf[i].y);
			SWAP_MAX(max.z,magBuf[i].z);
		}
		bias=(max+min)*0.5f;
		gain=2.0f/(max-min);

		init.clear();
		init.push_back(bias.x);
		init.push_back(bias.y);
		init.push_back(bias.z);
		init.push_back(gain.x);
		init.push_back(gain.y);
		init.push_back(gain.z);


		result.clear();
		result=BT::Simplex(&Move::MoveCalibration::integrateMagError, init);

		data.magBias=Vec3(result[0],result[1],result[2]);
		data.magGain=Vec3(result[3],result[4],result[5]);


		//ACCELEROMETER
		init.clear();
		init.push_back(0.0022);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0022);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0022);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0001);


		error1=integrateAccError(init);
		result.clear();
		result=BT::Simplex(&Move::MoveCalibration::integrateAccError, init);
		error2=integrateAccError(result);

		data.accGain=Mat3(result[0],result[1],result[2],result[3],result[4],result[5],result[6],result[7],result[8]);
		data.accBias=Vec3(result[9],result[10],result[11]);


		//GYROSCOPE
		init.clear();
		init.push_back(0.0016);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0016);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0001);
		init.push_back(0.0016);

		error1=integrateGyroError(init);
		result.clear();
		result=BT::Simplex(&Move::MoveCalibration::integrateGyroError, init);
		error2=integrateGyroError(result);
		data.gyroGain=Mat3(result[0],result[1],result[2],result[3],result[4],result[5],result[6],result[7],result[8]);



		////save calibration
		IniFile::SetValue("gyroGain", data.gyroGain, deviceName, "settings.cfg");
		IniFile::SetValue("accBias", data.accBias, deviceName, "settings.cfg");
		IniFile::SetValue("accGain", data.accGain, deviceName, "settings.cfg");
		IniFile::SetValue("magBias", data.magBias, deviceName, "settings.cfg");
		IniFile::SetValue("magGain", data.magGain, deviceName, "settings.cfg");
		
		delete[] magBuf;
		delete rawData;
		calibrated=true;
	}
}