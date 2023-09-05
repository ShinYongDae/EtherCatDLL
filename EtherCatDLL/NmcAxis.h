#pragma once

// NmcAxis.h : header file
//

#define TEN_SECOND		10000		 // Wait for max 10 minute.
#define TEN_MINUTE		600000		 // Wait for max 10 minute.
#define INVALIDE_DOUBLE	0xFFFFFFFFFFFFFFFF


#ifdef _WIN64
#include "Device/NMC/x64/NMC_Motion.h"
//#pragma comment (lib, "Device/NMC/x64/NMC_Comm.lib")
//#pragma comment (lib, "Device/NMC/x64/NMC_Driver.lib")
#pragma comment (lib, "Device/NMC/x64/NMC_Motion.lib")
#else
#include "Device/MMCE/x86/NMC_Motion.h"
//#pragma comment (lib, "Device/NMC/x86/NMC_Comm.lib")
//#pragma comment (lib, "Device/NMC/x86/NMC_Driver.lib")
#pragma comment (lib, "Device/NMC/x86/NMC_Motion.lib")
#endif

#undef POSITIVE
#undef NEGATIVE

#define MOTION_POSITIVE 1
#define MOTION_NEGATIVE	1

#include "NMC.h"


typedef struct MotorParam {
	BOOL			bType;		// Motor Type 0: servo 1: stepper
	BOOL			bSafety;	// Safety Flag
	double			fRatingSpeed;	// Position
	double			fLeadPitch;		// ex) Ball Screw Lead Pitch
	int				nEncPulse;		// Encoder Pulse 
	int				nEncMul;		// Encoder Multiplier 
	double			fGearRatio;		// Gear Ratio
	double			fMaxAccel;		// Maximum Acceleration in G
	double			fMinJerkTime;	// Minimum Jerk Time in Sec
	double			fInpRange;	// Inposition Range [mm]
	unsigned int	nInpTime;	//	Inposition Time [msec]
	double			fPosLimit;  // Positive Software Limit
	double			fNegLimit;  // Negative Software Limit
	int				nVibOffset; // Vibration offset
}structMotorParam;

typedef struct HomeParam {
	BOOL	bDir;//Initial Home Dir, TRUE:plus-dir	FALSE:minus-dir
	double	f1stSpd;// PreHomming Speed
	double	f2ndSpd;	// Homing Speed
	double	fAcc;		// Homing Accelation
	double	fShift;	// shift
	double	fOffset;// offset
	double	fMargin;	// Margin between Origin & index After Homing
}structHomeParam;

typedef struct SpeedParam {
	double	fSpd;		// Speed
	double	fAcc;		// Acceleration
	double	fDec;		// Deceleration
	double  fAccPeriod; // each acceleration period
	double	fJogFastSpd;// Speed
	double	fJogMidSpd;	// Speed
	double	fJogLowSpd;	// Speed
	double	fJogAcc;	// Acceleration
	BOOL	bJogDir;	// Jog Dir	//2012.07.17 hyk
}structSpeedParam;

typedef struct IOParam {
	BOOL	bPosLimit;	// 정방향 Signal Level of Limit Switch
	BOOL	bNegLimit;	// 역방향 Signal Level of Limit Switch
	BOOL	bOrg;		// Signal Level of Original Switch
	BOOL	bAmpFault;	// Signal Level of Amp Fault
	BOOL	bAmpEnable;	// Signal Level of Amp Enable
}structIOParam;

typedef struct GainParam {
	short	nP;		// P Gain
	short	nI;		// I Gain
	short	nD;		// D Gain
	short	nAccelFF;	// Acceleration Feed Forward
	short	nVelFF;		// Velocity Feed Forward
	short	nILimit;	// Integral Limit
	short	nOffset;	// Command Offset
	short	nDACLimit;	// DAC Limit
	short	nShift;		// Gain Shift
	short	nFrictFF;	// Friction Feed Forward
}structGainParam;

typedef struct MotionParam {
	structMotorParam Motor;
	structHomeParam Home;
	structSpeedParam Speed;
	structIOParam IO;
	structGainParam Gain;
}structMotionParam;

