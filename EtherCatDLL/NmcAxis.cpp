// NmcAxis.cpp : implementation file
//
#include "stdafx.h"

#include "NmcDevice.h"
#include "NmcAxis.h"
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNmcAxis


CNmcAxis::CNmcAxis(UINT16 nBoardId)
{
	m_nBoardId = nBoardId;
	m_nOffsetAxisID = 1;
}

CNmcAxis::~CNmcAxis()
{
}

void CNmcAxis::SetParentClassHandle(HWND hwnd)
{
	m_hParentClass = hwnd;
}

BEGIN_MESSAGE_MAP(CNmcAxis, CWnd)
	//{{AFX_MSG_MAP(CNmcAxis)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNmcAxis message handlers
int CNmcAxis::CheckError(INT nErrCode)
{
	char strErrMsg[MAX_PATH]={"",};

	if(nErrCode)
	{
		if (nErrCode != MC_OK)
		{
			CString strMsg;
			strMsg.Format(_T("error code : %d , message : "), nErrCode);
			strMsg += strErrMsg;
			AfxMessageBox(strMsg);
		}
	}
	
	return (int)nErrCode;
}

BOOL CNmcAxis::InitAxis()
{
	double f = pow(2, 31) - 600;   //2018.08.21 kjs
	if (m_stParam.Motor.bType == SERVO_MOTOR)
	{
	}
	else if (m_stParam.Motor.bType == STEPPER)
	{
	}

	//short axes[] = { 0, 1, 2 };
	//short JoyStick[] = { 5, 6, 7 };
	//((CNmcDevice*)FromHandle(m_hParentClass))->SetJoystickEnable(0, 0x00);
	return TRUE;
}

BOOL CNmcAxis::GetAmpEnable()
{
	MC_STATUS ms = MC_OK;
	UINT32 Status = 0;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &Status);
	return ((Status & mcPowerOn) ? TRUE : FALSE);
}

BOOL CNmcAxis::ClearStatus()
{
	MC_STATUS ms = MC_OK;
	ms = MC_Reset(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID);
	if (ms == MC_OK)
		return TRUE;
	return FALSE;
}

BOOL CNmcAxis::IsMotionDone()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if (ms != MC_OK)
		return FALSE;
	if (state ^ mcContinuousMotion && state ^ mcConstantVelocity && state ^ mcAccelerating && state ^ mcDecelerating)
	{
		MC_ReadStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
		if ((state & mcStandStill) > 0)
			return TRUE;
	}
	return FALSE;
}

BOOL CNmcAxis::WaitUntilMotionDone(int mSec)
{
	DWORD CurTimer, StartTimer;
	MSG message;

	StartTimer=GetTickCount();
	CurTimer=GetTickCount();
	ULONGLONG nWaitTick = GetTickCount64();
	while(!IsMotionDone() && mSec > int(CurTimer-StartTimer))
	{
		CurTimer=GetTickCount();
		if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
		Sleep(100);
		int nState = CheckAxisState();

		if (CheckAmpFaultSwitch())
		{
			CmdBufferClear();
			ClearStatus();
			AmpFaultReset();
			ClearStatus();
			return FALSE;
		}
		else if (GetAmpEnable() == 0)
		{
			return FALSE;
		}
		else
		{
			if (CheckAxisDone())
			{
				double dDiffPos = fabs(GetCommandPosition() - GetActualPosition());
				if (dDiffPos > GetInposition())
				{
					if (GetTickCount64() - nWaitTick>=1000)
					{
						CmdBufferClear();
						ClearStatus();
						SetCommandPosition(GetActualPosition());
						return FALSE;
					}
				}
			}
		}

		int nEx = CheckExceptionEvent();
		Sleep(100);
		if (nEx & ST_AMP_FAULT || nEx & ST_ERROR_LIMIT || nEx & ST_AMP_POWER_ONOFF)
		{
			CmdBufferClear();
			ClearStatus();
			return FALSE;
		}


		if (int(CurTimer - StartTimer) > 1)
		{
			if (nEx & ST_INPOSITION_STATUS)
			{
				return TRUE;
			}

			if (int(CurTimer - StartTimer) > 3)
			{
				if (nEx & ST_X_POS_LIMIT || nEx & nEx & ST_X_NEG_LIMIT)
				{
					return FALSE;
				}
			}
		}

		if (nEx & ST_AMP_FAULT || nEx & ST_ERROR_LIMIT)
		{
			return FALSE;
		}
	}

	if(mSec > int(CurTimer-StartTimer))
		return TRUE;

	return FALSE;
}

