// NmcDevice.cpp : implementation file
//
#include "stdafx.h"

#include "NmcDevice.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNmcDevice

CNmcDevice::CNmcDevice(UINT16 nBoardID, UINT16 nDevIdIoIn, UINT16 nDevIdIoOut)
{
	m_nBoardId = nBoardID;
	m_nDevIdIoIn = nDevIdIoIn;
	m_nDevIdIoOut = nDevIdIoOut;

	m_nOffsetIoOption = 0;
	m_nOffsetAxisID = 1;
	m_strErrMsg = _T("");

	m_nTotalAxisNum = 4;
	for(int i = 0; i<m_nTotalAxisNum; i++)
		m_pAxis[i] = NULL;
	m_dwDeviceIoOut=0xFFFFFFFF;
}

CNmcDevice::~CNmcDevice()
{
	for(int i = 0; i<m_nTotalAxisNum; i++)
	{
		if(!m_pAxis[i])
		{
			delete m_pAxis[i];
			m_pAxis[i] = NULL;
		}
	}
}


BEGIN_MESSAGE_MAP(CNmcDevice, CWnd)
	//{{AFX_MSG_MAP(CNmcDevice)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNmcDevice message handlers

BOOL CNmcDevice::InitAxisParam(int nAxis, structMotionParam &sttMotion)
{
	if(!m_pAxis[nAxis])
		return FALSE;

	m_pAxis[nAxis]->m_stParam.Motor.fLeadPitch = sttMotion.Motor.fLeadPitch; // [mm]
	m_pAxis[nAxis]->m_stParam.Motor.nEncPulse = sttMotion.Motor.nEncPulse; // [pulse/rev]
	m_pAxis[nAxis]->m_stParam.Motor.nEncMul = sttMotion.Motor.nEncMul;
	m_pAxis[nAxis]->m_stParam.Motor.fGearRatio = sttMotion.Motor.fGearRatio; // [load/roater]
	m_pAxis[nAxis]->m_stParam.Motor.fInpRange = sttMotion.Motor.fInpRange;
	m_pAxis[nAxis]->m_stParam.Motor.nInpTime = sttMotion.Motor.nInpTime;
	m_pAxis[nAxis]->m_stParam.Motor.fPosRes = sttMotion.Motor.fLeadPitch/(double)(sttMotion.Motor.nEncPulse*sttMotion.Motor.nEncMul*sttMotion.Motor.fGearRatio); // [mm]
	m_pAxis[nAxis]->m_stParam.Speed.fAccPeriod = sttMotion.Speed.fAccPeriod;
	m_pAxis[nAxis]->m_stParam.Speed.fMaxAcc = sttMotion.Motor.fMaxAccel; // [mm/s2]
	m_pAxis[nAxis]->m_stParam.Speed.fVel = sttMotion.Speed.fSpd; // [mm/s]
	m_pAxis[nAxis]->m_stParam.Speed.fAcc = sttMotion.Speed.fAcc; // [mm/s2]
	m_pAxis[nAxis]->m_stParam.Speed.fMinJerkTime = sttMotion.Motor.fMinJerkTime;
	m_pAxis[nAxis]->m_stParam.Speed.fMaxVel = sttMotion.Motor.fLeadPitch*(sttMotion.Motor.fRatingSpeed/60.); // [mm/s]
	m_pAxis[nAxis]->m_stParam.Speed.fRatingRPM = sttMotion.Motor.fRatingSpeed; // [rev/min]

	m_pAxis[nAxis]->m_stParam.Motor.nAxisID = nAxis;
	m_pAxis[nAxis]->m_stParam.Motor.bType = sttMotion.Motor.bType;				// Motor Type 0: servo 1: stepper
	m_pAxis[nAxis]->m_stParam.Motor.bSafety = sttMotion.Motor.bSafety;			// Safety Flag
	m_pAxis[nAxis]->m_stParam.Motor.fPosLimit = sttMotion.Motor.fPosLimit;		// Positive Software Limit
	m_pAxis[nAxis]->m_stParam.Motor.fNegLimit = sttMotion.Motor.fNegLimit;		// Negative Software Limit
	m_pAxis[nAxis]->m_stParam.Motor.nVibOffset = sttMotion.Motor.nVibOffset;	// Vibration offset

	m_pAxis[nAxis]->m_stParam.Home.bDir = sttMotion.Home.bDir;			// Initial Home Dir, TRUE:plus-dir	FALSE:minus-dir
	m_pAxis[nAxis]->m_stParam.Home.f1stSpd = sttMotion.Home.f1stSpd;	// PreHomming Speed
	m_pAxis[nAxis]->m_stParam.Home.f2ndSpd = sttMotion.Home.f2ndSpd;	// Homming Speed
	m_pAxis[nAxis]->m_stParam.Home.fAcc = sttMotion.Home.fAcc;			// Homming Accel
	m_pAxis[nAxis]->m_stParam.Home.fShift = sttMotion.Home.fShift;		// shift
	m_pAxis[nAxis]->m_stParam.Home.fOffset = sttMotion.Home.fOffset;	// offset
	m_pAxis[nAxis]->m_stParam.Home.fMargin = sttMotion.Home.fMargin;	// Margin between Origin & index After Homming
	m_pAxis[nAxis]->m_stParam.Home.bStatus = FALSE;

	m_pAxis[nAxis]->m_stParam.Speed.fDec = sttMotion.Speed.fDec;					// Deceleration
	m_pAxis[nAxis]->m_stParam.Speed.fJogFastSpd = sttMotion.Speed.fJogFastSpd;		// Speed
	m_pAxis[nAxis]->m_stParam.Speed.fJogMidSpd = sttMotion.Speed.fJogMidSpd;		// Speed
	m_pAxis[nAxis]->m_stParam.Speed.fJogLowSpd = sttMotion.Speed.fJogLowSpd;		// Speed
	m_pAxis[nAxis]->m_stParam.Speed.fJogAcc = sttMotion.Speed.fJogAcc;				// Acceleration

	m_pAxis[nAxis]->m_stParam.Io.bPosLimit = sttMotion.IO.bPosLimit;	// 정방향 리미트 스위치 신호 레벨
	m_pAxis[nAxis]->m_stParam.Io.bNegLimit = sttMotion.IO.bNegLimit;	// 역방향 리미트 스위치 신호 레벨
	m_pAxis[nAxis]->m_stParam.Io.bOrg = sttMotion.IO.bOrg;				// 원점 스위치 신호 레벨
	m_pAxis[nAxis]->m_stParam.Io.bAmpFault = sttMotion.IO.bAmpFault;	// Amp Fault 신호 레벨
	m_pAxis[nAxis]->m_stParam.Io.bAmpEnable = sttMotion.IO.bAmpEnable;	// Amp Enable 신호 레벨

	m_pAxis[nAxis]->m_stParam.Gain.nP = sttMotion.Gain.nP;					// P Gain
	m_pAxis[nAxis]->m_stParam.Gain.nI = sttMotion.Gain.nI;					// I Gain
	m_pAxis[nAxis]->m_stParam.Gain.nD = sttMotion.Gain.nD;					// D Gain
	m_pAxis[nAxis]->m_stParam.Gain.nAccelFF = sttMotion.Gain.nAccelFF;		// Acceleration Feed Forward
	m_pAxis[nAxis]->m_stParam.Gain.nVelFF = sttMotion.Gain.nVelFF;			// Velocity Feed Forward
	m_pAxis[nAxis]->m_stParam.Gain.nILimit = sttMotion.Gain.nILimit;		// Integral Limit
	m_pAxis[nAxis]->m_stParam.Gain.nOffset = sttMotion.Gain.nOffset;		// Command Offset
	m_pAxis[nAxis]->m_stParam.Gain.nDACLimit = sttMotion.Gain.nDACLimit;	// DAC Limit
	m_pAxis[nAxis]->m_stParam.Gain.nShift = sttMotion.Gain.nShift;			// Gain Shift
	m_pAxis[nAxis]->m_stParam.Gain.nFrictFF = sttMotion.Gain.nFrictFF;		// Friction Feed Forward



	double dLength = 0.0;

	m_pAxis[nAxis]->SetNegLimitAction(E_STOP_EVENT);
	Sleep(100);
	m_pAxis[nAxis]->SetPosLimitAction(E_STOP_EVENT);
	Sleep(100);
	m_pAxis[nAxis]->SetHomeAction(E_STOP_EVENT);
	Sleep(100);
	m_pAxis[nAxis]->SetNegSoftwareLimit(sttMotion.Motor.fNegLimit, E_STOP_EVENT);
	Sleep(100);
	m_pAxis[nAxis]->SetPosSoftwareLimit(sttMotion.Motor.fPosLimit, E_STOP_EVENT);
	Sleep(100);

/*
	<In - Position Check Type 설정>
		0: None(In - Position Check 하지 않음)
		1 : External Drive(In - Position State를 외부 Drive에서 받음)
		2 : Internal(In - Position을 내부에서 판단)(Default)
		3 : External Drive + Internal
*/		
	m_pAxis[nAxis]->SetInPosEnable(1);
	Sleep(100);

	if (sttMotion.Motor.bType == 0)		// Servo
	{
		m_pAxis[nAxis]->SetInPosLength(sttMotion.Motor.fInpRange);
		Sleep(100);
	}
	m_pAxis[nAxis]->SetEStopRate(20.0);
	Sleep(100);

	return TRUE;
}

