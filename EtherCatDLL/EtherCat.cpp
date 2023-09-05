// EtherCat.cpp: implementation of the CMotionNew class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "EtherCat.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEtherCat::CEtherCat()
{
	m_nBoardId = 1;
	m_nDevIdIoIn = 1000;
	m_nDevIdIoOut = 2000;

	m_pNmcDevice = NULL;
	m_pNmcDevice = new CNmcDevice(m_nBoardId, m_nDevIdIoIn, m_nDevIdIoOut);
	if (!m_pNmcDevice)
	{
		AfxMessageBox(_T("No Allocate MMC Device."));
		delete m_pNmcDevice;
		m_pNmcDevice = NULL;
		return;
	}
}

CEtherCat::~CEtherCat()
{
	if (!Exit())
	{
		AfxMessageBox(_T("Not free - CMotionNew Class."));
	}
}

double CEtherCat::GetSpeedProfileChangeLength(int nAxisID)
{
	double fSpeed = 0.f, fAcc = 0.f, fJerk = 0.f;
	double fT = 0.f, fS = 0.f, fSCurve = 0.f, fTrapezoidal = 0.f;
	double fLength = 0.0;

	for (fLength = 0.01; fLength < 1000; fLength++)
	{
		fSCurve = GetSpeedProfile(S_CURVE, nAxisID, fLength, fSpeed, fAcc, fJerk, HIGH_SPEED);
		fTrapezoidal = GetSpeedProfile(TRAPEZOIDAL, nAxisID, fLength, fSpeed, fAcc, fJerk, HIGH_SPEED);

		if (fSCurve < fTrapezoidal)
			break;
	}
	return fLength;
}

double CEtherCat::GetActualPosition(int nAxisID)
{
	double dCurPos;
	dCurPos = m_pNmcDevice->GetAxis(nAxisID)->GetActualPosition();
	return dCurPos;
}

double CEtherCat::GetCommandPosition(int nAxisID)
{
	double dCurPos;
	dCurPos = m_pNmcDevice->GetAxis(nAxisID)->GetCommandPosition();
	return dCurPos;
}

void CEtherCat::SetCommandPosition(int nAxisID, double dCurPos)
{
	m_pNmcDevice->GetAxis(nAxisID)->SetCommandPosition(dCurPos);
}

BOOL CEtherCat::EnableAmplifier(int nAxisID)
{
	BOOL bRtn = FALSE;

	ClearCommandBuffer(nAxisID);
	AmpFaultReset(nAxisID);
	ClearStatus(nAxisID);
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->SetAmpEnable(TRUE);
	Delay(100);
	SetCommandPosition(nAxisID, GetActualPosition(nAxisID));
	ClearStatus(nAxisID);
	Sleep(100);

	m_bAmpEnable[nAxisID] = TRUE;
	return bRtn;
}

BOOL CEtherCat::DisableAmplifier(int nAxisID)
{
	//// 0. 현재의 모션이 종료되기를 기다린다.
	//	WaitForAxisDone(nAxisID);	
	ClearCommandBuffer(nAxisID);
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->SetAmpEnable(FALSE);
	m_bAmpEnable[nAxisID] = FALSE;

	return bRtn;
}

BOOL CEtherCat::WaitForMotionDone(int nAxisID)
{
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->WaitUntilMotionDone();
	return bRtn;
}

BOOL CEtherCat::WaitForAxisDone(int nAxisID, unsigned int nWaitTime)
{
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->WaitUntilAxisDone(nWaitTime);
	return bRtn;
}

int CEtherCat::CheckMotionDone(int nAxisID)
{
	int nDone = 0;
	nDone = m_pNmcDevice->GetAxis(nAxisID)->CheckMotionDone();
	return nDone;
}

int CEtherCat::CheckLimitSwitch(int nAxisID, BOOL bDir)
{
	if (bDir == PLUS)
		return CheckPosLimitSwitch(nAxisID);
	else
		return CheckNegLimitSwitch(nAxisID);
}

int CEtherCat::CheckAxisDone(int nAxisID)
{
	int nDone = 0;
	nDone = m_pNmcDevice->GetAxis(nAxisID)->CheckAxisDone();
	return nDone;
}

int CEtherCat::CheckInMotion(int nAxisID)
{
	int nDone = 0;
	nDone = m_pNmcDevice->GetAxis(nAxisID)->CheckInMotion();
	return nDone;
}