BOOL CNmcAxis::WaitUntilAxisDone(unsigned int nWaitTime)
{
	DWORD CurTimer, dwStartTimer, dwSettleTimer, dwElapsedTime, dwSettlingTime;

	MSG message;

	double dDiffPos = 0.0;
	BOOL bDone = FALSE;

	dwStartTimer = GetTickCount();
	CurTimer = GetTickCount();

	// Check Positioning error
	if(m_stParam.Motor.bType == STEPPER)	//Stepping motor no encoder
	{
		return WaitUntilMotionDone(nWaitTime);
	}

	dwSettlingTime = 0;
	bDone = TRUE;

	dwSettleTimer = GetTickCount();
	ULONGLONG nTimeTick = GetTickCount64();
	int nRetryCnt = 0;
	do
	{
		
		if (CheckAxisDone())
		{
			dDiffPos = fabs(GetCommandPosition() - GetActualPosition());
			Sleep(100);
			if (dDiffPos > GetInposition())
			{
				if (nRetryCnt >= 10)
				{
					bDone = FALSE;
					break;
				}
				else
				{
					nRetryCnt++;
					dwSettleTimer = GetTickCount(); // reset to dwSettleTimer
					dwSettlingTime = 0;
					Sleep(10);
				}
			}
			else
			{
				bDone = TRUE;
				dwSettlingTime = GetTickCount() - dwSettleTimer;
				return TRUE;
			}
			dwElapsedTime = GetTickCount() - dwStartTimer;

			if (dwElapsedTime > nWaitTime)
			{
				bDone = FALSE;
				break;
			}
		}
		else
		{
			dwSettleTimer = GetTickCount(); // reset to dwSettleTimer
			dwSettlingTime = 0;

			if (GetTickCount64() - nTimeTick >= nWaitTime)
			{
				return FALSE;
			}
		}

		if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}

		int nState = CheckAxisState();

		if (CheckAmpFaultSwitch())
		{
			if (nState == 0)
			{
				StopVelocityMove(0);
			}
			ClearStatus();
			AmpFaultReset();
			ClearStatus();
			return FALSE;
		}
		else if (GetAmpEnable() == 0)
		{
			return FALSE;
		}

		int nEx = CheckExceptionEvent();
		if (nEx & ST_AMP_FAULT || nEx & ST_ERROR_LIMIT || nEx & ST_AMP_POWER_ONOFF)
		{
			return FALSE;
		}

	}while (dwSettlingTime < 1000);

	return bDone;
}

BOOL CNmcAxis::CheckLimitSwitch(int nDir) // PLUS (1), Minus (-1)
{
	INT nState=0;
	if(nDir > 0)
		nState = CheckPosLimitSwitch();
	else
		nState = CheckNegLimitSwitch();
	return nState;
}

BOOL CNmcAxis::CheckPosLimitSwitch()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if (ms != MC_OK)
		return FALSE;
	if (state & mcLimitSwitchPosEvent)
		return TRUE;
	return FALSE;
}

BOOL CNmcAxis::CheckNegLimitSwitch()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if (ms != MC_OK)
		return FALSE;
	if (state & mcLimitSwitchNegEvent)
		return TRUE;
	return FALSE;
}

double CNmcAxis::LenToPulse(double fData)
{
	if(m_stParam.Motor.fPosRes != 0.0)
	{
		fData /= m_stParam.Motor.fPosRes;
		return fData;
	}
	return (double)(INVALIDE_DOUBLE);
}

double CNmcAxis::GetMovingTotalTime(double dLen,double dVel,double dAcc,double dJerk)
{
	if(dLen == INVALIDE_DOUBLE)		return (double)(INVALIDE_DOUBLE);
	if(dVel == INVALIDE_DOUBLE)		dVel = m_stParam.Speed.fVel;
	if(dAcc == INVALIDE_DOUBLE)		dAcc = m_stParam.Speed.fAcc;
	if(dJerk == INVALIDE_DOUBLE)	dJerk = m_stParam.Speed.fMinJerkTime;

	return dAcc/dJerk + dVel/dAcc + dLen/dVel;
}

double CNmcAxis::GetAccTime(double dVel,double dAcc,double dJerk)
{
	if(dVel == INVALIDE_DOUBLE)		dVel = m_stParam.Speed.fVel;
	if(dAcc == INVALIDE_DOUBLE)		dAcc = m_stParam.Speed.fAcc;
	if(dJerk == INVALIDE_DOUBLE)	dJerk = m_stParam.Speed.fMinJerkTime;
	double dAccTime = dVel/dAcc;	// - dAcc/(dJerk*100.0);
	if (dAccTime < 0)		dAccTime *= -1;
	return dAccTime;
}

double CNmcAxis::GetJerkTime(double dAcc,double dJerk)
{
	if(dAcc == INVALIDE_DOUBLE)		dAcc = m_stParam.Speed.fAcc;
	if(dJerk == INVALIDE_DOUBLE)	dJerk = m_stParam.Speed.fMinJerkTime;
	return dAcc/dJerk;
}

double CNmcAxis::GetVelTime(double dLen,double dVel,double dAcc,double dJerk)
{
	if(dLen == INVALIDE_DOUBLE)		return (double)(INVALIDE_DOUBLE);
	if(dVel == INVALIDE_DOUBLE)		dVel = m_stParam.Speed.fVel;
	if(dAcc == INVALIDE_DOUBLE)		dAcc = m_stParam.Speed.fAcc;
	if(dJerk == INVALIDE_DOUBLE)	dJerk = m_stParam.Speed.fMinJerkTime;
	return dLen/dVel - dVel/dAcc - dAcc/dJerk;
}