BOOL CNmcDevice::InitDevice(int nDevice)
{
	MC_STATUS ms = MC_OK;
	TCHAR msg[MAX_ERR_LEN];

	UINT16 MstDevIDs[MAX_BOARD_CNT];
	UINT16 MstCnt;

	char cstrErrorMsg[MAX_ERR_LEN];
	int i = 0, j = 0;
	ULONGLONG StartTimer;
	UINT8	MstMode;

	ms = MC_GetMasterMap(MstDevIDs, &MstCnt);

	if (ms != MC_OK) //MC_OK가 아닐 경우 Error 출력
	{
		MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
		_stprintf(msg, _T("Error :: 0x%08X, %hs\n"), ms, cstrErrorMsg);
		TRACE(msg);
		return FALSE;
	}
	else
	{
		_stprintf(msg, _T("Board Count = %d , Board List : [%d],[%d],[%d],[%d]\n"),
			MstCnt,     // 설치된 보드 개수
			MstDevIDs[0],    // 첫번째 보드 ID
			MstDevIDs[1],    // 두번째 보드 ID
			MstDevIDs[2],    // 세번째 보드 ID
			MstDevIDs[3]);   // 네번째 보드 ID

		TRACE(msg);
	}

	for (i = 0; i < MstCnt; i++)
	{
		if (MstDevIDs[i] == 0xCCCC)
		{
			return FALSE;
		}
		ms = MC_MasterInit(MstDevIDs[i]);
		if (ms != MC_OK) //MC_OK가 아닐경우 Error 출력
		{
			MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
			_stprintf(msg, _T("Error :: 0x%08X, %hs\n"), ms, cstrErrorMsg);
			TRACE(msg);

			return FALSE;
		}
	}

	for (i = 0; i < MstCnt; i++)
	{
		ms = MC_MasterSTOP(MstDevIDs[i]);
		if (ms != MC_OK) //MC_OK가 아닐 경우 Error 출력
		{
			MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
			_stprintf(msg, _T("Error :: 0x%08X, %hs\n"), ms, cstrErrorMsg);
			TRACE(msg);

			return FALSE;
		}
	}

	for (i = 0; i < MstCnt; i++)
	{
		ms = MC_MasterRUN(MstDevIDs[i]);
		if (ms != MC_OK)
		{
			MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
			_stprintf(msg, _T("Error :: 0x%08X, %hs\n"), ms, cstrErrorMsg);
			TRACE(msg);

			return FALSE;
		}
		else
		{
			//MC_OK 리턴되면 Master State확인
			StartTimer = GetTickCount64();
			while (1)
			{
				//Sleep(100);
				ms = MasterGetCurMode(MstDevIDs[i], &MstMode);
				if (ms != MC_OK)
				{
					MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
					_stprintf(msg, _T("Error :: 0x%08X, %hs\n"), ms, cstrErrorMsg);
					TRACE(msg);

					return FALSE;
				}

				if (MstMode == EcMstMode::eMM_RUN)
				{
					//Master State = RUN
					break;
				}

				//Master가 Error상태이거나 통신이 끊긴 상태인지 확인
				if (MstMode == EcMstMode::eMM_ERR || MstMode == EcMstMode::eMM_LINKBROKEN)
				{
					TRACE(_T("Master State is ERROR or LINKBROKEN  State\n"));

					return FALSE;
				}

				if (GetTickCount64() - StartTimer > 5000)
				{
					TRACE(_T("Check Master Run Status Time Out\n"));

					return FALSE;
				}
			}
		}
	}

	UINT8 SlaveState;
	for (i = 0; i < MstCnt; i++)
	{
		Sleep(100);

		StartTimer = GetTickCount64();
		while (1)
		{
			ms = SlaveGetCurState(MstDevIDs[i], 1, &SlaveState);
			if (ms != MC_OK)
			{
				MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
				_stprintf(msg, _T("Error :: 0x%08X, %hs\n"), ms, cstrErrorMsg);
				TRACE(msg);

				return FALSE;
			}
			else
			{

				if (SlaveState == eST_OP)
				{
					_stprintf(msg, _T("Board%d, EcatAddr%d 통신 정상\n"), MstDevIDs[i], 1);
					TRACE(msg);

					break;
				}

				if (GetTickCount64() - StartTimer > 5000)
				{
					TRACE(_T("Get Current Slave State Time Out, 0x%08X\n"), SlaveState);

					return FALSE;
				}
			}
		}
	}

	return TRUE;
}