int CEtherCat::CheckNegLimitSwitch(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->CheckNegLimitSwitch();
	return nRtn;
}

int CEtherCat::CheckPosLimitSwitch(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->CheckPosLimitSwitch();
	return nRtn;
}

void CEtherCat::SetNegSoftwareLimit(int nAxisID, double fLimitVal, int nAction)
{
	INT nAct = INT(nAction);
	m_fNegLimit[nAxisID] = fLimitVal;
	m_pNmcDevice->GetAxis(nAxisID)->SetNegSoftwareLimit(fLimitVal, nAct);
}

void CEtherCat::SetPosSoftwareLimit(int nAxisID, double fLimitVal, int nAction)
{
	INT nAct = INT(nAction);
	m_fPosLimit[nAxisID] = fLimitVal;
	m_pNmcDevice->GetAxis(nAxisID)->SetPosSoftwareLimit(fLimitVal, nAct);
}

BOOL CEtherCat::ClearStatus(int nAxisID)
{
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->ClearStatus();
	return bRtn;
}

int CEtherCat::GetAmpEnable(int nAxisID, int *nState)
{
	*nState = m_pNmcDevice->GetAxis(nAxisID)->GetAmpEnable();
	return MC_OK;
}

void CEtherCat::SetHomeAction(int nAxisID, int nAction)
{
	INT nAct = (INT)nAction;
	m_pNmcDevice->GetAxis(nAxisID)->SetHomeAction(nAct);
}

void CEtherCat::Delay(int nDelayTime)
{
	Sleep(nDelayTime);
}

int CEtherCat::ClearFault(int nAxisID)
{
	int nEventID = CheckExceptionEvent(nAxisID);

	if (nEventID == ST_NONE) // 0, No event has occured
		return TRUE;
	if (nEventID & ST_HOME_SWITCH) // 1,Home logic was activated
	{
		SetHomeAction(nAxisID, NO_EVENT);
	}
	if (nEventID & ST_COLLISION_STATE)
	{
		ClearStatus(nAxisID);
		MC_STATUS ms = MC_OK;
		ms = MC_Reset(m_nBoardId, nAxisID);
	}

	if (nEventID & ST_POS_LIMIT) // 2,positive limit input was activated
	{
		ClearStatus(nAxisID);
	}
	if (nEventID & ST_NEG_LIMIT) // 3,negative limit input was activated
	{
		ClearStatus(nAxisID);
	}
	if (nEventID & ST_AMP_FAULT) // 4,Amplifier fault input was activated
	{
		return TRUE;
	}
	if (nEventID & ST_X_NEG_LIMIT) // 7,Software negative travel limit exceeded
	{
		ClearStatus(nAxisID);
	}
	if (nEventID & ST_X_POS_LIMIT) // 8,Software positive travel limit exceeded
	{
		ClearStatus(nAxisID);
	}
	if (nEventID & ST_ERROR_LIMIT) // 9,Software position error exceeded
	{
		ClearStatus(nAxisID);
	}
	if (nEventID & ST_PC_COMMAND) // 10,CPU generated stop,e-stop,abort,clear status
	{
	}
	if (nEventID & ST_OUT_OF_FRAMES) // 11,Attempted next frame with no frame present
	{
		return TRUE;
	}

	if (nEventID & ST_AMP_POWER_ONOFF) // 12,Attempted next frame with no frame present
	{
		ClearStatus(nAxisID);
		return TRUE;
	}

	if (nEventID & ST_ABS_COMM_ERROR) // 13,Attempted next frame with no frame present
	{
		return TRUE;
	}

	if (nEventID & ST_ABS_COMM_ERROR) // 14,Attempted next frame with no frame present
	{
		return TRUE;
	}

	if (nEventID & ST_INPOSITION_STATUS) // 15,Attempted next frame with no frame present
	{
		return TRUE;
	}

	if (nEventID & ST_RUN_STOP_COMMAND) // 14,Attempted next frame with no frame present
	{
		ClearStatus(nAxisID);
		return TRUE;
	}
	return TRUE;
}

int CEtherCat::CheckExceptionEvent(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->CheckExceptionEvent();
	return nRtn;
}

int CEtherCat::CheckAxisState(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->CheckAxisState();
	return nRtn;
}

