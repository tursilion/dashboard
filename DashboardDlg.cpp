//
// (C) 2005 Mike Brent aka Tursi aka HarmlessLion.com
// This software is provided AS-IS. No warranty
// express or implied is provided.
//
// This notice defines the entire license for this code.
// All rights not explicity granted here are reserved by the
// author.
//
// You may redistribute this software provided the original
// archive is UNCHANGED and a link back to my web page,
// http://harmlesslion.com, is provided as the author's site.
// It is acceptable to link directly to a subpage at harmlesslion.com
// provided that page offers a URL for that purpose
//
// Source code, if available, is provided for educational purposes
// only. You are welcome to read it, learn from it, mock
// it, and hack it up - for your own use only.
//
// Please contact me before distributing derived works or
// ports so that we may work out terms. I don't mind people
// using my code but it's been outright stolen before. In all
// cases the code must maintain credit to the original author(s).
//
// -COMMERCIAL USE- Contact me first. I didn't make
// any money off it - why should you? ;) If you just learned
// something from this, then go ahead. If you just pinched
// a routine or two, let me know, I'll probably just ask
// for credit. If you want to derive a commercial tool
// or use large portions, we need to talk. ;)
//
// Commercial use means ANY distribution for payment, whether or
// not for profit.
//
// If this, itself, is a derived work from someone else's code,
// then their original copyrights and licenses are left intact
// and in full force.
//
// http://harmlesslion.com - visit the web page for contact info
//
// DashboardDlg.cpp : implementation file
// This program does all work in km/h
// We sort of assume that speed, RPM and trouble codes (all mode 1/2) are available, we don't check

// Although the strings are hard coded, am making some thoughts about what they should be:

//cnt address mode var chksum
// -- -------- --  --  --
// 07 68 6A F1 01  0C  D0

// For address, the first byte first nibble seems to be direction, or type (6 = to car/query, 4 = from car/response)
// The first byte's second nibble seems to be the ISO key (8 on my car, 1 in some examples)
// The first byte is called 'priority/type' in one doc

// The second address byte is called 'tgt address'. I've only seen 6A to the car and 6B back.

// The third byte is called 'Source Address'. Scantool uses F1, while my Neon uses 40. These don't
// seem to line up well with the tgt address, even in the docs I've read.

// Mode, of course, is 1 to 9, with 1 and 2 being most useful. In the response, it has >40 added to it.

// Var is the variable being requested, and is repeated in the reply.

// chksum is a raw total of all bytes except the count. The count is only used by the chip in the
// converter box, not the car itself.

// Slightly more flexible now...

#include "stdafx.h"
#include <ddraw.h>
#include <math.h>
#include <conio.h>
#include "Dashboard.h"
#include "DashboardDlg.h"

// WinAmp defines
#include "wa_ipc.h"
#define WINAMP_KEY_PREV 40044
#define WINAMP_KEY_PLAY 40045
#define WINAMP_KEY_PAUSE 40046
#define WINAMP_KEY_NEXT 40048

// Parallel port address base
#define PIO_BASE 0x3BC

// Define this to skip the serial protocol and just use a SIN wave
//#define NO_OBD 1

// Comment this out for GDI
#ifndef _DEBUG
#define USE_DIRECTX
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Layout defines
// Status log
#define LOGX 16
#define LOGY 24
#define LOGLINES 8
#define LOGLENGTH 48
#define LOGFONT "OCR-A II"
#define LOGSIZE 15
#define LOGCOLOR 0x3f,0x3d,0xb1
#define WARNCOLOR 0xff,0x00,0x00

// Webcam capture
#define WEBCAMX 55
#define WEBCAMY 247

// Digital speed readout
#define SPEEDX 280
#define SPEEDY 12
#define SPEEDSIZE 105
#define SPEEDFONT "Digital Readout Thick Upright"
#define SPEEDCOLOR 0xa2,0xa2,0xff

// Speed scale (KM/H, MPH, mega-furlongs per fortnight)
#define SCALEX 478
#define SCALEY 25
#define SCALESIZE 30
#define SCALEFONT "Digital Readout Thick Upright"
#define SCALECOLOR 0xa2,0xa2,0xff

// Date 
#define DATEXCENTER 660
#define DATEY 445
#define DATEFONT "OCR-A II"
#define DATESIZE 15
#define DATECOLOR 0x59,0x57,0xdb

// Clock
#define CLOCKXCENTER 648
#define CLOCKY 470
#define CLOCKFONT "Digital Readout"
#define CLOCKSIZE 75
#define CLOCKCOLOR 0x59,0x57,0xdb

// Trip
#define TRIPXCENTER 648
#define TRIPY 559
#define TRIPFONT "OCR-A II"
#define TRIPSIZE 15
#define TRIPCOLOR 0x59,0x57,0xdb

// WinAmp Status (uses log font)
#define WINAMPY 552
#define WINAMPCOLOR 0xff,0x3d,0xb1

// Misc
#define FPS 10
// Get 55ms from the FPS counter - how many ticks it is +1 
#define MS55 (1)
// Neon Max speed is about 221km/h or 137mph (restrictor kicks in)
#define MAXSPEED 175
#define SPEEDRATIO (800.0/MAXSPEED)
// Neon Max RPM is about 6700 RPM (restrictor kicks in)
#define MAXRPM 6750
#define TACHRATIO (800.0/MAXRPM)
#define POWERUPTIME (FPS)
#define TEXTFLAGS (DT_NOCLIP|DT_NOPREFIX)

// km/h to mph - 1 km is 0.62139 miles. 1 hour is 1 hour.
#define mph(x) (int)((x)*0.62139)
// km/h to fpf - 1 km is 4.97112 furlongs. 1 fortnight is 336 hours.
// So we use MegaFurlongs per Fortnight - here multiplied by 10, divide by 10 for display
// TODO: I think this math is wrong. kmh*1670.29632 = fpf
// Where did 23384 come from?
#define fpf(x) (int)(((x)*23384.14848)/100000)

char szLog[LOGLINES][LOGLENGTH];

enum _states {
	WINAMP_NONE=0,
	WINAMP_STARTING,
	WINAMP_SHUFFLE_ON,
	WINAMP_SHUFFLE_OFF,
	WINAMP_PLAY,
	WINAMP_PAUSE,
	WINAMP_PAUSED,
	WINAMP_STOPPED, 
	WINAMP_PREV,
	WINAMP_NEXT
};

char szWinAmpState[10][32] = {
	"none",
	"WinAmp Starting",
	"WinAmp Shuffle On",
	"WinAmp Shuffle Off",
	"WinAmp Play",
	"WinAmp Pause",
	"WinAmp Paused",
	"WinAmp Stopped",
	"WinAmp Previous",
	"WinAmp Next"
};

// Monitor thread to speed up serial communication a bit ;)
void MonitorSerialWrapper(void *in) {
	CDashboardDlg *pDlg=(CDashboardDlg*)in;
	pDlg->MonitorSerialThread();
}

////////////////////////////////////////////////////////////
// DirectX full screen enumeration callback
////////////////////////////////////////////////////////////
HRESULT WINAPI myCallBack(LPDDSURFACEDESC2 ddSurface, LPVOID pData) {
	int *c;

	c=(int*)pData;

	if (ddSurface->ddpfPixelFormat.dwRGBBitCount == (DWORD)*c) {
		*c=(*c)|0x80;
		return DDENUMRET_CANCEL;
	}
	return DDENUMRET_OK;
}

// Helpers
inline COLORREF myRGB(unsigned char r, unsigned char g, unsigned char b) {
	return ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)));
}

void AddLog(char *sz, ...) {
	va_list va;

	memmove(szLog, szLog[1], (LOGLINES-1)*LOGLENGTH);

	va_start(va, sz);
	_vsnprintf(szLog[LOGLINES-1], LOGLENGTH, sz, va);
	szLog[LOGLINES-1][LOGLENGTH-1]='\0';
	va_end(va);
}