BOOL CNmcDevice::DestroyDevice()
{
	for(int iAxis=0; iAxis<m_nTotalAxisNum; iAxis++)	// 20110922 hyk mod
	{
		if(m_pAxis[iAxis])
		{
			if( !m_pAxis[iAxis]->IsMotionDone() )
			{
				m_pAxis[iAxis]->Stop();
				m_pAxis[iAxis]->CmdBufferClear();
			}
			m_pAxis[iAxis]->SetAmpEnable(FALSE);
		}
	}

	MC_STATUS ms = MC_OK;
	char cstrErrorMsg[MAX_ERR_LEN];
	MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
	if (ms != MC_OK)
	{
		AfxMessageBox(_T("Failed MMC Board Exit !!!"));
		return FALSE;
	}

	ResetAxesGroup();

	//Delete Memory Allocation For Axis Object.
	for (int i=0; i<m_nTotalAxisNum; i++)	//20110922 hyk mod
	{
		if(m_pAxis[i])		
		{
			delete(m_pAxis[i]);
			m_pAxis[i] = NULL;
		}
	}
	return TRUE;
}

BOOL CNmcDevice::CreateAxis(int nAxis, structMotionParam &sttMotion)
{
	if(!m_pAxis[nAxis])
	{
		m_pAxis[nAxis] = new CNmcAxis(m_nBoardId);
		m_pAxis[nAxis]->SetParentClassHandle(this->GetSafeHwnd());
	}

	if(!InitAxisParam(nAxis , sttMotion))
		return FALSE;
		
	return TRUE;
}