int CEtherCat::SetEStopRate(int nAxisID, double fRate)
{
	m_pNmcDevice->GetAxis(nAxisID)->SetEStopRate((int)(fRate));
	return MC_OK;
}

double CEtherCat::CalcAccTime(double fAccRate)
{
	double fAccTime = (1.0 / sqrt(fAccRate)) * 1000.0; // Sec -> mSec -> 1000.0
	return fAccTime;
}

BOOL CEtherCat::StartVelocityMotion(int nAxisID, double fVel, double fAcc, BOOL bSearchLimit)
{
	MC_STATUS ms = MC_OK;
	ms = MC_Reset(m_nBoardId, nAxisID);

	BOOL bSWLimit = 0;
	if ((ST_POS_LIMIT & CheckExceptionEvent(nAxisID)) || (CheckExceptionEvent(nAxisID) & ST_NEG_LIMIT) || (CheckExceptionEvent(nAxisID) & ST_X_NEG_LIMIT) || (CheckExceptionEvent(nAxisID) & ST_X_POS_LIMIT))
	{
		m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(FALSE);
		bSWLimit = 1;
		ClearStatus(nAxisID);
	}


	if (CheckAxisState(nAxisID) != ST_NONE)
	{
		int nEventID = CheckExceptionEvent(nAxisID);

		if (nEventID & ST_COLLISION_STATE)
		{
			ClearStatus(nAxisID);
			MC_STATUS ms = MC_OK;
			ms = MC_Reset(m_nBoardId, nAxisID);
		}
		if (nEventID& ST_X_POS_LIMIT || nEventID& ST_X_NEG_LIMIT || nEventID& ST_POS_LIMIT || nEventID & ST_NEG_LIMIT || nEventID &ST_AMP_FAULT)
		{
			m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(FALSE);
			int axisStatus = 0;
			ClearStatus(nAxisID);
			ClearFault(nAxisID); // Command 값을 Actual 값? ?치시켜 error값을 0로 ?? 
			GetAmpEnable(nAxisID, &axisStatus);
			if (!axisStatus)
				EnableAmplifier(nAxisID);

			if (CheckAmpFaultSwitch(nAxisID))
			{
				if (bSearchLimit == 0)
				{
					if (bSWLimit)
						m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(1);
				}
				return 0;
			}

		}
	}

	SyncPos(nAxisID);

	int AmpState = 0;
	GetAmpEnable(nAxisID, &AmpState);

	if (AmpState != TRUE)
	{
		EnableAmplifier(nAxisID);

		if (AmpState != TRUE)
		{
			if (bSearchLimit == 0)
			{
				if (bSWLimit)
					m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(1);
			}
		}
		return FALSE;
	}

	if (fabs(fVel) > m_fMaxVel[nAxisID])
	{
		if (bSearchLimit == 0)
		{
			if (bSWLimit)
				m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(1);
		}
		return FALSE;
	}


	if (m_bSafetyInterlock[nAxisID])
		SetHomeAction(nAxisID, E_STOP_EVENT);

	if (!m_pNmcDevice->GetAxis(nAxisID)->StartVelocityMove(fVel, fAcc))
	{
		if (CheckAxisDone(nAxisID))
		{

		}

		if (bSearchLimit == 0)
		{
			if (bSWLimit)
				m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(1);
		}
		return FALSE;
	}
	if (bSearchLimit == 0)
	{
		if (bSWLimit)
		{
			Sleep(30);
			m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(1);
		}
	}
	return TRUE;
}

int CEtherCat::ControllerIdle(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->ControllerIdle();
	return nRtn;
}

int CEtherCat::LenToPulse(int nAxisID, double &fData)
{
	fData = m_pNmcDevice->GetAxis(nAxisID)->LenToPulse(fData);
	return MC_OK;
}

void CEtherCat::SetPosLimitAction(int nAxisID, int nAction)
{
	INT nAct = (INT)nAction;
	m_pNmcDevice->GetAxis(nAxisID)->SetPosLimitAction(nAct);
}

void CEtherCat::SetNegLimitAction(int nAxisID, int nAction)
{
	INT nAct = (INT)nAction;
	m_pNmcDevice->GetAxis(nAxisID)->SetNegLimitAction(nAct);
}

