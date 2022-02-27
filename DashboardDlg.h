// DashboardDlg.h : header file
//

#if !defined(AFX_DASHBOARDDLG_H__E6CDC0B6_4A9B_4CEF_8ABA_BC15454E7349__INCLUDED_)
#define AFX_DASHBOARDDLG_H__E6CDC0B6_4A9B_4CEF_8ABA_BC15454E7349__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDashboardDlg dialog

class CDashboardDlg : public CDialog
{
// Construction
public:
	bool fLastExtButton;
	volatile long nCommTimeout;
	int bufidx;
	unsigned char inp[256];
	HANDLE hPort;
	double dist;
	int distanceMode;	// 0=km, 1=miles, 2=megafurlongs per fortnight
	time_t StartTime;
	char szTrip[32];
	char szTime[16];
	char szDate[16];
	int UpdateCountdown;
	int ActualRPM;
	int ActualSpeed;
	int CurrentRPM;
	int CurrentSpeed;
	int ActualLoad;
	int CurrentLoad;
	int ActualTemp;
	int CurrentTemp;
	CDC WorkDC;
	CBitmap WorkBmp;
	CFont TripFont;
	CFont ClockFont;
	CFont DateFont;
	CFont ScaleFont;
	CFont SpeedFont;
	CFont LogFont;
	char szMiniPic[256];
	HANDLE hTach, hSpeed, hBackground, hTachMask, hSpeedMask, hWebcam;
	char szExtFile[MAX_PATH+1];
	char szWinAmp[MAX_PATH+1], szLastWinAmp[128];
	int WinAmpStatus;	// 0 - none, 1 - starting, 2 - paused, 3 - stopped, 4 - prev, 5 - next
	bool fWinAmpPlaying, fWinAmpRandom, fWinAmpPrev, fWinAmpNext;
	CDashboardDlg(CWnd* pParent = NULL);	// standard constructor
	void MonitorSerialThread();
	void StartWinAmp();
	bool ReadPIOPaperOut();

	void takedownDirectDraw();
	void SetupDirectDraw();

	IDirectDraw7 *lpdd;					// DirectDraw object
	LPDIRECTDRAWSURFACE7 lpdds;			// Primary surface
	LPDIRECTDRAWSURFACE7 ddsBack;		// Back buffer

// Dialog Data
	//{{AFX_DATA(CDashboardDlg)
	enum { IDD = IDD_DASHBOARD_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDashboardDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDashboardDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DASHBOARDDLG_H__E6CDC0B6_4A9B_4CEF_8ABA_BC15454E7349__INCLUDED_)