BOOL CNmcAxis::StartVelocityMove(double fVel, double fAcc)
{
	if(fVel == INVALIDE_DOUBLE)		fVel = m_stParam.Speed.fVel;
	if(fAcc == INVALIDE_DOUBLE)		fAcc = m_stParam.Speed.fAcc;

	if (fabs(fVel) > m_stParam.Speed.fMaxVel)
	{
		AfxMessageBox(_T("Exceed Maximum Speed"));
		return FALSE;
	}
	
	if (!WaitUntilAxisDone(TEN_SECOND))
	{
		return FALSE;
	}

	double dVel, dAcc;
	INT nAccTime;

	MC_STATUS err = MC_OK;
	MC_DIRECTION enDir = mcPositiveDirection;
	if (fVel < 0.0)
	{
		enDir = mcNegativeDirection;
		fVel *= -1.0;
	}

	dVel = LenToPulse(fVel);
	dAcc = LenToPulse(fAcc);
	err = MC_MoveVelocity(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dVel, dAcc, dAcc, 0, enDir, mcAborting);

	if (err != MC_OK)
	{
		if (err & MC_LIMIT_POSITION_OVER)
			return TRUE;
		return FALSE;
	}
	return TRUE;
}

BOOL CNmcAxis::StopVelocityMove(BOOL bWait)
{
	MC_STATUS err = MC_OK;

	err = MC_Halt(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, 500000.0, 0.0, mcAborting);

	if (bWait)
	{
		if (!WaitUntilMotionDone(TEN_SECOND))
			return FALSE;
		Sleep(50);		// prevent Jerk problem at stop. 
		if(!ClearStatus())
			return FALSE;
		Sleep(10);
	}
	return TRUE;
}

void CNmcAxis::SetNegLimitAction(INT nAction)
{
	MC_STATUS ms = MC_OK; 
	ms = MC_WriteBoolParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpEnableHWLimitNeg, (nAction != NO_EVENT));
}

INT CNmcAxis::GetNegLimitAction()
{
	INT nAction = E_STOP_EVENT;
	MC_STATUS ms = MC_OK;
	bool Value;

	ms = MC_ReadBoolParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpEnableHWLimitNeg, &Value);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-GetNegLimitAction()"));
		return nAction;
	}
	if (Value)
		nAction = E_STOP_EVENT;
	else
		nAction = NO_EVENT;

	return nAction;
}

void CNmcAxis::SetPosLimitAction(INT nAction)
{
	MC_STATUS ms = MC_OK;
	ms = MC_WriteBoolParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpEnableHWLimitPos, (nAction != NO_EVENT));
}

INT CNmcAxis::GetPosLimitAction()
{
	INT nAction = E_STOP_EVENT;
	MC_STATUS ms = MC_OK;
	bool Value;
	ms = MC_ReadBoolParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpEnableHWLimitPos, &Value);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-GetNegLimitAction()"));
		return nAction;
	}
	if (Value)
		nAction = E_STOP_EVENT;
	else
		nAction = NO_EVENT;

	return (int)nAction;
}

BOOL CNmcAxis::StartPtPMotion(double fPos, double fVel, double fAcc, double fDec,BOOL bAbs,BOOL bWait)
{
	int error=0;
	MC_STATUS ms = MC_OK;
	CString msg;
	char cstrErrorMsg[MAX_ERR_LEN];


	double dJerkTime = GetAccTime(fVel, fAcc); //0.2; // [sec]

	double dPos = LenToPulse(fPos);
 	double dVel = LenToPulse(fVel);
	double dAcc = LenToPulse(fAcc);
	double dDec = LenToPulse(fDec);
	double dJerk =  LenToPulse(fAcc / dJerkTime);

	if (bAbs)
	{
		// absolute coordinate move
		if (!bWait)
		{
			ms = MC_MoveAbsolute(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcPositiveDirection, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
		else
		{
			ms = MC_MoveAbsolute(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcPositiveDirection, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}

			WaitUntilMotionDone(TEN_SECOND);

		}
	}
	else
	{
		// incremental coordinate move
		if (!bWait)
		{
			ms = MC_MoveRelative(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
		else
		{
			ms = MC_MoveRelative(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}

			WaitUntilMotionDone(TEN_SECOND);

		}
	}

	return TRUE;
}

BOOL CNmcAxis::CheckAxisDone()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;

	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-MC_ReadAxisStatus"));
		return FALSE;
	}

	if (state & mcErrorStop)
	{
		AmpFaultReset();
		ClearStatus();
		return TRUE;
	}
	else if (state & mcStandStill)
		return TRUE;
	else if (CheckMotionDone())
		return TRUE;

	return FALSE;
}

BOOL CNmcAxis::CheckMotionDone()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;

	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if (ms != MC_OK)
		return FALSE;
	if (state ^ mcContinuousMotion && state ^ mcConstantVelocity && state ^ mcAccelerating && state ^ mcDecelerating)
	{
		MC_ReadStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
		if ((state & mcStandStill) > 0)
			return TRUE;
	}

	return FALSE;
}

BOOL CNmcAxis::CheckInposition()
{
	INT nDone = 0;
	nDone = CheckAxisDone();
	return (nDone ? TRUE: FALSE);
}

BOOL CNmcAxis::CheckInMotion()
{
	INT nDone = 0;
	nDone = CheckMotionDone();
	return (nDone ? TRUE:FALSE);
}

double CNmcAxis::GetPosRes()
{
	return m_stParam.Motor.fPosRes;
}

BOOL CNmcAxis::SetAmpEnable(BOOL bOn)
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;

	if(bOn)
	{
		if(GetAmpEnable())
		{
			return TRUE;
		}
	}
	ClearStatus();
	Sleep(10);
	ClearStatus();
	Sleep(10);
	ms = MC_Power(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, bOn);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-SetAmpEnable()"));
		return FALSE;
	}		
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if(ms != MC_OK || state & mcDriveFault)
	{
		AfxMessageBox(_T("Failed Amp Enable !!!"));
		return FALSE;
	}
	if(!GetAmpEnable())
	{
		INT nAction = GetNegLimitAction();
		if(CheckLimitSwitch(-1))
		{
			nAction = GetNegLimitAction();
			SetNegLimitAction(E_STOP_EVENT);//ABORT_EVENT);
			Sleep(10);
			SetNegLimitAction(NO_EVENT);//ABORT_EVENT);
			Sleep(10);
		}
		else if(CheckLimitSwitch(1))
		{
			nAction = GetPosLimitAction();
			SetPosLimitAction(E_STOP_EVENT);//ABORT_EVENT);
			Sleep(10);
			SetPosLimitAction(NO_EVENT);//ABORT_EVENT);
			Sleep(10);
		}
		ClearStatus();
		SetAmpEnable(bOn);

		if(CheckLimitSwitch(-1))
		{
			SetNegLimitAction(nAction);//ABORT_EVENT);
			Sleep(10);
		}
		else if(CheckLimitSwitch(1))
		{
			SetPosLimitAction(nAction);//ABORT_EVENT);
			Sleep(10);
		}
		
		if(!GetAmpEnable())
			return FALSE;
	}
	return TRUE;
}