typedef enum { INC = FALSE, ABS = TRUE } Coordinate;
typedef enum { NO_WAIT = FALSE, WAIT = TRUE } MotionWait;


/////////////////////////////////////////////////////////////////////////////
// CNmcAxis window

class CNmcAxis : public CWnd
{
	UINT16	m_nBoardId;
	INT		m_nOffsetAxisID;

	int CheckError(INT nErrCode);

// Construction
public:
	CNmcAxis(UINT16 nBoardId);

// Attributes
public:

	typedef struct MOTOR
	{
		INT		nAxisID;		// Number of Axis : 0 ~ 7
		BOOL	bType;			// Motor Type 0: servo 1: stepper
		BOOL    bSafety;		// Safety Flag
		double	fLeadPitch;		// ex) Ball Screw Lead Pitch
		INT		nEncPulse;		// Encoder Pulse [pulse/rev]
		INT		nEncMul;		// Encoder Multiplier : encoder_ratio = A/B
		double	fGearRatio;		// Gear Ratio
		double	fInpRange;		// Inposition Range [counts]
		unsigned int nInpTime;	// Inposition Time [msec]
		double  fPosLimit;		// Positive Software Limit
		double  fNegLimit;		// Negative Software Limit
		INT		nVibOffset;		// Vibration offset
		INT		nPulseMode;		// 0:CW/CCW, 1:SIGN/PULSE
		INT		nCordDir;		// 0:CORD_CW, 1:CORD_CCW
		INT		nEncoDir;		// 0:ENCODER_CW (-), 1:ENCODER_CCW (+) 
		INT		nEncRatioA;		// encoder_ratio = A/B
		INT		nEncRatioB;		// encoder_ratio = A/B
		double	fPosRes;		// [mm]
	}stMotor;

	typedef struct HOME
	{
		BOOL	bStatus;
		BOOL	bDir;		//Initial Home Dir, TRUE:plus-dir	FALSE:minus-dir
		double	f1stSpd;	// PreHomming Speed
		double	f2ndSpd;	// Homming Speed
		double	fAcc;		// Homming Accel
		double	fShift;		// shift
		double	fOffset;	// offset
		double	fMargin;	// Margin between Origin & index After Homming
	}stHome;

	typedef struct SPEED
	{
		double  fMaxVel;		// [mm/s]
		double	fRatingRPM;		// Position
		double	fMinJerkTime;	// Minimum Jerk Time in Sec
		double	fMaxAcc;		// Maximum Acceleration in G
		double	fVel;			// Speed
		double	fAcc;			// Acceleration
		double	fDec;			// Deceleration
		double  fAccPeriod;		// each acceleration period
		double	fJogFastSpd;	// Speed
		double	fJogMidSpd;		// Speed
		double	fJogLowSpd;		// Speed
		double	fJogAcc;		// Acceleration
	}stSpeed;

	typedef struct IO
	{
		BOOL	bPosLimit;		// 정방향 리미트 스위치 신호 레벨
		BOOL	bNegLimit;		// 역방향 리미트 스위치 신호 레벨
		BOOL	bOrg;			// 원점 스위치 신호 레벨
		BOOL	bAmpFault;		// Amp Fault 신호 레벨
		BOOL	bAmpEnable;		// Amp Enable 신호 레벨
		BOOL	bAmpReset;		// Amp Reset 신호 레벨
		BOOL	bInp;			// Inposition 신호 레벨
	}stIO;

	typedef struct GAIN
	{
		INT	nP;			// P Gain
		INT	nI;			// I Gain
		INT	nD;			// D Gain
		INT	nAccelFF;	// Acceleration Feed Forward
		INT	nVelFF;		// Velocity Feed Forward
		INT	nILimit;	// Integral Limit
		INT	nOffset;	// Command Offset
		INT	nDACLimit;	// DAC Limit
		INT	nShift;		// Gain Shift
		INT	nFrictFF;	// Friction Feed Forward
	}stGain;

	typedef struct PARAM
	{
		stMotor Motor;
		stHome Home;
		stSpeed Speed;
		stIO Io;
		stGain Gain;
	}stParam;