CNmcAxis* CNmcDevice::GetAxis(int nAxis)
{
	CString strMsg;
	if(!m_pAxis[nAxis])
	{
		strMsg.Format(_T("Didn't create %d axis."), nAxis);
		AfxMessageBox(strMsg);
	}
	return m_pAxis[nAxis];
}

int CNmcDevice::Out32(int port, long value)
{
	int nRtn = MC_OK;
	UINT uBitSize = 32;
	UINT uDataSize = uBitSize / 8;
	UINT8* bData = new BYTE[uDataSize];
	long val;

	//IO 상태 Read
	MC_IO_READ(m_nBoardId, m_nDevIdIoIn, 0, 0, uDataSize, bData);
	val = (long)bData;
	val = ~val;

	if(val != value)
	{
		value = ~value;

		//IO 상태 Write		 
		MC_IO_WRITE(m_nBoardId, m_nDevIdIoOut, 0, uDataSize, (UINT8*)&value);
	}
	delete bData;

	return nRtn;
}

int CNmcDevice::In32(int port, long *value)
{
	int nRtn = MC_OK;
	UINT uBitSize = 32;
	UINT uDataSize = uBitSize / 8;
	long nData;

	//IO 상태 Read
	MC_IO_READ(m_nBoardId, m_nDevIdIoIn, 1, 0, uDataSize, (UINT8*)&nData);
	*value = (long)nData;

	return nRtn;
}