BOOL CEtherCat::GetOriginStatus(int nAxisID)
{
	return m_bOrigin[nAxisID];
}

int CEtherCat::SetPIDFilterParam(int nAxisID, int *pCoeffs)
{
	return MC_OK;
}

int CEtherCat::SetStopRate(int nAxisID, double fRate)
{
	int nRtn = 0;
	PulseToLen(nAxisID, fRate);
	double fRateTime = CalcAccTime(fRate);
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->SetStopRate((int)fRateTime);
	return nRtn;
}

int CEtherCat::StopVelocityMotion(int nAxisID, BOOL bWait)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->StopVelocityMove(bWait);
	return nRtn;
}

int CEtherCat::GetStopRate(int nAxisID, double *fRate)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->GetStopRate((int)*fRate);
	return nRtn;
}

int CEtherCat::SetVelocity(int nAxisID, double fVelocity)
{
	return MC_OK;
}

int CEtherCat::CheckHomeSwitch(int nAxisID)
{
	int nRtn = MC_OK;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->CheckHomeSwitch();
	return nRtn;
}

int CEtherCat::ReduceVibration(int nAxisID, int nOffset)
{
	return TRUE;
}

int CEtherCat::GetPIDFilterParam(int nAxisID, int *nFilter)
{
	return MC_OK;
}

void CEtherCat::SetOriginStatus(int nAxisID, BOOL bStatus)
{
	m_bOrigin[nAxisID] = bStatus;
}

int CEtherCat::SetPosition(int nAxisID, double fPos)
{
	int nRtn = MC_OK;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->SetPosition(fPos);
	return nRtn;
}

int CEtherCat::StartPtPMotion(int nAxisID, double fPos, double fVel, double fAcc, double fDec, BOOL bAbs, BOOL bWait)
{
	BOOL bRtn = TRUE;
	double 	fTgtPos;
	if (m_bSafetyInterlock[nAxisID])
		SetHomeAction(nAxisID, E_STOP_EVENT);


	if (CheckAxisState(nAxisID) != ST_NONE)
	{
		int nEventID = CheckExceptionEvent(nAxisID);
		if (nEventID & ST_INPOSITION_STATUS)
			nEventID ^= ST_INPOSITION_STATUS;
		if (nEventID & ST_COLLISION_STATE)
		{
			ClearStatus(nAxisID);
			MC_STATUS ms = MC_OK;
			ms = MC_Reset(m_nBoardId, nAxisID);
		}
		if (nEventID& ST_POS_LIMIT || nEventID & ST_NEG_LIMIT || nEventID &ST_AMP_FAULT || nEventID &ST_X_NEG_LIMIT || nEventID &ST_X_POS_LIMIT)
		{
			m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(FALSE);

			BOOL bSWLimit = 1;
			ClearStatus(nAxisID);

			int axisStatus = 0;
			ClearStatus(nAxisID);
			ClearFault(nAxisID); // Command 값을 Actual 값? ?치시켜 error값을 0로 ?? 
			GetAmpEnable(nAxisID, &axisStatus);

			if (!axisStatus)
				EnableAmplifier(nAxisID);

			if (CheckAmpFaultSwitch(nAxisID))
			{
				return 0;
			}

		}
	}
	int AmpState = 0;
	GetAmpEnable(nAxisID, &AmpState);

	if (AmpState == 0)
		EnableAmplifier(nAxisID);

	SyncPos(nAxisID);

	fTgtPos = min(max(fPos, m_pNmcDevice->GetAxis(nAxisID)->m_stParam.Motor.fNegLimit), m_pNmcDevice->GetAxis(nAxisID)->m_stParam.Motor.fPosLimit);
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->StartPtPMove(fPos, fVel, fAcc, fDec, bAbs, bWait);
	return int(bRtn ? 1 : 0);
}

int CEtherCat::SetAmpEnableLevel(int nAxisID, int nLevel)
{
	int nRtn = MC_OK;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->SetAmpEnableLevel(nLevel);
	return nRtn;
}

BOOL CEtherCat::SetAmpEnable(int nAxisID, int state)
{
	BOOL bRtn = FALSE;
	BOOL bState = state ? TRUE : FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->SetAmpEnable(bState);
	return bRtn;
}

BOOL CEtherCat::PulseToLen(int nAxisID, double &fData)
{
	double dData = 0.0;
	dData = m_pNmcDevice->GetAxis(nAxisID)->PulseToLen(fData);
	fData = dData;
	return TRUE;
}