BOOL CNmcAxis::SetPosition(double fPos)
{	
	MC_STATUS ms = MC_OK;
	int error=0;
	double dPos = LenToPulse(fPos);
	ms = MC_SetPosition(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, false, mcImmediately);
	if (ms == MC_LIMIT_ERROR_PARAM_3)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CNmcAxis::SetEStopRate(int nStopTime) // [mSec]
{
	MC_STATUS ms = MC_OK;
	ms = MC_WriteIntParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpmcpEStopType, IMMEDIATE);
	return ((ms == MC_OK) ? TRUE : FALSE);
}

BOOL CNmcAxis::SetEStop() // [mSec]
{
	MC_STATUS ms = MC_OK;
	ms = MC_WriteIntParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpmcpEStopType, IMMEDIATE);
	return ((ms == MC_OK) ? TRUE : FALSE);
}

BOOL CNmcAxis::SetStopRate(int nStopTime) // [mSec]
{
	MC_STATUS ms = MC_OK;
	ms = MC_WriteIntParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpmcpEStopType, IMMEDIATE);
	return ((ms == MC_OK) ? TRUE : FALSE);
}

BOOL CNmcAxis::SetStop() // [mSec]
{
	MC_STATUS ms = MC_OK;
	ms = MC_WriteIntParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpmcpEStopType, IMMEDIATE);
	return ((ms == MC_OK) ? TRUE : FALSE);
}

BOOL CNmcAxis::SetHomeAction(INT nAction)
{
	MC_STATUS ms = MC_OK;
	UINT32 bitnum = -1;

	switch (nAction)
	{
	case NO_EVENT:
		bitnum = 255;	//Disable
		break;

	case STOP_EVENT:
		bitnum = 2;		//Default Home Limit Bit Number
		break;

	case E_STOP_EVENT:
		bitnum = 2;		//Default Home Limit Bit Number
		break;

	case ABORT_EVENT:
		bitnum = 2;		//Default Home Limit Bit Number
		break;

	default:
		break;
}

#if MACHINE_TYPE == VRS_AUTOMATION
	if (nAction != NO_EVENT)
	{
		return 1;
	}
#endif
	//if(set_home(m_stParam.Motor.nAxisID, (int)nAction) != MMC_OK)
	//	return FALSE;
	ms = MC_WriteIntParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpHomeInputNum, bitnum);
	//return TRUE;
	return ((ms == MC_OK) ? TRUE : FALSE);
}

void CNmcAxis::EnableSwLimit(BOOL bEnable)
{
	if(!bEnable)
	{
		SetPosSoftwareLimit(m_stParam.Motor.fPosLimit, NO_EVENT);
		SetNegSoftwareLimit(m_stParam.Motor.fNegLimit, NO_EVENT);
	}
	else
	{
		SetPosSoftwareLimit(m_stParam.Motor.fPosLimit, E_STOP_EVENT);
		SetNegSoftwareLimit(m_stParam.Motor.fNegLimit, E_STOP_EVENT);
	}
	Sleep(10);
}

