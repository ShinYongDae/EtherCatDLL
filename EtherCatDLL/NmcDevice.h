#pragma once

// NmcDevice.h : header file
//

#include "NmcAxis.h"

/////////////////////////////////////////////////////////////////////////////
// CNmcDevice window

class CNmcDevice : public CWnd
{
	UINT16	m_nBoardId;
	UINT16	m_nDevIdIoIn;
	UINT16	m_nDevIdIoOut;

	UINT16 m_nOffsetIoOption;

	INT		m_nOffsetAxisID;

	double m_dSpeed[4];
	double m_dAcc[4];
	double m_dDec[4];
	double m_dJerk[4];

	//	CNmcAxis* m_pAxis[8]; // Number of Axis : 0 ~ 7
	CNmcAxis* m_pAxis[4]; // Number of Axis : 0 ~ 3 20110920 hyk mod
	DWORD m_dwDeviceIoOut;

	BOOL InitAxisParam(int nAxis, structMotionParam &sttMotion);

// Construction
public:
	CNmcDevice(UINT16 nBoardID, UINT16 nDevIdIoIn, UINT16 nDevIdIoOut);

// Attributes
public:
	CString m_strErrMsg;

// Operations
public:

	int m_nTotalAxisNum;

	int GetTotalAxisNum();

	BOOL InitDevice(int nDevice); // nDevice : 1 ~ 4
	CNmcAxis* GetAxis(int nAxis);
	int Out32(int port, long value);
	int In32(int port, long *value);
	int ReadOut32(int port, long *value);
	void OutBit(int port, long bit, bool flag);
	BOOL InBit(int port, long bit);
	BOOL CreateAxis(int nAxis, structMotionParam &sttMotion);
	BOOL DestroyDevice();
	void SetMapAxes(int nNumAxes, short* axes);
	void SetMoveAccel(int nAxisId, double fSpeed, double fAcc);
	BOOL SetAxesGroup(int nAxisId_0, int nAxisId_1);
	BOOL ResetAxesGroup();
	CString GetErrorMessage();
	double GetAccTime(int nAxisId, double dVel, double dAcc, double dJerk=25.0);

	int OutOption(int port, long value);
	int InOption(int port, long *value);

	BOOL SetJoystickEnable(int nBoardNum, short bEnable);
	BOOL GetJoystickEnable(int nBoardNum, short &bEnable);
	BOOL SetJoystickVelocity(int nBoardNum, short nVel0, short nVel1, short nVel2, short nVel3);
	BOOL GetJoystickVelocity(int nBoardNum, short & nVel0, short & nVel1, short & nVel2, short & nVel3);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNmcDevice)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNmcDevice();

	// Generated message map functions
protected:
	//{{AFX_MSG(CNmcDevice)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