double CEtherCat::GetPosRes(int nAxisID)
{
	return m_fPosRes[nAxisID];
}

BOOL CEtherCat::InitAxisConfigure(int nAxisID)
{
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->InitAxis();
	return bRtn;
}

int CEtherCat::ControllerRun(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->ControllerRun();
	return nRtn;
}

CString CEtherCat::GetErrorMessage()
{
	CString strRtn = _T("");
	strRtn = m_pNmcDevice->GetErrorMessage();
	return strRtn;
}

int CEtherCat::InitNmcBoard()
{
 	m_pNmcDevice->InitDevice(1); // 1 is Number Of MMC Board.

	structMotionParam *psttMotion;

	for (int axis = 0; axis <= 3; axis++)
	{
		//switch (axis)
		//{
		//case MOTOR_X: psttMotion = &pVrsDoc->m_MotionX;
		//	break;
		//case MOTOR_Y: psttMotion = &pVrsDoc->m_MotionY;
		//	break;
		//case MOTOR_FOCUS: psttMotion = &pVrsDoc->m_MotionFocus;
		//	break;
		//case MOTOR_ZOOM: psttMotion = &pVrsDoc->m_MotionZoom;
		//	break;
		//}
		m_pNmcDevice->CreateAxis(axis, *psttMotion);
		AmpFaultReset(axis);
	}

	m_pNmcDevice->ResetAxesGroup();

	//===================================================================
	return 0; //error;
}

BOOL CEtherCat::Exit()
{
	if (m_pNmcDevice)
	{
		if (!m_pNmcDevice->DestroyDevice())
			return FALSE;

		delete m_pNmcDevice;
		m_pNmcDevice = NULL;
	}
	return TRUE;
}

void CEtherCat::SyncPos(int nAxisID)
{
	if (m_bUseEncoderSignal[nAxisID])
	{
		double dPos = m_pNmcDevice->GetAxis(nAxisID)->GetActualPosition();

		if (fabs(dPos - m_pNmcDevice->GetAxis(nAxisID)->GetCommandPosition()) > 0.005)
			m_pNmcDevice->GetAxis(nAxisID)->SetCommandPosition(dPos);
	}
}

void CEtherCat::EmergencyStop(int nAxisID)
{
	m_pNmcDevice->GetAxis(nAxisID)->SetEStop();
}

void CEtherCat::NormalStop(int nAxisID)
{
}

double CEtherCat::GetInposition(int nAxisID)
{
	double dRtn = 0.0;
	dRtn = m_pNmcDevice->GetAxis(nAxisID)->GetInposition();
	return dRtn;
}