int CNmcDevice::OutOption(int port, long value)
{
	int nRtn = MC_OK;
	long ReadVal = 0;
	if (MC_OK == In32(port, &ReadVal))
	{
		value = (value << m_nOffsetIoOption);
		// OutOption의 value에 해당하는 IO의 비트의 위치값으로 ReadVal에 적용하여 출력 value를 생성
		nRtn = Out32(port, value);
	}

	return nRtn;
}

int CNmcDevice::InOption(int port, long *value)
{
	int nRtn = MC_OK;
	long ReadVal = 0;

	if (MC_OK == In32(port, &ReadVal))
	{
		ReadVal = (ReadVal >> m_nOffsetIoOption);
		// ReadVal의 value에 해당하는 IO의 비트의 위치값으로 OutOption에 적용하여 입력 value를 생성
		*value = ~ReadVal;
	}

	return nRtn;
}

BOOL CNmcDevice::SetJoystickEnable(int nBoardNum, short bEnable)
{
	INT nLevel = 0, nState = 0;
	return (nState ? TRUE : FALSE);
}

BOOL CNmcDevice::GetJoystickEnable(int nBoardNum, short &bEnable)
{
	INT nLevel = 0, nState = 0;
	return (nState ? TRUE : FALSE);
}

BOOL CNmcDevice::SetJoystickVelocity(int nBoardNum, short nVel0, short nVel1, short nVel2, short nVel3)
{
	INT nLevel = 0, nState = 0;
	return (nState ? TRUE : FALSE);
}

BOOL CNmcDevice::GetJoystickVelocity(int nBoardNum, short & nVel0, short & nVel1, short & nVel2, short & nVel3)
{
	INT nLevel = 0, nState = 0;
	return (nState ? TRUE : FALSE);
}

int CNmcDevice::ReadOut32(int port, long *value)
{
	int nRtn = MC_OK;
	nRtn = In32(port, value);
	return nRtn;
}

void CNmcDevice::OutBit(int port, long bit, bool flag)
{
	if(flag)
	{
		m_dwDeviceIoOut |= (0x00000001 << bit); //OFF is High Voltage.
	}
	else
	{
		m_dwDeviceIoOut &= ~(0x00000001 << bit); //ON is Low Voltage.
	}

	Out32(port, (long)m_dwDeviceIoOut);
}

BOOL CNmcDevice::InBit(int port, long bit)
{
	long value = 0;
	if (MC_OK != In32(port, &value))
	{
		CString strMsg;
		strMsg.Format(_T("port %d bit %d is Error."), port, bit);
		AfxMessageBox(strMsg);

		return FALSE;
	}
	return( !(value & (0x00000001 << bit)) );
}

void CNmcDevice::SetMapAxes(int nNumAxes, short* axes)
{
	INT nAxes = (INT)nNumAxes;
	INT* npAxes = (INT*)axes;
}

// #check
void CNmcDevice::SetMoveAccel(int nAxisId, double fSpeed, double fAcc)
{
	double dNmcAccTime = 1.0;
	if (fSpeed < 0.0)	fSpeed = -1.0 * fSpeed;
	if (fAcc < 0.0)	fAcc = -1.0 * fAcc;
	double dAccTime = GetAccTime(nAxisId, fSpeed, fAcc);

	m_dSpeed[nAxisId] = fSpeed;
	m_dAcc[nAxisId] = fAcc;
	m_dDec[nAxisId] = fAcc;
	m_dJerk[nAxisId] = fAcc / dAccTime;
}