	stParam m_stParam;

	HWND m_hParentClass;

// Operations
public:
	BOOL SetStop();
	BOOL SetInPosLength(double dLength);
	BOOL SetInPosEnable(int nEnable);


	void SetCommandPosition(double dPos);
	BOOL InitAxis();
	BOOL GetAmpEnable();
	BOOL ClearStatus();
	BOOL IsMotionDone();
	BOOL WaitUntilMotionDone(int mSec= TEN_SECOND);
	BOOL CheckLimitSwitch(int nDir); // PLUS (1), Minus (-1)
	BOOL CheckPosLimitSwitch();
	BOOL CheckNegLimitSwitch();
	double GetJerkTime(double dAcc=INVALIDE_DOUBLE, double dJerk=INVALIDE_DOUBLE);
	double GetAccTime(double dVel=INVALIDE_DOUBLE, double dAcc=INVALIDE_DOUBLE, double dJerk=INVALIDE_DOUBLE);
	double GetVelTime(double dLen=INVALIDE_DOUBLE, double dVel=INVALIDE_DOUBLE, double dAcc=INVALIDE_DOUBLE, double dJerk=INVALIDE_DOUBLE);
	double GetMovingTotalTime(double dLen=INVALIDE_DOUBLE, double dVel=INVALIDE_DOUBLE, double dAcc=INVALIDE_DOUBLE, double dJerk=INVALIDE_DOUBLE);
	double LenToPulse(double fData);
	BOOL StopVelocityMove(BOOL bWait=1);
	BOOL StartVelocityMove(double fVel=INVALIDE_DOUBLE, double fAcc=INVALIDE_DOUBLE);
	void SetNegLimitAction(INT nAction);
	void SetPosLimitAction(INT nAction);
	INT GetNegLimitAction();
	INT GetPosLimitAction();
	BOOL StartPtPMotion(double fPos,double fVel,double fAcc,double fDec,BOOL bAbs=TRUE,BOOL bWait=TRUE);
	BOOL CheckMotionDone();
	BOOL CheckInposition();

	BOOL CheckInMotion();
	BOOL CheckAxisDone();
	double GetPosRes();
	BOOL SetAmpEnable(BOOL bOn);
	BOOL SetEStopRate(int nStopTime); // [mSec]
	BOOL SetEStop();
	BOOL SetHomeAction(INT nAction);
	void SetOriginStatus(BOOL bStatus = FALSE);
	BOOL SetPosition(double fPos);
	BOOL StartHomming(int axis);
	void EnableSwLimit(BOOL bEnable=TRUE);
	int CmdBufferClear();
	BOOL Stop(int nRate=10);	//For iRate * 10 msec, Stopping.
	double PulseToLen(double fData);
	double GetInposition();
	BOOL StartSCurveMove(double fPos,double fVel,double fAcc,double fJerk,BOOL bAbs=TRUE,BOOL bWait=TRUE);
	double GetNegSoftwareLimit();
	double GetPosSoftwareLimit();
	void SetNegSoftwareLimit(double fLimitVal, INT nAction);
	void SetPosSoftwareLimit(double fLimitVal, INT nAction);
	int CheckExceptionEvent();
	BOOL SetStopRate(int nStopTime); // [mSec]
	int CheckAxisState();
	BOOL ControllerIdle();
	INT GetStopRate(INT nRate);
	BOOL SetVelocity(double fVelocity);
	BOOL CheckHomeSwitch();
	BOOL StartPtPMove(double fPos, double fVel, double fAcc, double fDec,BOOL bAbs,BOOL bWait);
	BOOL SetAmpEnableLevel(int nLevel);
	BOOL ControllerRun();
	void SetParentClassHandle(HWND hwnd);
	BOOL CheckAmpFaultSwitch();

	double GetActualPosition();
	double GetCommandPosition();
	BOOL WaitUntilAxisDone(unsigned int mSec= TEN_SECOND);



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNmcAxis)
	//}}AFX_VIRTUAL

// Implementation
public:
	int AmpFaultReset();
	virtual ~CNmcAxis();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNmcAxis)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


