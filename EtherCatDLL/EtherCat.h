#pragma once

#include "NmcDevice.h"

#define COEFFICIENTS		10

typedef enum { SERVO = 0, STEP } MOTOR_TYPE;
typedef enum { TRAPEZOIDAL = 0, S_CURVE, PARABOLIC } SpeedProfile;
typedef enum { LOW_SPEED = 0, MIDDLE_SPEED = 1, HIGH_SPEED = 2 } enMotionSpeed;
typedef enum { MINUS = -1, PLUS = 1 } Direction;
typedef enum { PREVIOUS = -1, STOPED = 0, NEXT = 1, SPECIFIED = 2 } enMotionStatus;


class AFX_EXT_CLASS CEtherCat
{
	UINT16	m_nBoardId;
	UINT16	m_nDevIdIoIn;
	UINT16	m_nDevIdIoOut;

public:
	double m_fPosLimit[4]; // Software Positive Limit
	double m_fNegLimit[4]; // Software Negative Limit
	double m_fRatingRpm[4];
	double m_fEncPulse[4];
	double m_fEncMul[4];
	double m_fBallPitch[4];
	double m_fGearRatio[4];
	double m_fPosRes[4];    // [mm]
	double m_fMaxVel[4];	// [mm/s]
	double m_fMaxAcc[4];    // [mm/s2]
	double m_fVel[4];	// [mm/s]
	double m_fAcc[4];    // [mm/s2]
	double m_fMinJerkTime[4];
	double m_fAccPeriod[4]; // [%]
	BOOL   m_bOrigin[4]; // 원점복귀 완료 flag
	BOOL   m_fShift[4]; // 원점 Shift 이동량
	BOOL   m_bSafetyInterlock[4]; // 안전 센서 Interlock Flag
	BOOL   m_bAmpEnable[4]; // Amp Enable flag
	double m_fProfileChange[4]; // Speed Profile Change
	BOOL   m_bUseEncoderSignal[4];	//Encoder Signal Enable or Not

public:
	int AmpFaultReset(int nAxisID);
	CNmcDevice* m_pNmcDevice;

	int m_nFilters[4][COEFFICIENTS]; // Filter Coefficients
	BOOL m_MotorDir[4]; // Motor Dir

	double m_fInpLenth[4]; // Inposition Range [mm]
	unsigned int m_nInpTime[4];

	CEtherCat();
	~CEtherCat();

	BOOL GetJoystickEnable(int nBoardNum, short &bEnable);
	BOOL SetJoystickVelocity(int nBoardNum, short nVel0, short nVel1, short nVel2, short nVel3);
	BOOL SetJoystickEnable(int nBoardNum, short bEnable);
	void SetMotorDir(int lMotor, int lDirection);
	int OutOption(int port, long value);
	int InOption(int port, long *value);
	int In32(int port, long *value);
	int Out32(int port, long value);
	int InitNmcBoard();
	BOOL Exit();
	double CalcAccTime(double fAccRate);
	double GetActualPosition(int nAxisID);
	double GetCommandPosition(int nAxisID);
	void SetCommandPosition(int nAxisID, double dCurPos);
	void SyncPos(int nAxisID);

	void EmergencyStop(int nAxisID);
	void NormalStop(int nAxisID);

	BOOL DisableAmplifier(int nAxisID);
	BOOL EnableAmplifier(int nAxisID);

	int CheckMotionDone(int nAxisID);
	int CheckLimitSwitch(int nAxisID, BOOL bDir);
	int CheckAxisDone(int nAxisID);
	int CheckInMotion(int nAxisID);

	int CheckNegLimitSwitch(int nAxisID);
	int CheckPosLimitSwitch(int nAxisID);
	void SetNegSoftwareLimit(int nAxisID, double fLimitVal, int nAction);
	void SetPosSoftwareLimit(int nAxisID, double fLimitVal, int nAction);
	BOOL ClearStatus(int nAxisID);
	int GetAmpEnable(int nAxisID, int *nState);
	void SetHomeAction(int nAxisID, int nAction);
	void Delay(int nDelayTime);
	int ClearFault(int nAxisID);
	int CheckExceptionEvent(int nAxisID);
	int CheckAxisState(int nAxisID);
	int SetEStopRate(int nAxisID, double fRate);
	BOOL StartVelocityMotion(int nAxisID, double fVel, double fAcc, BOOL bSearchLimit = 0);
	int ControllerIdle(int nAxisID);
	int LenToPulse(int nAxisID, double &fData);
	void SetPosLimitAction(int nAxisID, int nAction);
	void SetNegLimitAction(int nAxisID, int nAction);
	BOOL GetOriginStatus(int nAxisID);
	int SetPIDFilterParam(int nAxisID, int *pCoeffs);
	int SetStopRate(int nAxisID, double fRate);
	int StopVelocityMotion(int nAxisID, BOOL bWait = 1);
	int GetStopRate(int nAxisID, double *fRate);
	int SetVelocity(int nAxisID, double fVelocity);
	int CheckHomeSwitch(int nAxisID);
	int	ReduceVibration(int nAxisID, int nOffset);
	void SetOriginStatus(int nAxisID, BOOL bStatus);
	int SetPosition(int nAxisID, double fPos);
	int StartPtPMotion(int nAxisID, double fPos, double fVel, double fAcc, double fDec, BOOL bAbs = TRUE, BOOL bWait = TRUE);
	int SetAmpEnableLevel(int nAxisID, int nLevel);
	BOOL SetAmpEnable(int nAxisID, int state);
	int PulseToLen(int nAxisID, double &fData);
	double GetPosRes(int nAxisID);
	int InitAxisConfigure(int nAxisID);
	int ControllerRun(int nAxisID);
	CString GetErrorMessage();
	int GetPIDFilterParam(int nAxisID, int *nFilter);
	double GetInposition(int nAxisID);
	double GetSpeedProfile(int nMode, int nAxisID, double fMovingLength, double &fVel, double &fAcc, double &fJerk, int nSpeedType = LOW_SPEED);
	int StartSCurveMotion(int nAxisID, double fPos, double fVel, double fAcc, double fJerk, BOOL bAbs = TRUE, BOOL bWait = TRUE);
	BOOL WaitForMotionDone(int nAxisID);
	BOOL WaitForAxisDone(int nAxisID, unsigned int nWaitTime = 3000);

	double GetSpeedProfileChangeLength(int nAxisID);
	BOOL GetSCurveVelocity(double dLen, double &dVel, double &dAcc, double &dJerk);
	double GetMotionTime(double dLen, double dVel, double dAcc, double dJerk);
	BOOL SearchHomePos(int axis);
	int CheckAmpFaultSwitch(int nAxisID);
	int ClearCommandBuffer(int nAxisID);
	void GetSoftwareLimit(int nAxisID, double &fPosLimit, double &fNegLimit);
};