void AddWarn(char *sz, ...) {
	va_list va;
	char buf[256], *p;

	// disk log only
	va_start(va, sz);
	_vsnprintf(buf, 256, sz, va);
	buf[255]='\0';
	va_end(va);

	FILE *fp;
	fp=fopen("C:\\LOG.txt", "a");
	fprintf(fp, "%s\n", buf);
	fclose(fp);

	p=strchr(buf, '|');
	if (p) {
		*p='\0';
		memmove(szLog, szLog[1], (LOGLINES-1)*LOGLENGTH);
		_snprintf(szLog[LOGLINES-1], LOGLENGTH, "Warning: %s", buf);
		szLog[LOGLINES-1][LOGLENGTH-1]='\0';
	}
}

// Functions to check for and dismiss any message boxes
BOOL CALLBACK WndEnum(HWND hwnd, LPARAM) {
	HWND hBtn=NULL, hNewBtn=NULL;
	
	do {
		hNewBtn=FindWindowEx(hwnd, hBtn, "Button", "OK");
		if (NULL==hNewBtn) {
			hNewBtn=FindWindowEx(hwnd, hBtn, "Button", "OK");
			if (NULL == hNewBtn) {
				hNewBtn=FindWindowEx(hwnd, hBtn, "Button", "OK");
			}
		}
		if (NULL == hNewBtn) {
			return TRUE;
		}
		hBtn=hNewBtn;

		// We found an 'OK' button. Log and push it!
		char buf[128], buf2[256];
		strcpy(buf, "");

		HWND hstatic=FindWindowEx(hwnd, NULL, "Static", NULL);
		if (NULL != hstatic) {
			::GetWindowText(hstatic, buf, 128);
		} else {
			::GetWindowText(hwnd, buf, 128);
		}
		buf[127]='\0';
		_snprintf(buf2, 256, "%s|Messagebox closed", buf);
		buf2[255]='\0';
		AddWarn(buf2);

		int nId=::GetDlgCtrlID(hBtn);
		if (nId) {
			::PostMessage(hwnd, WM_COMMAND, nId, (LPARAM)hBtn);
		} else {
			::PostMessage(hBtn, WM_LBUTTONDOWN, 0, 0);
			::PostMessage(hBtn, WM_LBUTTONUP, 0, 0);
		}
	} while (hBtn != NULL);
	
	return TRUE;
}

void ScanForMsgBoxes() {
	EnumWindows(WndEnum, 0);
}

/////////////////////////////////////////////////////////////////////////////
// CDashboardDlg dialog

CDashboardDlg::CDashboardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDashboardDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDashboardDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	CurrentSpeed=888;
	CurrentRPM=8888;
	CurrentLoad=188;
	CurrentTemp=888;
	ActualSpeed=0;
	ActualRPM=0;
	ActualLoad=0;
	ActualTemp=0;
	UpdateCountdown=POWERUPTIME;
	distanceMode=0;
	hWebcam=NULL;
	dist=9999;
	strcpy(szTrip, "XXXXXXXXXXXXXXXXXXXXX");
	strcpy(szTime, "88:88");
	strcpy(szDate, "18/88/8888");
	for (int idx=0; idx<LOGLINES; idx++) {
		strcpy(szLog[idx], "XXXXXXXXXXXXXXXXXXXX");
	}
	strcpy(szMiniPic, "C:\\Folder.bmp");

	lpdd=NULL;	
	lpdds=NULL;	
	ddsBack=NULL;

	// We check these, and if they don't match the WinAmp state, we set them
	fWinAmpPlaying=true;
	fWinAmpRandom=true;
	fWinAmpPrev=false;
	fWinAmpNext=false;
}

void CDashboardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDashboardDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDashboardDlg, CDialog)
	//{{AFX_MSG_MAP(CDashboardDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDashboardDlg message handlers

BOOL CDashboardDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Ensure time zone is set
	_tzset();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
#ifndef _DEBUG
//	ShowCursor(false);				// hide the cursor on this window
#endif

#ifdef USE_DIRECTX
	SetupDirectDraw();
#else
#ifdef _DEBUG
	SetWindowPos(&wndTop, 0, 0, 800, 600, SWP_NOCOPYBITS|SWP_SHOWWINDOW);
#else
	SetWindowPos(&wndTopMost, 0, 0, 800, 600, SWP_NOCOPYBITS|SWP_SHOWWINDOW);