BOOL CNmcAxis::StartHomming(int axis)
{
	MC_STATUS ms = MC_OK;

	MSG message;
	CString strTitleMsg = _T(""), strMsg = _T("");
	int nRoof = 0; double dPos = 0.0;
	ULONGLONG nOriginTick = GetTickCount64();

	CmdBufferClear();
	Sleep(100);
	AmpFaultReset();
	Sleep(100);

	if (!GetAmpEnable())
	{
		for (nRoof = 0; nRoof < 10; nRoof++)
		{
			SetAmpEnable(TRUE);
			Sleep(100);
			if (GetAmpEnable())
				break;
		}
		if (!GetAmpEnable())
			return FALSE;
	}
	Sleep(100);
	SetCommandPosition(GetActualPosition());
	Sleep(100);

	EnableSwLimit(FALSE);
	Sleep(100);
	ClearStatus();
	Sleep(100);


	// Step1 Check! Limit Sensor Activated and then Escape the position	
	if(CheckLimitSwitch(m_stParam.Home.bDir))
	{
		Sleep(100);
		SetNegLimitAction(NO_EVENT);
		Sleep(100);
		SetPosLimitAction(NO_EVENT);
		Sleep(100);

		EnableSwLimit(FALSE);
		Sleep(100);
		SetCommandPosition(GetActualPosition());
		Sleep(100);

		AmpFaultReset();
		Sleep(100);
		ClearStatus();
		Sleep(100);

		if (!StartVelocityMove(m_stParam.Home.f1stSpd*(-m_stParam.Home.bDir), m_stParam.Home.fAcc))
		{
			Sleep(100);
			if(CheckAxisDone())
			{
				Sleep(100);
				double dPos = GetActualPosition();
				Sleep(100);
				SetCommandPosition(dPos);
				Sleep(100);

				AmpFaultReset();
				Sleep(100);
				ClearStatus();
				Sleep(100);

				if (!StartVelocityMove(m_stParam.Home.f1stSpd*(-m_stParam.Home.bDir), m_stParam.Home.fAcc))
				{
					Sleep(100);
					AfxMessageBox(_T("SearchHomePos Fail : StartVelocityMove Fail axisdone"));
					return FALSE;
				}
			}
			else
			{
				AfxMessageBox(_T("SearchHomePos Fail : StartVelocityMove Fail"));
				return FALSE;
			}
		}
		Sleep(100);
		EnableSwLimit(FALSE);
		Sleep(100);
		int nRet = 0;
		while(CheckLimitSwitch(m_stParam.Home.bDir))
		{
			if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessage(&message);
			}

			int nState=CheckAxisState();
			if (CheckAmpFaultSwitch())
			{
				SetAmpEnable(FALSE);
				ClearStatus();
				SetAmpEnable(TRUE);
				ClearStatus();

				AmpFaultReset();
				ClearStatus();

				if (GetAmpEnable() == 0)
				{
					SetAmpEnable(1);
				}
				SetCommandPosition(GetActualPosition());

				AmpFaultReset();
				ClearStatus();

				if (!StartVelocityMove(m_stParam.Home.f1stSpd*(-m_stParam.Home.bDir), m_stParam.Home.fAcc))
				{
					if (CheckAxisDone())
					{
						double dPos = GetActualPosition();
						SetCommandPosition(dPos);

						AmpFaultReset();
						ClearStatus();

						if (!StartVelocityMove(m_stParam.Home.f1stSpd*(-m_stParam.Home.bDir), m_stParam.Home.fAcc))
						{

						}
					}
					else
					{

					}
				}

				nRet++;
			}

			if (nRet > 3)
			{
				StopVelocityMove();
				return FALSE;
			}

			if (GetTickCount64() - nOriginTick >= 10000)
			{
				StopVelocityMove();
				return FALSE;
			}

		}
		Sleep(100);
		StopVelocityMove();
		Sleep(100);
	}

	if (!ClearStatus())
	{
		return FALSE;
	}
	if (!WaitUntilMotionDone(TEN_SECOND))
	{
		return FALSE;
	}

	dPos = GetActualPosition();
	SetCommandPosition(dPos);

	if(!GetAmpEnable())	
	{
		SetAmpEnable(TRUE);
		Sleep(100);
	}

	// Step2 Move to Limit Sensor Position
	SetNegLimitAction(E_STOP_EVENT);
	SetPosLimitAction(E_STOP_EVENT);
	EnableSwLimit(FALSE);

	SetHomeAction(E_STOP_EVENT);
	ClearStatus();

	SetHomeAction(NO_EVENT);
	Sleep(100);

	dPos = GetActualPosition();
	SetCommandPosition(dPos);

	EnableSwLimit(FALSE);

	AmpFaultReset();
	ClearStatus();
	EnableSwLimit(FALSE);

	if (!StartVelocityMove(m_stParam.Home.f1stSpd*m_stParam.Home.bDir, m_stParam.Home.fAcc))
	{
		double dPos = GetActualPosition();
		SetCommandPosition(dPos);

		AmpFaultReset();
		ClearStatus();

		if (!StartVelocityMove(m_stParam.Home.f1stSpd*m_stParam.Home.bDir, m_stParam.Home.fAcc))
		{
			return FALSE;
		}
	}
	AmpFaultReset();
	ClearStatus();

	nOriginTick = GetTickCount64();
	while(!CheckLimitSwitch(m_stParam.Home.bDir))
	{
		if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}

		if (GetTickCount64() - nOriginTick >= 120000)
		{
			return FALSE;
		}

		if (CheckAmpFaultSwitch())
		{
			StopVelocityMove();
			AmpFaultReset();
			ClearStatus();
			return FALSE;
		}
	}
	StopVelocityMove();

	AmpFaultReset();
	ClearStatus();

	if (1)
	{
		double dPos = GetActualPosition();
		SetCommandPosition(dPos);
	}

	if(!GetAmpEnable())	
	{
		SetAmpEnable(TRUE);
		Sleep(100);
	}

	SetNegLimitAction(NO_EVENT);
	SetPosLimitAction(NO_EVENT);

	// Step3 Escape from Limit Sensor Position
	SetPosition(0.0);
	EnableSwLimit(FALSE);

	WaitUntilMotionDone(TEN_SECOND);
	Sleep(100);

	AmpFaultReset();
	ClearStatus();

	StartPtPMotion(10.0*(-m_stParam.Home.bDir), m_stParam.Home.f2ndSpd, m_stParam.Home.fAcc, m_stParam.Home.fAcc, ABS, NO_WAIT);
	nOriginTick = GetTickCount64();
	while (!CheckMotionDone())
	{
		if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}

		if (GetTickCount64() - nOriginTick >= 120000)
		{
			return FALSE;
		}
	}

	// Step4 Home Search by Limit Sensor Touch
	if(!GetAmpEnable())
	{
		SetAmpEnable(TRUE);
		Sleep(100);
	}

	SetNegLimitAction(E_STOP_EVENT);
	SetPosLimitAction(E_STOP_EVENT);

	ClearStatus();
	EnableSwLimit(FALSE);

	dPos = GetActualPosition();
	SetCommandPosition(dPos);

	AmpFaultReset();
	ClearStatus();
	EnableSwLimit(FALSE);

	if (!StartVelocityMove(m_stParam.Home.f2ndSpd*m_stParam.Home.bDir, m_stParam.Home.fAcc))
	{
		if (CheckAxisDone())
		{
			dPos = GetActualPosition();
			SetCommandPosition(dPos);

			AmpFaultReset();
			ClearStatus();

			if (!StartVelocityMove(m_stParam.Home.f2ndSpd*m_stParam.Home.bDir, m_stParam.Home.fAcc))
			{
				AfxMessageBox(_T("SearchHomePos Fail : StartVelocityMove Fail axisdone f2ndSpd"));
				return FALSE;
			}
		}
		else
		{
			AfxMessageBox(_T("SearchHomePos Fail : StartVelocityMove Fail f2ndSpd"));
			return FALSE;
		}
	}

	AmpFaultReset();
	ClearStatus();
	nOriginTick = GetTickCount64();
	while (!CheckLimitSwitch(m_stParam.Home.bDir))
	{
		if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}

		if (GetTickCount64() - nOriginTick >= 60000)
		{
			return FALSE;
		}

		int nState = CheckAxisState();
		if (nState == ABORT_EVENT)
		{
			StopVelocityMove();
			AmpFaultReset();
			ClearStatus();
			return FALSE;
		}
	}
	StopVelocityMove();
	dPos = GetActualPosition();
	Sleep(100);
	SetCommandPosition(dPos);
	Sleep(100);

	if(!GetAmpEnable())	
	{
		SetAmpEnable(TRUE);
		Sleep(100);
	}
	SetNegLimitAction(NO_EVENT);//E_STOP_EVENT);
	SetPosLimitAction(NO_EVENT);//E_STOP_EVENT);
	
	Sleep(500);
	SetPosition(0.0);
	Sleep(500);
	
	ClearStatus();   // Event 해제
	EnableSwLimit(FALSE);

	// Step5 Home Shift by Specified Position
	if(m_stParam.Home.fShift != 0.0)
	{
		if(!GetAmpEnable())	
		{
			SetAmpEnable(TRUE);
			Sleep(100);
		}

		WaitUntilMotionDone(TEN_SECOND);
		Sleep(100);

		AmpFaultReset();
		ClearStatus();

		if (!StartPtPMotion(m_stParam.Home.fShift, m_stParam.Home.f2ndSpd, m_stParam.Home.fAcc, m_stParam.Home.fAcc, ABS, WAIT))
		{
			ClearStatus();
			SetAmpEnable(FALSE);
			SetAmpEnable(TRUE);
			ClearStatus();

			WaitUntilMotionDone(TEN_SECOND);
			Sleep(100);

			AmpFaultReset();
			ClearStatus();

			if (!StartPtPMotion(m_stParam.Home.fShift, m_stParam.Home.f2ndSpd, m_stParam.Home.fAcc, m_stParam.Home.fAcc, ABS, WAIT))
			{
				return FALSE;
			}
		}
		nOriginTick = GetTickCount64();
		while (!CheckAxisDone())//while (!pMotion->CheckAxisDone(axis))
		{
			if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
			{
				::TranslateMessage(&message);
				::DispatchMessage(&message);
			}

			if (GetTickCount64() - nOriginTick >= 60000)
			{
				return FALSE;
			}

			int nState = CheckAxisState();
			if (CheckAmpFaultSwitch())
			{
				StopVelocityMove();
				AmpFaultReset();
				ClearStatus();
				return FALSE;
			}
		}

		// Step6 Set Zero Position
		SetPosition(0.0);
	}

	ClearStatus();   // Event 해제
	SetHomeAction(NO_EVENT);
	SetOriginStatus(TRUE);

	EnableSwLimit();
	SetPosLimitAction(E_STOP_EVENT);
	SetNegLimitAction(E_STOP_EVENT);

	if(!GetAmpEnable())	
	{
		SetAmpEnable(TRUE); 
		Sleep(100);
	}
	ClearStatus();

	// Step7 Origin Return Finished
	return TRUE;
}