double CNmcDevice::GetAccTime(int nAxisId, double dVel,double dAcc,double dJerk)
{
	double dNmcAccTime = 1.0;
	dNmcAccTime = GetAxis(nAxisId)->GetAccTime(dVel, dAcc, dJerk);
	return dNmcAccTime;
}

BOOL CNmcDevice::ResetAxesGroup()
{
	MC_STATUS ms = MC_OK;
	UINT16 nAxesNum = 2;
	CString msg;
	char cstrErrorMsg[MAX_ERR_LEN];

	UINT32 GroupStatus = 0;
	UINT32 AxisStatus = 0;
	UINT32 AxisStatus2 = 0;
	UINT16 GroupNo = 0;
	UINT16 PositionCount = 2;	// 2축 직선보간운동

	MC_GroupReadStatus(m_nBoardId, GroupNo, &GroupStatus);
	if (GroupStatus & GroupStandby)
	{
		ms = MC_GroupDisable(m_nBoardId, GroupNo);
		Sleep(100);

		if (ms != MC_OK)
		{
			MC_GetErrorMessage(ms, MAX_ERR_LEN, cstrErrorMsg);
			msg.Format(_T("Error :: 0x%08X, %s"), ms, cstrErrorMsg);
			AfxMessageBox(msg);

			return FALSE;
		}
		else
		{
			//MC_GroupReadStatus를 통해 GroupDisabled 되었는지 확인
			MC_GroupReadStatus(m_nBoardId, GroupNo, &GroupStatus);
			if (GroupStatus & GroupDisabled)
			{
				//GroupDisabled가 On이면 GroupDisable 완료
				//AfxMessageBox(_T("GroupDisable Done"));
				return TRUE;
			}
			else
			{
				//GroupDisable 실패하면 Status 값 출력
				msg.Format(_T("GroupDisable Fail, GroupStatus: 0x%04x"), GroupStatus);
				AfxMessageBox(msg);
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

BOOL CNmcDevice::SetAxesGroup(int nAxisId_0, int nAxisId_1)
{
	MC_STATUS ms = MC_OK;
	UINT16 nAxesNum = 2;
	UINT16 arnAxes[2] = { nAxisId_0 + m_nOffsetAxisID, nAxisId_1 + m_nOffsetAxisID };
	MC_DIRECTION pDirArray[2] = { mcPositiveDirection, mcPositiveDirection };
	UINT8 ErrorStopMode = 1;	// ErrorStop이 발생할 경우 처리 방법 선택(0: Error발생축만 정지, 1: 모든축 동작 정지)
	CString msg;
	char cstrErrorMsg[MAX_ERR_LEN];

	UINT32 GroupStatus = 0;
	UINT32 AxisStatus = 0;
	UINT32 AxisStatus2 = 0;
	UINT16 GroupNo = 0;
	UINT16 PositionCount = 2;	// 2축 직선보간운동

	MC_GroupReadStatus(m_nBoardId, GroupNo, &GroupStatus);
	if (GroupStatus & GroupStandby)
	{
		;
	}
	else
	{
		// MMCE0-Axis1를 Group0의 Identity0로 추가
		MC_AddAxisToGroup(m_nBoardId, arnAxes[0], GroupNo, 0);
		// MMCE0-Axis2를 Group0의 Identity1로 추가
		MC_AddAxisToGroup(m_nBoardId, arnAxes[1], GroupNo, 1);
		ms = MC_GroupEnable(m_nBoardId, 0);
		Sleep(100);

		//MC_GroupReadStatus를 통해 GroupEnable 되었는지 확인
		MC_GroupReadStatus(m_nBoardId, GroupNo, &GroupStatus);
		if (GroupStatus & GroupStandby)
		{
			;
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

CString CNmcDevice::GetErrorMessage()
{
	return m_strErrMsg;
}

int CNmcDevice::GetTotalAxisNum()
{
	return m_nTotalAxisNum;
}