double CEtherCat::GetSpeedProfile(int nMode, int nAxisID, double fLength, double &fVel, double &fAcc, double &fJerk, int nSpeedType)
{
	double dMotionTime = 0.0, dAccTime = 0.0, dVelocityTime = 0.0, dSettleTime = 0.0, dTotalTime = 0.0;
	double dAccTimeToMaxSpeed = 0.0;
	double dMaxVelocity = 0.0, dMaxAccel = 0.0, dMaxJerk = 0.0;
	double dVelocity = 0.0, dAccel = 0.0, dAccPeriod = 0.0, dAccLength = 0.0;

	dMaxVelocity = m_fMaxVel[nAxisID];
	dMaxAccel = m_fMaxAcc[nAxisID];

	dVelocity = m_fVel[nAxisID];
	dAccel = m_fAcc[nAxisID];

	dMaxJerk = dMaxAccel / m_fMinJerkTime[nAxisID];
	dAccPeriod = m_fAccPeriod[nAxisID];

	// 속도, 가속도 값이 최대 가속도이내로 설정되도록 한다.
	if (dVelocity > dMaxVelocity || dVelocity < 0.0)
		dVelocity = dMaxVelocity;

	if (dAccel > dMaxAccel || dAccel < 0.0)
		dAccel = dMaxAccel;

	if (nSpeedType == LOW_SPEED)
	{
		dVelocity /= 4.;
		dAccel /= 4.;
		dMaxJerk /= 4.;
	}
	if (nSpeedType == MIDDLE_SPEED)
	{
		dVelocity /= 2.;
		dAccel /= 2.;
		dMaxJerk /= 2.;
	}
	if (nSpeedType == HIGH_SPEED)
	{
		dVelocity /= 1.;
		dAccel /= 1.;
		dMaxJerk /= 1.;
	}

	if (nMode == S_CURVE)
	{
		fVel = dVelocity;
		fAcc = dAccel;
		fJerk = dMaxJerk;
		if (GetSCurveVelocity(fLength, fVel, fAcc, dMaxJerk) == FALSE)
		{
			fVel = dVelocity;
			fAcc = dAccel;
			fJerk = dMaxJerk;//dAcc * 10.0;//
		}
		dMotionTime = GetMotionTime(fLength, fVel, fAcc, fJerk);
	}
	else if (nMode == TRAPEZOIDAL)
	{
		// 가속 구간 값이 50%를 초과할경우 50%로 규제한다.
		if (dAccPeriod > 50.0)
			dAccPeriod = 50.0;

		if (dAccPeriod < 50.0) // trapezoidal profile
		{
			// 전체 이동 거리중 가속구간의 거리 및 소요 시간 계산,
			dAccLength = fLength * dAccPeriod * 2.0 / 100.0;
		}
		else // triangular profile
		{
			// 전체 이동 거리의 1/2구간의 거리 및 소요 시간 계산,
			dAccLength = fLength / 2.0;
		}
		dAccTime = sqrt(2.0 * dAccLength / dAccel);

		fAcc = dAccel;

		if (dVelocity < fAcc * dAccTime)
			fVel = dVelocity;
		else
			fVel = fAcc * dAccTime;

		// 정속 구간의 이동시간을 구한다.
		dVelocityTime = (fLength - dAccLength) / fVel;
		dMotionTime = dAccTime * 2 + dVelocityTime;
	}

	dSettleTime = (double)(100.0 / 1000.0);
	dTotalTime = dMotionTime + dSettleTime;

	return dTotalTime;
}

double CEtherCat::GetMotionTime(double dLen, double dVel, double dAcc, double dJerk)
{
	return dAcc / dJerk + dVel / dAcc + dLen / dVel;//[sec]
}

BOOL CEtherCat::GetSCurveVelocity(double dLen, double &dVel, double &dAcc, double &dJerk)
{
	do {
		if ((dVel / dAcc) < (dAcc / dJerk))
			dVel = dAcc * dAcc / dJerk;

		double fTemp = (dLen / dVel - (dVel / dAcc + dAcc / dJerk));
		if (fTemp > 0 || fabs(fTemp) < 0.000000001)
		{
			break;
		}
		else
		{
			double a = 1 / dAcc;
			double b = dAcc / dJerk;
			double c = -dLen;
			double r1 = (-b + sqrt(b*b - 4 * a * c)) / (2 * a);
			double r2 = (-b - sqrt(b*b - 4 * a * c)) / (2 * a);

			if (r1 > r2)
			{
				dVel = r1;
				if (dVel / dAcc < dAcc / dJerk)
					dAcc = sqrt(dJerk * dVel);
			}
			else
			{
				AfxMessageBox(_T("Error : r1 < r2"));
				return FALSE;
			}
		}
	} while (1);
	return TRUE;
}