void CNmcAxis::SetOriginStatus(BOOL bStatus)
{
	m_stParam.Home.bStatus = bStatus;
}

int CNmcAxis::CmdBufferClear()
{
	int nRtn = 0;
	ClearStatus();
	return nRtn;
}

BOOL CNmcAxis::Stop(int nRate)	//For iRate * 10 msec, Stopping.
{
	MC_STATUS ms = MC_OK;
	char cstrErrorMsg[MAX_ERR_LEN];
	TCHAR msg[MAX_ERR_LEN];

	DWORD CurTimer = 0, StartTimer = 0;
	MSG message;
	BOOL bErr = FALSE;

	bErr = SetStop();

	StartTimer=GetTickCount();
	CurTimer=GetTickCount();
	while( !IsMotionDone() && CurTimer-StartTimer < 1000. )
	{
		CurTimer=GetTickCount();
		if(::PeekMessage(&message,NULL,0,0,PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
	}
	if( CurTimer-StartTimer >= 1000.)
	{
		AfxMessageBox(_T("Error Motion Done On Axis Stop\nFor 1000msec!!!"));
	}

	if (!ClearStatus())
		bErr = TRUE;
	if (!SetAmpEnable(TRUE))
		bErr = TRUE;

	ms = MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
	if (ms != MC_OK || bErr)
	{
		_stprintf(msg, _T("Error :: 0x%08X, %hs"), ms, cstrErrorMsg);
		AfxMessageBox(msg);
		return FALSE;
	}
	return TRUE;
}

double CNmcAxis::GetInposition()
{
	MC_STATUS ms = MC_OK;
	double Length;

	ms = MC_ReadParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpInPositionWindowSize, &Length);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-GetInposition()"));
		return 0.0;
	}
	double dInpos = PulseToLen(Length);
	return dInpos;	
}