#endif
#endif

	hBackground=LoadImage(NULL, "Background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hSpeed=LoadImage(NULL, "Speedometer.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hSpeedMask=LoadImage(NULL, "SpeedMask.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hTach=LoadImage(NULL, "Tachometer.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	hTachMask=LoadImage(NULL, "TachMask.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
//	hWebcam=LoadImage(NULL, "WebCam.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	
	HDC hScrn;
	CDC ScrnDC;

	WorkDC.CreateCompatibleDC(NULL);

	hScrn=::GetDC(NULL);
	ScrnDC.Attach(hScrn);
	WorkBmp.CreateCompatibleBitmap(&ScrnDC,800,600);
	ScrnDC.Detach();
	::ReleaseDC(NULL, hScrn);

	WorkDC.SelectObject(&WorkBmp);

	TripFont.CreatePointFont(TRIPSIZE*10, TRIPFONT);
	ClockFont.CreatePointFont(CLOCKSIZE*10, CLOCKFONT);
	DateFont.CreatePointFont(DATESIZE*10, DATEFONT);
	ScaleFont.CreatePointFont(SCALESIZE*10, SCALEFONT);
	SpeedFont.CreatePointFont(SPEEDSIZE*10, SPEEDFONT);
	LogFont.CreatePointFont(LOGSIZE*10, LOGFONT);

	// Start listening to the serial port
	_beginthread(MonitorSerialWrapper, 0, (void*)this);

	// Start the log
	for (int idx=0; idx<LOGLINES; idx++) {
		strcpy(szLog[idx], "");
	}
	AddLog("Initializing v1.0...");
	fLastExtButton=ReadPIOPaperOut();

	// Set tick counter
	SetTimer(1, 1000/FPS, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDashboardDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);
#ifdef _DEBUG
		LARGE_INTEGER start, end, freq;
		QueryPerformanceCounter(&start);
#endif

		dc.BitBlt(0, 0, 800, 600, &WorkDC, 0, 0, SRCCOPY);

#ifdef _DEBUG
		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);

		char buf[128];
		sprintf(buf, "BLIT: %I64d ticks/%I64d ticks per second\n", end.QuadPart-start.QuadPart, freq.QuadPart);
		OutputDebugString(buf);
		AddWarn(buf);
#endif
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDashboardDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDashboardDlg::OnOK() 
{
	// Ignore this
	return;
}

void CDashboardDlg::OnCancel() 
{
	// Ignore this
	return;
}

void CDashboardDlg::OnClose() 
{
#ifdef USE_DIRECTX
	takedownDirectDraw();
#endif

	CDialog::OnClose();

#ifndef NO_OBD
	CloseHandle(hPort);
	hPort=NULL;
#endif

	CDialog::EndDialog(IDOK);
}

// States
enum {
	sTIMEOUT=-99,
	sSLEEP=-7,	// this must be set so that WAITISO is -1!
	sHELLO,
	sWAITHELLO,
	sHARDWARE,
	sWAITHARDWARE,
	sISO,
	sWAITISO,	// This must be -1!

	sRPM,		// this must be 0!
	sWAITRPM,
	sSPEED,
	sWAITSPEED,
	sTROUBLE,
	sWAITTROUBLE,
	sLOAD,
	sWAITLOAD,
	sTEMP,
	sWAITTEMP,
	sMAX,

	sLOOP=99
};

#if sRPM != 0
#error State enum does not line up correctly!
#endif

// States = check >-99 and < sMAX, then add sSLEEP
const char szStates[][16] = {
	"Sleep",
	"Hello",
	"WaitHello",
	"Hardware",
	"WaitHardware",
	"ISO",
	"WaitISO",
	"RPM",
	"WaitRPM",
	"Speed",
	"WaitSpeed",
	"Trouble",
	"WaitTrouble",
	"Load",
	"WaitLoad",
	"Temp",
	"WaitTemp",
};
// The next array is the loop we enter starting with step 4.
// We loop this way so we always sample speed and RPM, and
// cycle through any other stat we're interested in, but are less
// real time. :)
const int nCycle[] = {
	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
	sTROUBLE, sWAITTROUBLE, 
	sLOOP
	// If you wanted to sample more, it looks a little better to mix
	// speed and RPM sampling in between other calls. This particular
	// loop samples the trouble indicator only every 5 passes
};

const char *GetStateName(int nCommState) {
	static char buf[16]="";	
	int t;

	t=(nCommState<0?nCommState:nCycle[nCommState]);
	if ((t>sTIMEOUT)&&(t<sMAX)) {
		t-=sSLEEP;
		strcpy(buf, szStates[t]);
	} else {
		sprintf(buf, "%d", t);
	}

	return buf;
}

void CDashboardDlg::StartWinAmp() {
	STARTUPINFO pstart;
	PROCESS_INFORMATION pinfo;
	DWORD dwRet=0;
	int nCount=0;

	WinAmpStatus=WINAMP_STARTING;

	HWND hWnd=::FindWindow("Winamp v1.x", NULL);
	if (NULL == hWnd) {		// no need to restart if it's already up
		ZeroMemory(&pstart, sizeof(pstart));
		pstart.cb=sizeof(pstart);
		pstart.dwFlags=STARTF_USESHOWWINDOW;
		pstart.wShowWindow=SW_HIDE;
		while (!dwRet) {
			dwRet=CreateProcess(NULL, szWinAmp, NULL, NULL, FALSE, 0, NULL, NULL, &pstart, &pinfo);
			if (!dwRet) {
				AddLog("WinAmp failed to start, code 0x%08x", GetLastError());
				nCount++;
				if (nCount > 3) {
					AddWarn("Giving up on WinAmp|Last code 0x%08x", GetLastError());
					return;
				}
				Sleep(2000);
			}
		}

		// I guess technically we could hold onto this process handle to find WinAmp later,
		// but searching dynamically isn't hard and is more flexible if WinAmp crashes or
		// was already started.
		CloseHandle(pinfo.hProcess);
		CloseHandle(pinfo.hThread);
	}
}

void CDashboardDlg::MonitorSerialThread() {
#ifndef NO_OBD
	DWORD nCnt;
#endif

	// track last noted trouble code
	static int nLastTroubleCode=0;
	// State variable to keep track of what we need to do
	static int nCommState=sHELLO;	
	// Track how many errors in a row - if too many, we reset
	static int nTimeoutErrors=0;

	// Read our preferences for timeouts
	int SleepTime, HelloTimeout, ISOTimeout, RPMTimeout, SpeedTimeout;
	int TroubleTimeout, LoadTimeout, TempTimeout;
	SleepTime     = GetPrivateProfileInt("Timeouts", "Sleep", 40, ".\\Dashboard.ini");
	HelloTimeout  = GetPrivateProfileInt("Timeouts", "Hello", 12, ".\\Dashboard.ini");
	ISOTimeout    = GetPrivateProfileInt("Timeouts", "ISO",  200, ".\\Dashboard.ini");
	RPMTimeout    = GetPrivateProfileInt("Timeouts", "RPM",   1, ".\\Dashboard.ini");
	SpeedTimeout  = GetPrivateProfileInt("Timeouts", "Speed", 1, ".\\Dashboard.ini");
	TroubleTimeout= GetPrivateProfileInt("Timeouts","Trouble",1, ".\\Dashboard.ini");
	LoadTimeout   = GetPrivateProfileInt("Timeouts", "Load",  1, ".\\Dashboard.ini");
	TempTimeout   = GetPrivateProfileInt("Timeouts", "Temp",  1, ".\\Dashboard.ini");
	GetPrivateProfileString("External", "NoteFile", "", szExtFile, MAX_PATH, ".\\Dashboard.ini");
	GetPrivateProfileString("External", "WinAmp", "", szWinAmp, MAX_PATH, ".\\Dashboard.ini");
	strcpy(szLastWinAmp, "");
	WinAmpStatus=WINAMP_NONE;

	// If we need to, start WinAmp
	if (strlen(szWinAmp)) {
		StartWinAmp();
	}

#ifndef NO_OBD
	int idx;
	// Create a handle for serial communication
	// open the serial port
	for (idx=0; idx<3; idx++) {
		hPort=CreateFile("COM1:", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
		if (NULL == hPort) {
			AfxMessageBox("Failed to open COM1");
			continue;
		}
		break;
	}
	if (idx >= 3) {
		AfxMessageBox("Can't open communication port - giving up.");
		EndDialog(IDCANCEL);
		return;
	}

	// Configure the serial port for 19200, 8N1
	DCB PortDCB;

	// Initialize the DCBlength member. 
	PortDCB.DCBlength = sizeof (DCB); 

	// Get the default port setting information.
	GetCommState (hPort, &PortDCB);

	// Change the DCB structure settings.
	PortDCB.BaudRate = CBR_19200;         // Current baud 
	PortDCB.fBinary = TRUE;               // Binary mode; no EOF check 
	PortDCB.fParity = FALSE;              // Enable parity checking 
	PortDCB.fOutxCtsFlow = FALSE;         // No CTS output flow control 
	PortDCB.fOutxDsrFlow = FALSE;         // No DSR output flow control 
	PortDCB.fDtrControl = DTR_CONTROL_ENABLE; 
										  // DTR flow control type 
	PortDCB.fDsrSensitivity = FALSE;      // DSR sensitivity 
	PortDCB.fTXContinueOnXoff = TRUE;     // XOFF continues Tx 
	PortDCB.fOutX = FALSE;                // No XON/XOFF out flow control 
	PortDCB.fInX = FALSE;                 // No XON/XOFF in flow control 
	PortDCB.fErrorChar = FALSE;           // Disable error replacement 
	PortDCB.fNull = FALSE;                // Disable null stripping 
	PortDCB.fRtsControl = RTS_CONTROL_ENABLE; 
										  // RTS flow control 
	PortDCB.fAbortOnError = FALSE;        // Do not abort reads/writes on 
										  // error
	PortDCB.ByteSize = 8;                 // Number of bits/byte, 4-8 
	PortDCB.Parity = NOPARITY;            // 0-4=no,odd,even,mark,space 
	PortDCB.StopBits = ONESTOPBIT;        // 0,1,2 = 1, 1.5, 2 

	// Configure the port according to the specifications of the DCB 
	// structure.
	if (!SetCommState (hPort, &PortDCB))
	{
		// Could not configure the serial port.
		AfxMessageBox("Unable to configure the serial port");
//		EndDialog(IDCANCEL);
		return;
	}

	// Set timeouts
	COMMTIMEOUTS cto;
	cto.ReadIntervalTimeout=MAXDWORD;	// timeout immediately on read
	cto.ReadTotalTimeoutConstant=0;
	cto.ReadTotalTimeoutMultiplier=0;
	cto.WriteTotalTimeoutConstant=0;	// No timeout on write
	cto.WriteTotalTimeoutMultiplier=0;
	if (!SetCommTimeouts(hPort, &cto)) {
		AfxMessageBox("Failed to set port timeouts");
		EndDialog(IDCANCEL);
		return;
	}

	if (!SetCommMask(hPort, EV_RXCHAR)) {
		AfxMessageBox("Failed to set port event mask.");
		EndDialog(IDCANCEL);
		return;
	}

	// All done
	InterlockedExchange(&nCommTimeout, 0);

	// Most of the cases in this switch can be condensed into a function later, 
	// when this is actually working.
	for (;;) {
		// Overlapped isn't worth the effort here, and without
		// it, WaitCommEvent becomes fairly complex, since the
		// WriteFiles are synchronous. So hell with it. We poll.
		Sleep(55);	// We spend nearly 100% of our time in kernel without this!
		nCommTimeout++;
	
		// The early state changes without setting the listen event are okay,
		// we only need the boost when we're deliberately listening, not for errors
		switch (nCommState<0?nCommState:nCycle[nCommState]) {
		case sTIMEOUT: // Timeout
			nTimeoutErrors++;
			if (nTimeoutErrors > 3) {
				AddWarn("Timeout. Resetting.|Previous state = %s", GetStateName(nCommTimeout));
				nCommState=sSLEEP;
			} else {
				nCommState=nCommTimeout++;
			}
			break;
		
		case sSLEEP:	// -7= Sleep (only used in reset)
			if (nCommTimeout > MS55*SleepTime) {
				nCommTimeout=0;
				nCommState++;
				nTimeoutErrors=0;
			}
			break;

		case sHELLO:	// -6= Send 0x20 byte as Hello
			WriteFile(hPort, "\x20", 1, &nCnt, NULL);
			nCommTimeout=0;
			nCommState++;
			bufidx=0;
			break;

		case sWAITHELLO:	// -5= Wait for 0xFF from car
			nCnt=0;
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x.", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}
			if (nCnt > 0) {
				int i;
				bufidx+=nCnt;
				nCommTimeout=0;
				for (i=0; i<bufidx; i++) {
					if (inp[i] == 0xff) break;
				}
				if (i<bufidx) {
					// Got it :)
					AddLog("Setting up data link...");
					bufidx=0;
					nCommState++;
					nTimeoutErrors=0;
					break;
				}
			}
			if (nCommTimeout > MS55*HelloTimeout) {
				nCommTimeout=nCommState;
				nCommState=sTIMEOUT;
			}
			break;

		case sHARDWARE:	// -4= Send >41, >00 to select VPW hardware? 
			WriteFile(hPort, "\x41\x00", 2, &nCnt, NULL);
			nCommTimeout=0;
			nCommState++;
			bufidx=0;
			break;

		case sWAITHARDWARE:	// -3= Wait for car to reply with xx,yy,yy. High bit of X is set on error. Low nibble of X is byte count. yy is expected to be 0
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}
			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
				if (bufidx >= 2) {
					if (inp[0]&0x80) {
						AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], inp[2], GetStateName(nCommState));
						nCommState=sSLEEP;
						break;
					}
					if (bufidx >= (inp[0]&0xf)) {
						AddLog("Hardware connect.");
						nCommState++;
						nTimeoutErrors=0;
						break;
					}
				}
				if (nCommState != -1) {
					break;
				}
			}

			// This is allowed to take up to 5 seconds!
			if (nCommTimeout > MS55*ISOTimeout) {
				nCommTimeout=nCommState;
				nCommState=sTIMEOUT;
			}
			break;		


		case sISO:	// -2= Send >42, >02, >33 to select ISO protocol ?
			WriteFile(hPort, "\x42\x02\x33", 3, &nCnt, NULL);
			nCommTimeout=0;
			nCommState++;
			bufidx=0;
			break;

		case sWAITISO:	// -1= Wait for car to reply with xx,yy,yy. High bit of X is set on error. Low nibble of X is byte count. yy is the ISO key #
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}
			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
				if (bufidx >= 2) {
					if (inp[0]&0x80) {
						AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], inp[2], GetStateName(nCommState));
						nCommState=sSLEEP;
						break;
					}
					if (bufidx >= (inp[0]&0xf)) {
						int nKey=0;
						for (int i=1; i<bufidx; i++) {
							nKey<<=8;
							nKey|=inp[i];
						}
						AddLog("Established. ISO Key 0x%X", nKey);
						nCommState++;
						nTimeoutErrors=0;
						break;
					}
				}
				if (nCommState != sWAITISO) {
					break;
				}
			}

			// This is allowed to take up to 5 seconds!
			if (nCommTimeout > MS55*ISOTimeout) {
				nCommTimeout=nCommState;
				nCommState=sTIMEOUT;
			}
			break;		

		case sRPM:	// 4 = Request RPM - We always expect this, even if the engine isn't running
			    //cnt address mode var chksum
			    // -- -------- --  --  --
				// 07 68 6A F1 01  0C  D0
			    //    61?
			WriteFile(hPort, "\x06\x68\x6a\xf1\x01\x0c\xd0", 7, &nCnt, NULL);
			nCommTimeout=1-RPMTimeout;
			nCommState++;
			bufidx=0;
			break;

		case sWAITRPM:	// 5 = Wait for RPM reply
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}

			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
			}

			if (nCommTimeout > MS55) {
				unsigned char chk;
				int idx;

				// Process the buffer
				if (bufidx == 0) {
					// No data
					nCommTimeout=nCommState;
					nCommState=sTIMEOUT;
					break;
				}
				// Error from car?
				if (inp[0]&0x80) {
					AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Not enough data?
				if (bufidx < 8) {
					AddWarn("Partial data rx|- data %d, %d - In state %s.", 8, bufidx, GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Checksum?
				chk=0;
				for (idx=0; idx<bufidx-1; idx++) {
					chk+=inp[idx];
				}
				if (chk != inp[bufidx-1]) {
					AddWarn("Bad checksum.|- data 0x%02x, 0x%02x - In state %s.", chk, inp[bufidx-1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Right variable?
				if (inp[4]!=0x0c) {
					AddWarn("Incorrect variable|- data 0x%02x, 0x%02x - In state %s.", 0x0c, inp[4], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Well, this must be it!
				ActualRPM=((inp[5]<<8)|inp[6])/4;
				nCommState++;
				nTimeoutErrors=0;
			}
			break;		

		case sSPEED:	// 6 = Request Speed - we always expect this, even if the engine isn't running
			    //cnt address mode var chksum
			    // -- -------- --  --  --
				// 07 68 6A F1 01  0D  D1
			    //    61?
			WriteFile(hPort, "\x06\x68\x6a\xf1\x01\x0d\xd1", 7, &nCnt, NULL);
			nCommTimeout=1-SpeedTimeout;
			nCommState++;
			bufidx=0;
			break;

		case sWAITSPEED:	// 7 = Wait for Speed reply
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}

			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
			}

			if (nCommTimeout > MS55) {
				unsigned char chk;
				int idx;

				// Process the buffer
				if (bufidx == 0) {
					// No data
					nCommTimeout=nCommState;
					nCommState=sTIMEOUT;
					break;
				}
				// Error from car?
				if (inp[0]&0x80) {
					AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Not enough data?
				if (bufidx < 7) {
					AddWarn("Partial data rx|- data %d, %d - In state %s.", 7, bufidx, GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Checksum?
				chk=0;
				for (idx=0; idx<bufidx-1; idx++) {
					chk+=inp[idx];
				}
				if (chk != inp[bufidx-1]) {
					AddWarn("Bad checksum.|- data 0x%02x, 0x%02x - In state %s.", chk, inp[bufidx-1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Right variable?
				if (inp[4]!=0x0d) {
					AddWarn("Incorrect variable|- data 0x%02x, 0x%02x - In state %s.", 0x0d, inp[4], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Well, this must be it!
				ActualSpeed=inp[5];
				nCommState++;
				nTimeoutErrors=0;
			}
			break;		

		case sTROUBLE:	// 8 = Request Trouble codes
			    //cnt address mode var chksum
			    // -- -------- --  --  --
				// 07 68 6A F1 02  02  CD
			    //    61?
			WriteFile(hPort, "\x06\x68\x6a\xf1\x02\x02\xcd", 7, &nCnt, NULL);
			nCommTimeout=1-TroubleTimeout;
			nCommState++;
			bufidx=0;
			break;

		case sWAITTROUBLE:	// 9 = Wait for trouble code reply
				// Note that the car is allowed to ignore this one unless there are errors stored! :)
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}

			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
			}

			if (nCommTimeout > MS55) {
				unsigned char chk;
				int idx;

				// Process the buffer
				if (bufidx == 0) {
					// No data - ignore that
					nCommState++;
					break;
				}
				// Error from car?
				if (inp[0]&0x80) {
					AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Not enough data?
				if (bufidx < 9) {	// Might be 8 per the docs??
					AddWarn("Partial data rx|- data %d, %d - In state %s.", 9, bufidx, GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Checksum?
				chk=0;
				for (idx=0; idx<bufidx-1; idx++) {
					chk+=inp[idx];
				}
				if (chk != inp[bufidx-1]) {
					AddWarn("Bad checksum.|- data 0x%02x, 0x%02x - In state %s.", chk, inp[bufidx-1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Right variable?
				if (inp[4]!=0x02) {
					AddWarn("Incorrect variable|- data 0x%02x, 0x%02x - In state %s.", 0x02, inp[4], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Well, this must be it!
				idx=(inp[6]<<8)|inp[7];
				if (idx != 0) {
					// There is a trouble code!
					if (idx!=nLastTroubleCode) {
						char out[16];

						nLastTroubleCode=idx;
						switch (idx&0xc000) {
						case 0x0000:	out[0]='P'; break;
						case 0x4000:	out[0]='C'; break;
						case 0x8000:	out[0]='B'; break;
						case 0xC000:	out[0]='U'; break;
						}
						out[1]=((idx&0x3000)>>12)+'0';
						out[2]=((idx&0x0f00)>>8)+'0';
						out[3]=((idx&0x00f0)>>4)+'0';
						out[4]=(idx&0x000f)+'0';
						out[5]='\0';

						AddWarn("System code %s|",out);
					}
				}
				nCommState++;
				nTimeoutErrors=0;
			}
			break;		

		case sLOAD: // 10= Request engine load - this may be ignored if the engine is not running!
			    //cnt address mode var chksum
			    // -- -------- --  --  --
				// 07 68 6A F1 01  04  C7
			    //    61?
			WriteFile(hPort, "\x06\x68\x6a\xf1\x01\x04\xc7", 7, &nCnt, NULL);
			nCommTimeout=1-LoadTimeout;
			nCommState++;
			bufidx=0;
			break;

		case sWAITLOAD: // 11= Wait for engine load reply
			// NOTE that the car may ignore this one!
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}

			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
			}

			if (nCommTimeout > MS55) {
				unsigned char chk;
				int idx;

				// Process the buffer
				if (bufidx == 0) {
					// No data - never mind, then
					nCommState++;
					break;
				}
				// Error from car?
				if (inp[0]&0x80) {
					AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Not enough data?
				if (bufidx < 7) {
					AddWarn("Partial data rx|- data %d, %d - In state %s.", 7, bufidx, GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Checksum?
				chk=0;
				for (idx=0; idx<bufidx-1; idx++) {
					chk+=inp[idx];
				}
				if (chk != inp[bufidx-1]) {
					AddWarn("Bad checksum.|- data 0x%02x, 0x%02x - In state %s.", chk, inp[bufidx-1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Right variable?
				if (inp[4]!=0x04) {
					AddWarn("Incorrect variable|- data 0x%02x, 0x%02x - In state %s.", 0x04, inp[4], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Well, this must be it!
				ActualLoad=(inp[5]*100)/255;
				nCommState++;
				nTimeoutErrors=0;
			}
			break;		

		case sTEMP:	// 12= Request coolant temperature - We expect this even if the engine is not running
			    //cnt address mode var chksum
			    // -- -------- --  --  --
				// 07 68 6A F1 01  05  C8
			    //    61?
			WriteFile(hPort, "\x06\x68\x6a\xf1\x01\x05\xc8", 7, &nCnt, NULL);
			nCommTimeout=1-TempTimeout;
			nCommState++;
			bufidx=0;
			break;

		case sWAITTEMP:	// 13= Wait for coolant temperature reply
			if (!ReadFile(hPort, &inp[bufidx], 128-bufidx, &nCnt, NULL)) {
				AddWarn("Serial read failed.|In state %s, code 0x%08x", GetStateName(nCommState), GetLastError());
				nCommState=sSLEEP;
				break;
			}

			if (nCnt > 0) {
				bufidx+=nCnt;
				nCommTimeout=0;
			}

			if (nCommTimeout > MS55) {
				unsigned char chk;
				int idx;

				// Process the buffer
				if (bufidx == 0) {
					// No data
					nCommTimeout=nCommState;
					nCommState=sTIMEOUT;
					break;
				}
				// Error from car?
				if (inp[0]&0x80) {
					AddWarn("Communication error.|Or command reject - data 0x%02x, 0x%02x - In state %s.", inp[0], inp[1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Not enough data?
				if (bufidx < 7) {
					AddWarn("Partial data rx|- data %d, %d - In state %s.", 7, bufidx, GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Checksum?
				chk=0;
				for (idx=0; idx<bufidx-1; idx++) {
					chk+=inp[idx];
				}
				if (chk != inp[bufidx-1]) {
					AddWarn("Bad checksum.|- data 0x%02x, 0x%02x - In state %s.", chk, inp[bufidx-1], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Right variable?
				if (inp[4]!=0x05) {
					AddWarn("Incorrect variable|- data 0x%02x, 0x%02x - In state %s.", 0x05, inp[4], GetStateName(nCommState));
					// we'll try this one again next pass
					nCommState++;
					break;
				}
				// Well, this must be it!
				ActualTemp=inp[5]-40;
				nCommState++;
				nTimeoutErrors=0;
			}
			break;		
		
		case sLOOP: // Loop
			nCommState=0;
			break;

		default: // oops.. how did we get here?
			AddWarn("Invalid state %d.|", nCommState);
			nCommState=sSLEEP;
			break;
		}

		if (fLastExtButton!=ReadPIOPaperOut()) {
			fLastExtButton=ReadPIOPaperOut();
			fWinAmpNext=true;
			WinAmpStatus=WINAMP_NEXT;
		}
	}
#else
	// NO_OBD is defined
	for (;;) {
		Sleep(55);
		nCommTimeout++;

		ActualSpeed=(int)(sin(nCommTimeout/100.0)*MAXSPEED);
		if (ActualSpeed < 0) ActualSpeed*=-1;
		ActualRPM=(int)(cos(nCommTimeout/112.0)*MAXRPM);
		if (ActualRPM < 0) ActualRPM*=-1;

		if (nCommTimeout > 9999) {
			break;
		}

		if (fLastExtButton!=ReadPIOPaperOut()) {
			fLastExtButton=ReadPIOPaperOut();
			fWinAmpNext=true;
			WinAmpStatus=WINAMP_NEXT;
		}
	}
#endif
}

void CDashboardDlg::OnTimer(UINT nIDEvent) 
{
	char szWorkBuffer[LOGLINES*(LOGLENGTH+2)];
	static bool Inited=false;
	static int nSecondCnt=0;

	// Beyond this point, stop if it's not our update timer
	if (1 != nIDEvent) {
		return;
	}

	if (UpdateCountdown > 0) {
		// This lets us freeze the display with all text lit up
		UpdateCountdown--;

		if (UpdateCountdown == (FPS/2)) {
			// Clear the speed
			CurrentSpeed=0;
		}

		if (UpdateCountdown == 0) {
			// Clear the temp displays
			time(&StartTime);
			dist=0;
		}

	} else {
		// Scaling code
		if (ActualSpeed > CurrentSpeed) {
			CurrentSpeed+=min(ActualSpeed-CurrentSpeed,40);
		}
		if (ActualSpeed < CurrentSpeed) {
			CurrentSpeed-=min(CurrentSpeed-ActualSpeed,40);
		}
		
		if (ActualRPM > CurrentRPM+50) {
			CurrentRPM+=min(ActualRPM-CurrentRPM,900);
		}
		if (ActualRPM < CurrentRPM) {
			CurrentRPM-=min(CurrentRPM-ActualRPM,900);
		}

		// Update distance travelled - use more accurate counters
		// (even though we're guessing at the speed? oh well)
		// (This proved to work quite well on a long (>4000km) trip!)
		static LARGE_INTEGER nLastCount={0,0}, nFreq={0,0};

		if (nFreq.QuadPart==0) {
			// first call - setup
			QueryPerformanceFrequency(&nFreq);
			QueryPerformanceCounter(&nLastCount);
		} else {
			LARGE_INTEGER nThisCount;
			double nThisDiff;
			// We're good
			QueryPerformanceCounter(&nThisCount);
			nThisDiff=(nThisCount.QuadPart-nLastCount.QuadPart)/(double)(nFreq.QuadPart);
			if (nThisDiff > 0.1) {
				// Evaluate every 1/10th of a second
				dist+=(ActualSpeed/3600.0)*nThisDiff;		// km/sec * seconds elapsed
				nLastCount=nThisCount;
			}
		}

		// Update Time/Date/Trip every FPS loops
		static int nCnt=0;

		nCnt++;
		if (nCnt >= FPS) {
			time_t t;
			struct tm *local;	
			static char szOldPic[256]="";
			
			nSecondCnt++;

			// Tell windows we're using the machine, thank you, don't blank or shut down
			SetThreadExecutionState(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED);

			// Do Time
			time(&t);
			local=localtime(&t);

			strftime(szDate, 16, "%m/%d/%Y", local);
			strftime(szTime, 16, "%H:%M", local);

			if (nSecondCnt%10 == 0) {
				// Every 10 Seconds, check for popups
				ScanForMsgBoxes();
			}

			if ((nSecondCnt%30 == 0)||(NULL == hWebcam)||(strcmp(szOldPic, szMiniPic))) {
				// every 30 seconds or so
				if (NULL != hWebcam) {
					DeleteObject(hWebcam);
					hWebcam=NULL;
				}
				hWebcam=LoadImage(NULL, szMiniPic, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			}

			nCnt=0;

			DWORD x=(DWORD)t-(DWORD)StartTime;
			double diff=x/3600.0;
			int difftime=(int)(diff*10);
			int disttime=(int)dist;

			switch (distanceMode) {
			case 0:
				sprintf(szTrip, "TRIP: %d.%dhrs - %d km", difftime/10,difftime%10, disttime);
				break;
			case 1:
				sprintf(szTrip, "TRIP: %d.%dhrs - %d mi", difftime/10,difftime%10, mph(disttime));
				break;
			case 2:
				sprintf(szTrip, "TRIP: %d.%dhrs - %d Mf", difftime/10,difftime%10, fpf(disttime)/10);
			}

			// Check our external file - if it exists, we need to load it's notes into the log!
			if (strlen(szExtFile)) {
				HANDLE hFile;		// Use Win32 for somewhat better synchronization

				// Greedy - we want to open the file for all rights with no sharing (so we can nuke it)
				hFile=CreateFile(szExtFile, GENERIC_ALL, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE, NULL);

				if (INVALID_HANDLE_VALUE != hFile) {
					// We got one :)
					char buf[2048];		// max data is 2k
					DWORD dwRead;

					if (ReadFile(hFile, buf, 2048, &dwRead, NULL)) {
						CString out;
						DWORD parsed=0;

						buf[dwRead]='\0';
						while (parsed < dwRead) {
							out="";
							while ((buf[parsed])&&(buf[parsed]!='\n')&&(buf[parsed]!='\r')) {
								if (buf[parsed]>=' ') {
									out+=buf[parsed++];
								} else {
									parsed++;
								}
							}
							if (out.GetLength() > 0) {
								AddLog("%s",out);
							}
							while ((buf[parsed]=='\n')||(buf[parsed]=='\r')) {
								parsed++;
							}
						}
					}
					CloseHandle(hFile);
				}
			}

			// Work with WinAmp
			if (strlen(szWinAmp)) {
				HWND hWnd=::FindWindow("Winamp v1.x", NULL);
				if (NULL != hWnd) {
					int res;
					// Update status
					if (WINAMP_STARTING == WinAmpStatus) {
						WinAmpStatus=WINAMP_NONE;
					}
					// Check playing state
					res = ::SendMessage(hWnd, WM_WA_IPC, 0, IPC_ISPLAYING);
					if (1 == res) {
						// playing
						if (!fWinAmpPlaying) {	// if it's playing and shouldn't be, stop it
							::SendMessage(hWnd, WM_COMMAND, WINAMP_KEY_PAUSE, 0);
							if (WINAMP_PAUSE == WinAmpStatus) {
								WinAmpStatus=WINAMP_NONE;
							}
						} else {
							WinAmpStatus=WINAMP_NONE;
						}
					} else {
						// stopped or paused
						if (res == 3) {
							WinAmpStatus=WINAMP_PAUSED;
						} else {
							WinAmpStatus=WINAMP_STOPPED;
						}
						if (fWinAmpPlaying) {		// if it's not playing and should be, we start it
							::SendMessage(hWnd, WM_COMMAND, WINAMP_KEY_PLAY, 0);
							WinAmpStatus=WINAMP_PLAY;
						}
					}

					// Check shuffle state
					res = ::SendMessage(hWnd, WM_WA_IPC, 0, IPC_GET_SHUFFLE);
					if (res) {
						if (!fWinAmpRandom) {		// Turn off shuffle
							::SendMessage(hWnd, WM_WA_IPC, 0, IPC_SET_SHUFFLE);
							if (WinAmpStatus == WINAMP_SHUFFLE_OFF) {
								WinAmpStatus=WINAMP_NONE;
							}
						}
					} else {
						if (fWinAmpRandom) {		// Turn ON shuffle
							::SendMessage(hWnd, WM_WA_IPC, 1, IPC_SET_SHUFFLE);
							if (WinAmpStatus == WINAMP_SHUFFLE_ON) {
								WinAmpStatus=WINAMP_NONE;
							}
						}
					}

					// Check for Prev/next commands
					if (fWinAmpPrev) {
						fWinAmpPrev=false;
						::SendMessage(hWnd, WM_COMMAND, WINAMP_KEY_PREV, 0);
						if (WINAMP_PREV == WinAmpStatus) {
							WinAmpStatus=WINAMP_NONE;
						}
					}

					if (fWinAmpNext) {
						fWinAmpNext=false;
						::SendMessage(hWnd, WM_COMMAND, WINAMP_KEY_NEXT, 0);
						if (WINAMP_NEXT == WinAmpStatus) {
							WinAmpStatus=WINAMP_NONE;
						}
					}
					
					// Get the current song title
					int nPos=::SendMessage(hWnd, WM_WA_IPC, 0, IPC_GETLISTPOS);
					void *pText=(void*)::SendMessage(hWnd, WM_WA_IPC, nPos, IPC_GETPLAYLISTTITLE);
					if (NULL != pText) {
						// Now, NullSoft says we can't use this from other processes, But the trick is,
						// you see, we just have to read the memory from their process, it still gives
						// us a valid pointer! (Just not valid to US!) 
						char myBuf[64];		// don't want more than 64 chars anyway, and I use the first 3!
						DWORD dwPID;		// WinAmp PID
						HANDLE hProc;		// Handle to process

						// First, we need the WinAmp process handle.
						::GetWindowThreadProcessId(hWnd, &dwPID);
						hProc=OpenProcess(PROCESS_VM_READ, FALSE, dwPID);
						if (NULL != hProc) {
							strcpy(myBuf, "-> ");
							// Then, we just read it's memory!
							if (ReadProcessMemory(hProc, pText, &myBuf[3], 61, NULL)) {
								// This could still fail if there are not 61 bytes allocated there,
								// but I'm not walking their heap just to check this! It works most (all?)
								// of the time - they probably have a large enough buffer anyway.
								myBuf[63]='\0';
								if (strcmp(szLastWinAmp, myBuf)) {
									AddLog("%s", myBuf);
									strcpy(szLastWinAmp, myBuf);
								}
							}
							CloseHandle(hProc);
						} else {
							DWORD dw=GetLastError();
							dw=dw;
						}
					}
					// Similarly, get the current path so we can try for a music bitmap (instead of the webcam)
					pText=(void*)::SendMessage(hWnd, WM_WA_IPC, nPos, IPC_GETPLAYLISTFILE);
					if (NULL != pText) {
						// warning: pointer to another process memory space
						char myBuf[256];	// don't want more than 256 chars anyway
						DWORD dwPID;		// WinAmp PID
						HANDLE hProc;		// Handle to process

						// First, we need the WinAmp process handle.
						::GetWindowThreadProcessId(hWnd, &dwPID);
						hProc=OpenProcess(PROCESS_VM_READ, FALSE, dwPID);
						if (NULL != hProc) {
							// Then, we just read it's memory!
							if (ReadProcessMemory(hProc, pText, myBuf, 256, NULL)) {
								// This could still fail if there are not 256 bytes allocated there,
								// but I'm not walking their heap just to check this! It works most (all?)
								// of the time - they probably have a large enough buffer anyway.
								myBuf[255]='\0';
								// First, check for file.bmp
								char *pDot=strrchr(myBuf, '.');
								if (pDot) {
									*(pDot+1)='b';
									*(pDot+2)='m';
									*(pDot+3)='p';
									*(pDot+4)='\0';
									HANDLE htest=CreateFile(myBuf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
									if (INVALID_HANDLE_VALUE != htest) {
										strcpy(szMiniPic, myBuf);
									} else {
										// then check for a folder.bmp
										pDot=strrchr(myBuf, '\\');
										if (pDot) {
											*(pDot+1)='\0';
											strcat(myBuf, "folder.bmp");
											strcpy(szMiniPic, myBuf);
										}
									}
								}
							}
							CloseHandle(hProc);
						} else {
							DWORD dw=GetLastError();
							dw=dw;
						}
					}
				} else {
					// Doesn't seem to be running! Start it again.
					StartWinAmp();
				}
			}
		}
	}

	// If WinAmp is starting, draw less often to reduce CPU load till it's up!
	if (WINAMP_STARTING == WinAmpStatus) {
		if (nSecondCnt%2 != 0) {
			return;
		}
	}

	// Redraw our work buffer
	CDC tmpDC;
	CBitmap *pOrgBmp, *pTmpBmp, *pMaskBmp;
	int tmpval;

#ifdef _DEBUG
		LARGE_INTEGER start, end, freq;
		QueryPerformanceCounter(&start);
#endif

	tmpDC.CreateCompatibleDC(&WorkDC);
	pTmpBmp=CBitmap::FromHandle((HBITMAP)hBackground);
	pOrgBmp=tmpDC.SelectObject(pTmpBmp);

	// Background
	WorkDC.BitBlt(0, 0, 800, 600, &tmpDC, 0, 0, SRCCOPY);

	// Gauges 
	tmpval=(int)(min(CurrentRPM, MAXRPM)*TACHRATIO);
	tmpval-=(tmpval%16);
	pMaskBmp=CBitmap::FromHandle((HBITMAP)hTachMask);
	pTmpBmp=CBitmap::FromHandle((HBITMAP)hTach);
	tmpDC.SelectObject(pTmpBmp);
	WorkDC.MaskBlt(0, 0, tmpval, 600, &tmpDC, 0, 0, *pMaskBmp, 0, 0, MAKEROP4(SRCCOPY,SRCPAINT));
	WorkDC.MaskBlt(tmpval, 0, 5, 600, &tmpDC, tmpval, 0, *pMaskBmp, tmpval, 0, MAKEROP4(BLACKNESS, SRCPAINT));

	tmpval=(int)(min(CurrentSpeed, MAXSPEED)*SPEEDRATIO);
	tmpval-=(tmpval%16);
	pMaskBmp=CBitmap::FromHandle((HBITMAP)hSpeedMask);
	pTmpBmp=CBitmap::FromHandle((HBITMAP)hSpeed);
	tmpDC.SelectObject(pTmpBmp);
	WorkDC.MaskBlt(0, 0, tmpval, 600, &tmpDC, 0, 0, *pMaskBmp, 0, 0, MAKEROP4(SRCCOPY, SRCPAINT));
	WorkDC.MaskBlt(tmpval, 0, 5, 600, &tmpDC, tmpval, 0, *pMaskBmp, tmpval, 0, MAKEROP4(BLACKNESS, SRCPAINT));

	// Text
	RECT rect;
	bool flag;
	WorkDC.SetBkMode(TRANSPARENT);

	// Log
	WorkDC.SelectObject(&LogFont);
	rect.left=LOGX;
	rect.top=LOGY;
	rect.right=800;
	rect.bottom=600;

	WorkDC.SetTextColor(myRGB(LOGCOLOR));
	flag=false;
	char *pDat=szWorkBuffer;
	for (int idx=0; idx<LOGLINES; idx++) {
		if (strncmp(szLog[idx], "Warning:", 8)) {
			int l=strlen(szLog[idx]);
			memcpy(pDat, szLog[idx], l);
			pDat+=l;
		} else {
			flag=true;
		}
		*(pDat++)='\r';
		*(pDat++)='\n';
	}
	*pDat='\0';
	WorkDC.DrawText(szWorkBuffer, pDat-szWorkBuffer, &rect, TEXTFLAGS);

	if (flag) {
		WorkDC.SetTextColor(myRGB(WARNCOLOR));
		char *pDat=szWorkBuffer;
		for (int idx=0; idx<LOGLINES; idx++) {
			if (!strncmp(szLog[idx], "Warning:", 8)) {
				int l=strlen(szLog[idx]);
				memcpy(pDat, szLog[idx], l);
				pDat+=l;
			}
			*(pDat++)='\r';
			*(pDat++)='\n';
		}
		*pDat='\0';
		WorkDC.DrawText(szWorkBuffer, pDat-szWorkBuffer, &rect, TEXTFLAGS);
	}

	// Speedometer (MUST come after log as they overlap at times)
	WorkDC.SetTextColor(myRGB(SPEEDCOLOR));
	rect.left=SPEEDX;
	rect.top=SPEEDY;

	WorkDC.SelectObject(&SpeedFont);
	switch (distanceMode) {
	case 0:
		sprintf(szWorkBuffer, "%3d", CurrentSpeed);
		break;
	case 1:
		sprintf(szWorkBuffer, "%3d", mph(CurrentSpeed));
		break;
	case 2:
		sprintf(szWorkBuffer, "%2d.%1d", fpf(CurrentSpeed)/10,fpf(CurrentSpeed)%10);
		break;
	}
	WorkDC.DrawText(szWorkBuffer, strlen(szWorkBuffer), &rect, TEXTFLAGS);

	rect.left=SCALEX;
	rect.top=SCALEY;
	WorkDC.SelectObject(&ScaleFont);
	
	if (UpdateCountdown) {
		WorkDC.DrawText("km/h", 4, &rect, TEXTFLAGS);
		rect.top+=32;
		WorkDC.DrawText("mph", 3, &rect, TEXTFLAGS);
		rect.top+=32;
		WorkDC.DrawText("Mff", 3, &rect, TEXTFLAGS);
	} else {
		switch (distanceMode) {
		case 0:
			WorkDC.DrawText("km/h", 4, &rect, TEXTFLAGS);
			break;
		case 1:
			rect.top+=32;
			WorkDC.DrawText("mph", 3, &rect, TEXTFLAGS);
			break;
		case 2:
			rect.top+=64;
			WorkDC.DrawText("Mff", 3, &rect, TEXTFLAGS);
			break;
		}
	}

	// Clock - this text gets centered, so the RECT is a little more work
	WorkDC.SetTextColor(myRGB(DATECOLOR));
	
	rect.left=DATEXCENTER-(800-DATEXCENTER);
	rect.top=DATEY;
	WorkDC.SelectObject(&DateFont);
	WorkDC.DrawText(szDate, strlen(szDate), &rect, TEXTFLAGS|DT_CENTER);

	rect.left=CLOCKXCENTER-(800-CLOCKXCENTER);
	rect.top=CLOCKY;
	WorkDC.SelectObject(&ClockFont);
	WorkDC.DrawText(szTime, strlen(szTime), &rect, TEXTFLAGS|DT_CENTER);

	rect.left=TRIPXCENTER-(800-TRIPXCENTER);
	rect.top=TRIPY;
	WorkDC.SelectObject(&TripFont);
	WorkDC.DrawText(szTrip, strlen(szTrip), &rect, TEXTFLAGS|DT_CENTER);

	// WinAmp status text, if any
	if (WinAmpStatus) {
		WorkDC.SetTextColor(myRGB(WINAMPCOLOR));
		WorkDC.SelectObject(&LogFont);
		rect.left=0;
		rect.top=WINAMPY;
		rect.right=799;
		rect.bottom=600;

		WorkDC.DrawText(szWinAmpState[WinAmpStatus], strlen(szWinAmpState[WinAmpStatus]), &rect, TEXTFLAGS|DT_CENTER);
	}

	// Webcam
	if (NULL != hWebcam) {
		pTmpBmp=CBitmap::FromHandle((HBITMAP)hWebcam);
		tmpDC.SelectObject(pTmpBmp);
		WorkDC.BitBlt(WEBCAMX, WEBCAMY, 130, 130, &tmpDC, 0, 0, SRCCOPY);
	}

	// Done!
	tmpDC.SelectObject(pOrgBmp);

#ifdef _DEBUG
		QueryPerformanceCounter(&end);
		QueryPerformanceFrequency(&freq);

		char buf[128];
		sprintf(buf, "DRAW: %I64d ticks/%I64d ticks per second\n", end.QuadPart-start.QuadPart, freq.QuadPart);
		OutputDebugString(buf);
		AddWarn(buf);
#endif

	InvalidateRect(NULL, false);

}

////////////////////////////////////////////////////////////
// Setup DirectDraw, 800x600x16 fullscreen mode
// In order for Fullscreen to work, only the main thread
// may call this function!
////////////////////////////////////////////////////////////
void CDashboardDlg::SetupDirectDraw() {
	int x,y,c;

	if (DirectDrawCreateEx(NULL, (void**)&lpdd, IID_IDirectDraw7, NULL)!=DD_OK) {
		MessageBox("Unable to initialize DirectDraw 7 - Requires DirectX 7 or higher", "Dashboard Error", MB_OK);
		lpdd=NULL;
	} else {
		DDSURFACEDESC2 myDesc;

		x=800; y=600; c=16;

		// Check if mode is legal
		ZeroMemory(&myDesc, sizeof(myDesc));
		myDesc.dwSize=sizeof(myDesc);
		myDesc.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
		myDesc.dwWidth=x;
		myDesc.dwHeight=y;
		lpdd->EnumDisplayModes(0, &myDesc, (void*)&c, myCallBack);
		// If a valid mode was found, 'c' has 0x80 ORd with it
		if (0 == (c&0x80)) {
			MessageBox("Requested graphics mode is not supported on the primary display.", "Dashboard Error", MB_OK);
			if (lpdd) lpdd->Release();
			lpdd=NULL;
			goto optout;
		}

		c&=0x7f;	// Remove the flag bit

		if (lpdd->SetCooperativeLevel(GetSafeHwnd(), DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_FULLSCREEN | DDSCL_ALLOWMODEX)!=DD_OK) {
			MessageBox("Unable to set cooperative level\nFullscreen DX is not available", "Dashboard Error", MB_OK);
			if (lpdd) lpdd->Release();
			lpdd=NULL;
			goto optout;
		}

		if (lpdd->SetDisplayMode(x,y,c,0,0) != DD_OK) {
			MessageBox("Unable to set display mode.\nRequested DX mode is not available", "Dashboard Error", MB_OK);
			goto optout;
		}

		DDSURFACEDESC2 ddsd;

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize=sizeof(ddsd);
		ddsd.dwFlags=DDSD_CAPS;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;

		if (lpdd->CreateSurface(&ddsd, &lpdds, NULL) !=DD_OK) {
			MessageBox("Unable to create primary surface\nDX mode is not available", "Dashboard Error", MB_OK);
			if (lpdd) lpdd->Release();
			lpdd=NULL;
			goto optout;
		}

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize=sizeof(ddsd);
		ddsd.dwFlags=DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.dwWidth=800;
		ddsd.dwHeight=600;

		if (lpdd->CreateSurface(&ddsd, &ddsBack, NULL) !=DD_OK) {
			MessageBox("Unable to create back buffer surface\nDX mode is not available", "Dashboard Error", MB_OK);
			ddsBack=NULL;
			lpdds->Release();
			lpdds=NULL;
			lpdd->Release();
			lpdd=NULL;
			goto optout;
		}

optout: ;
	}
}

////////////////////////////////////////////////////////////
// Release all references to DirectDraw objects
////////////////////////////////////////////////////////////
void CDashboardDlg::takedownDirectDraw() {	
	if (NULL != ddsBack) ddsBack->Release();
	ddsBack=NULL;
	if (NULL != lpdds) lpdds->Release();
	lpdds=NULL;
	if (NULL != lpdd) lpdd->Release();
	lpdd=NULL;
}

void CDashboardDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CDialog::OnLButtonDown(nFlags, point);

	// We have several hot-spots for clicking (touching with a touch screen!)
	// Top Left - WinAmp Previous
	// Top Right - WinAmp Next
	// Extreme Top Left - Volume adjust (sndvol32)
	// Extreme Top Right - Shutdown System (all of windows)
	// Top Center - Play/Pause
	// Bottom Left - Winamp Shuffle toggle
	// Bottom Right - Miles/KM
	// This function only sets flags, to prevent deadlock, except for shutdown
	
	if (point.y < 50) {
		// 'extreme' top - early out if matched
		if (point.x < 50) {
			// extreme top left
			ShellExecute(GetSafeHwnd(), "open", "sndvol32.exe", NULL, NULL, SW_SHOWNORMAL);
			return;
		}
		if (point.x > 750) {
#ifndef _DEBUG
			// extreme top right (assumes 800x600)
			// need to get priviledges to shut down (MSDN sample)
			HANDLE hToken; 
			TOKEN_PRIVILEGES tkp; 
 
			// Get a token for this process. 
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
				AddLog("Failed to OpenProcessToken - can't shut down"); 
			} else {
 				// Get the LUID for the shutdown privilege. 
				LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
 				tkp.PrivilegeCount = 1;  // one privilege to set    
				tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
				// Get the shutdown privilege for this process. 
				AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
 				// Cannot test the return value of AdjustTokenPrivileges directly
				if (GetLastError() != ERROR_SUCCESS) {
					AddLog("Failed to AdjustTokenPrivileges - can't shut down"); 
				} else {
					if (!ExitWindowsEx(EWX_POWEROFF|EWX_FORCEIFHUNG, 0)) {
						AddLog("Failed to initiate shutdown.");
					} else {
						EndDialog(IDOK);
					}
				}
			}
			return;
#else
			EndDialog(IDOK);
#endif
		}
	}

	if (point.y < 300) {
		// top
		if (point.x < 266) {
			// top left
			fWinAmpPrev=true;
			WinAmpStatus=WINAMP_PREV;
		} else if (point.x > 534) {
			// top right
			fWinAmpNext=true;
			WinAmpStatus=WINAMP_NEXT;
		} else {
			// top center
			fWinAmpPlaying=!fWinAmpPlaying;
			if (!fWinAmpPlaying) {
				WinAmpStatus=WINAMP_PAUSE;
			} else {
				WinAmpStatus=WINAMP_PLAY;
			}
		}
	} else {
		// bottom
		if (point.x < 400) {
			// bottom left
			fWinAmpRandom=!fWinAmpRandom;
			if (fWinAmpRandom) {
				WinAmpStatus=WINAMP_SHUFFLE_ON;
			} else {
				WinAmpStatus=WINAMP_SHUFFLE_OFF;
			}
		} else {
			// bottom right
			distanceMode=distanceMode++;
			if (distanceMode == 3) {
				distanceMode=0;
			}
			switch (distanceMode) {
			case 0:
				AddLog("Selected kilometers per hour"); 
				break;
			case 1:
				AddLog("Selected miles per hour");
				break;
			case 2:
				AddLog("Selected mega furlongs per fortnight");
				break;
			}
		}
	}
}

// Read paper out (pin 12) and return it's state
// This requires UserPort or similar driver under 2k/XP to work
// If not available, we'll catch the exception and do nothing
bool CDashboardDlg::ReadPIOPaperOut() {

	return false;
#if 0
	int byte;

	try {
		byte=_inp(PIO_BASE+1);

		// from this byte:
		// d0 = n/c
		// d1 = n/c
		// d2 = n/c
		// d3 = !ERROR (pin 15)
		// d4 = SELECTED (pin 13)
		// d5 = PAPER OUT (pin 12) - used!
		// d6 = ACK (pin 10)
		// d7 = !BUSY (pin 11)
		return ((byte & 0x20) != 0);
	}
	catch( char * ) {
		return false;
	}
#endif
}