int CEtherCat::StartSCurveMotion(int nAxisID, double fPos, double fVel, double fAcc, double fJerk, BOOL bAbs, BOOL bWait)
{
	if (m_bSafetyInterlock[nAxisID])
		SetHomeAction(nAxisID, E_STOP_EVENT);

	if (CheckAxisState(nAxisID) != ST_NONE)
	{
		int nEventID = CheckExceptionEvent(nAxisID);
		if (nEventID & ST_INPOSITION_STATUS)
			nEventID ^= ST_INPOSITION_STATUS;
		if (nEventID & ST_COLLISION_STATE)
		{
			ClearStatus(nAxisID);
			MC_STATUS ms = MC_OK;
			ms = MC_Reset(m_nBoardId, nAxisID);
		}
		if (nEventID & ST_AMP_POWER_ONOFF)
		{
			int axisStatus = 0;
			ClearStatus(nAxisID);
			ClearFault(nAxisID); // Command 값을 Actual 값으로 error값을 0로 ?? 
			GetAmpEnable(nAxisID, &axisStatus);

			if (!axisStatus)
			{
				EnableAmplifier(nAxisID);
				ClearStatus(nAxisID);
				ClearFault(nAxisID); // Command 값을 Actual 값으로 error값을 0로 ?? 
			}
		}
		if (nEventID & ST_X_POS_LIMIT || nEventID & ST_X_NEG_LIMIT || nEventID & ST_POS_LIMIT || nEventID & ST_NEG_LIMIT || nEventID & ST_AMP_FAULT)
		{
			m_pNmcDevice->GetAxis(nAxisID)->EnableSwLimit(FALSE);

			int axisStatus = 0;
			ClearStatus(nAxisID);
			ClearFault(nAxisID); // Command 값을 Actual 값으로 error값을 0로 ?? 
			GetAmpEnable(nAxisID, &axisStatus);

			if (!axisStatus)
				EnableAmplifier(nAxisID);

			if (CheckAmpFaultSwitch(nAxisID))
			{
				return 0;
			}

		}
		else if (nEventID == ST_HOME_SWITCH)
		{
			//ALARM_MOTION_STATE_RESTORE_FAIL
			return 0;
		}
	}
	SyncPos(nAxisID);
	m_pNmcDevice->GetAxis(nAxisID)->StartSCurveMove(fPos, fVel, fAcc, fJerk, bAbs, bWait);

	return MC_OK;
}

BOOL CEtherCat::SearchHomePos(int axis)
{
	BOOL bRtn = FALSE;
	CString strTitleMsg, strMsg;

	structMotionParam sttMotion;

	//switch (axis)
	//{
	//case Xaxis:
	//	sttMotion = pVrsDoc->m_MotionX;
	//	break;
	//case Yaxis:
	//	sttMotion = pVrsDoc->m_MotionY;
	//	break;
	//case FOCUS:
	//	sttMotion = pVrsDoc->m_MotionFocus;
	//	break;
	//case ZOOM:
	//	sttMotion = pVrsDoc->m_MotionZoom;
	//	break;
	//}

	if (sttMotion.Motor.bType == -1)
		return TRUE;

	bRtn = m_pNmcDevice->GetAxis(axis)->StartHomming(axis);

	if (bRtn)
	{
		SetOriginStatus(axis, TRUE);
		return TRUE;
	}

	SetOriginStatus(axis, FALSE);
	return FALSE;
}

void CEtherCat::GetSoftwareLimit(int nAxisID, double &fPosLimit, double &fNegLimit)
{
	fPosLimit = m_fPosLimit[nAxisID];
	fNegLimit = m_fNegLimit[nAxisID];
}

int CEtherCat::AmpFaultReset(int nAxisID)
{
	int nRtn = 0;
	nRtn = m_pNmcDevice->GetAxis(nAxisID)->AmpFaultReset();
	return nRtn;
}

int CEtherCat::CheckAmpFaultSwitch(int nAxisID)
{
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->CheckAmpFaultSwitch();
	return int(bRtn ? TRUE : FALSE);
}

int CEtherCat::ClearCommandBuffer(int nAxisID)
{
	BOOL bRtn = FALSE;
	bRtn = m_pNmcDevice->GetAxis(nAxisID)->CmdBufferClear();
	return bRtn;
}

int CEtherCat::Out32(int port, long value)
{
	return m_pNmcDevice->Out32(port, value);
}

int CEtherCat::InOption(int port, long *value)
{
	return m_pNmcDevice->InOption(port, value);
}

int CEtherCat::OutOption(int port, long value)
{
	return m_pNmcDevice->OutOption(port, value);
}

void CEtherCat::SetMotorDir(int lMotor, int lDirection)
{
	m_MotorDir[lMotor] = lDirection;
}

BOOL CEtherCat::SetJoystickEnable(int nBoardNum, short bEnable)
{
	return m_pNmcDevice->SetJoystickEnable(nBoardNum, bEnable);
}

BOOL CEtherCat::SetJoystickVelocity(int nBoardNum, short nVel0, short nVel1, short nVel2, short nVel3)
{
	return m_pNmcDevice->SetJoystickVelocity(nBoardNum, nVel0, nVel1, nVel2, nVel3);
}

BOOL CEtherCat::GetJoystickEnable(int nBoardNum, short &bEnable)
{
	return m_pNmcDevice->GetJoystickEnable(nBoardNum, bEnable);
}