double CNmcAxis::PulseToLen(double fData)
{
	if(m_stParam.Motor.fPosRes != 0.0)
	{
		fData *= m_stParam.Motor.fPosRes;
		return fData;// /4.0;
	}
	else
		return -1.0;
}

BOOL CNmcAxis::StartSCurveMove(double fPos,double fVel,double fAcc,double fJerk,BOOL bAbs,BOOL bWait)
{
	MC_STATUS ms = MC_OK;
	int error=0;
	CString msg;
	char cstrErrorMsg[MAX_ERR_LEN];

	double dPos = LenToPulse(fPos);
	double dVel = LenToPulse(fVel);
	double dAcc = LenToPulse(fAcc);
	double dJerkTime = GetAccTime(fVel, fAcc, fJerk);
	double dJerk = LenToPulse(fAcc / dJerkTime);

	// symmetrical trapezoidal motion sequence
	if(bAbs)
	{
		// absolute coordinate move
		if(!bWait)
		{
			ms = MC_MoveAbsolute(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dAcc, dJerk, mcPositiveDirection, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
		else
		{
			ms = MC_MoveAbsolute(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dAcc, dJerk, mcPositiveDirection, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
			WaitUntilMotionDone(TEN_SECOND);
		}
	}
	else
	{
		// incremental coordinate move
		if(!bWait)
		{
			ms = MC_MoveRelative(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dAcc, dJerk, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
		else
		{
			ms = MC_MoveRelative(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dAcc, dJerk, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
			WaitUntilMotionDone(TEN_SECOND);
		}
	}
	return ((ms ==MC_OK) ? TRUE : FALSE);
	
	return FALSE;
}

double CNmcAxis::GetActualPosition()
{
	MC_STATUS ms = MC_OK;
	double dPos;
	double dCurPulse = 0.f, dCurPos = 0.f;

	ms = MC_ReadParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpActualPosition, &dPos);

	if(ms != MC_OK)
	{
		AfxMessageBox(_T("Error-GetActualPosition()"));
		return 0.0; 
	}
	dCurPos = PulseToLen(dPos);

	return dCurPos;
}

double CNmcAxis::GetCommandPosition()
{
	MC_STATUS ms = MC_OK;
	double dPos;
	double dCurPulse = 0.f, dCurPos = 0.f;

	ms = MC_ReadParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpCommandedPosition, &dPos);

	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-GetCommandPosition()"));
		return 0.0;
	}
	dCurPos = PulseToLen(dPos);

	return dCurPos;
}

void CNmcAxis::SetCommandPosition(double dPos)
{	
	MC_STATUS ms = MC_OK;
	double dPulse = LenToPulse(dPos);
	//CheckError(error = set_command(m_stParam.Motor.nAxisID, dPulse));

	// CommandPosition is read only on NMC. 
	//ms = MC_WriteParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpCommandedPosition, dPulse);
	//if (ms != MC_OK)
	//{
	//	AfxMessageBox(_T("Error-SetCommandPosition()"));
	//	return;
	//}
}

double CNmcAxis::GetNegSoftwareLimit()
{
	return m_stParam.Motor.fNegLimit;
}

void CNmcAxis::SetNegSoftwareLimit(double fLimitVal, INT nAction)
{
	MC_STATUS ms = MC_OK;

	bool enable = true;

	switch (nAction)
	{
	case NO_EVENT:
		enable = false;
		break;

	case STOP_EVENT:
		enable = true;
		break;

	case E_STOP_EVENT:
		enable = true;
		break;

	case ABORT_EVENT:
		enable = true;
		break;

	default:
		break;
	}

	double dPos = LenToPulse(fLimitVal);

	ms = MC_WriteParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpSWLimitNeg, dPos);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-SetNegSoftwareLimit()"));
		return;
	}

	ms = MC_WriteBoolParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpEnableLimitNeg, enable);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-SetNegSoftwareLimit()"));
		return;
	}

}

double CNmcAxis::GetPosSoftwareLimit()
{
	return m_stParam.Motor.fPosLimit;
}

void CNmcAxis::SetPosSoftwareLimit(double fLimitVal, INT nAction)
{
	MC_STATUS ms = MC_OK;

	bool enable = true;

	switch (nAction)
	{
	case NO_EVENT:
		enable = false;
		break;

	case STOP_EVENT:
		enable = true;
		break;

	case E_STOP_EVENT:
		enable = true;
		break;

	case ABORT_EVENT:
		enable = true;
		break;

	default:
		break;
	}

	double dPos = LenToPulse(fLimitVal);

	ms = MC_WriteParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpSWLimitPos, dPos);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-SetPosSoftwareLimit()"));
		return;
	}

	ms = MC_WriteBoolParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpEnableLimitPos, enable);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Error-SetPosSoftwareLimit()"));
		return;
	}
}

int CNmcAxis::CheckExceptionEvent()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	return ((int)state);
}

int CNmcAxis::CheckAxisState()
{
	int nstate = 0;
	nstate = CheckExceptionEvent();
	return nstate;
}

BOOL CNmcAxis::ControllerIdle()
{
	int error=0;
	return TRUE;
}

INT CNmcAxis::GetStopRate(INT nRate)
{
	int error=0;
	return nRate;
}

BOOL CNmcAxis::SetVelocity(double fVelocity)
{
	MC_STATUS ms = MC_OK;
	double dPulse = LenToPulse(fVelocity);
	return TRUE;
}

BOOL CNmcAxis::CheckHomeSwitch()
{
	return FALSE;

	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;

	INT nState=0;
	INT nLevel=0;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if(ms == MC_OK)
	{
		if (state & mcHomeAbsSwitch)
			return TRUE;
	}
 	return FALSE;
}

BOOL CNmcAxis::StartPtPMove(double fPos, double fVel, double fAcc, double fDec,BOOL bAbs,BOOL bWait)
{
	MC_STATUS ms = MC_OK;
	CString msg;
	char cstrErrorMsg[MAX_ERR_LEN];

	double dJerkTime = GetAccTime(fVel, fAcc); //0.2; // [sec]

	double dPos = LenToPulse(fPos);
	double dVel = LenToPulse(fVel);
	double dAcc = LenToPulse(fAcc);
	double dDec = LenToPulse(fDec);
	double dJerk = LenToPulse(fAcc / dJerkTime);

	if (bAbs)
	{
		// absolute coordinate move
		if (!bWait)
		{
			ms = MC_MoveAbsolute(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcPositiveDirection, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
		else
		{
			ms = MC_MoveAbsolute(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcPositiveDirection, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}

			WaitUntilMotionDone(TEN_SECOND);

		}
	}
	else
	{
		// incremental coordinate move
		if (!bWait)
		{
			ms = MC_MoveRelative(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
		else
		{
			ms = MC_MoveRelative(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, dPos, dVel, dAcc, dDec, dJerk, mcAborting);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
				AfxMessageBox(msg);
				return FALSE;
			}

			WaitUntilMotionDone(TEN_SECOND);

		}
	}

	return TRUE;
}

BOOL CNmcAxis::SetAmpEnableLevel(int nLevel)
{
	return TRUE;
}

BOOL CNmcAxis::ControllerRun()
{
	int error=0;
	return TRUE;
}

int CNmcAxis::AmpFaultReset()
{
	MC_STATUS ms = MC_OK;
	ClearStatus();
	ms = MC_Reset(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID);
	return ((ms == MC_OK) ? TRUE : FALSE);
}

BOOL CNmcAxis::CheckAmpFaultSwitch()
{
	MC_STATUS ms = MC_OK;
	UINT32 state = 0x00000000;

	INT nState = 0;
	INT nLevel = 0;
	ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
	if (ms == MC_OK)
	{
		if (state & mcDriveFault)
		{
			ms = MC_ReadAxisStatus(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, &state);
			if (ms == MC_OK)
			{
				if (state & mcDriveFault)
					return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL CNmcAxis::SetInPosLength(double dLength)
{
	MC_STATUS ms = MC_OK;
	double dLen = LenToPulse(dLength);
	ms = MC_WriteParameter(m_nBoardId, m_stParam.Motor.nAxisID + m_nOffsetAxisID, mcpInPositionWindowSize, dLen);
	return ((ms == MC_OK) ? TRUE : FALSE);
}

/*
In-Position Check Type 설정

0: None (In-Position Check 하지 않음)
1: External Drive (In-Position State를 외부 Drive에서 받음)
2: Internal(In-Position을 내부에서 판단)(Default)
3: External Drive + Internal

*/
BOOL CNmcAxis::SetInPosEnable(int nEnable)
{
	MC_STATUS ms = MC_OK;
	UINT32 enable = nEnable;
	return TRUE;
}


