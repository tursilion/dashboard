// RawOBD.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include <windows.h>
#include <varargs.h>


void MonitorSerialThread();

char szNoteFile[MAX_PATH];


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

	sLOOP=99,
	sDELAY,
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
//	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
//	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
//	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
//	sRPM, sWAITRPM, sSPEED, sWAITSPEED, 
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sTROUBLE, sWAITTROUBLE, 
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sTROUBLE, sWAITTROUBLE, 
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sTROUBLE, sWAITTROUBLE, 
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sTROUBLE, sWAITTROUBLE, 
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sDELAY,sDELAY,sDELAY,sDELAY,sDELAY,			// 5s delay
	sTROUBLE, sWAITTROUBLE, 
	sLOOP
	// If you wanted to sample more, it looks a little better to mix
	// speed and RPM sampling in between other calls. This particular
	// loop samples the trouble indicator only every 5 passes
};

#define AddWarn AddLog
void AddLog(char *sz, ...) {
	char buf[MAX_PATH];
	va_list va;

	va_start(va, sz);
	_vsnprintf(buf, MAX_PATH, sz, va);
	buf[MAX_PATH-1]='\0';
	va_end(va);
	printf("%s\n", buf);
	
	FILE *fp=fopen(szNoteFile, "a");
	if (NULL != fp) {
		fprintf(fp, "%s\n", buf);
		fclose(fp);
	} else {
		printf("Can't open file %s\n", szNoteFile);
	}
}

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
const char *GetStateName2(int nCommState) {
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

void MonitorSerialThread() {
	DWORD nCnt;
	char szComPort[MAX_PATH];
	volatile long nCommTimeout;
	int bufidx;
	unsigned char inp[256];
	HANDLE hPort;
	int ActualRPM;
	int ActualSpeed;
	int ActualLoad;
	int ActualTemp;

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
	GetPrivateProfileString("General", "OBDPort", "COM1:", szComPort, MAX_PATH, ".\\Dashboard.ini");
	GetPrivateProfileString("External", "NoteFile", "", szNoteFile, MAX_PATH, ".\\Dashboard.ini");

	int idx;
	// Create a handle for serial communication
	// open the serial port
	for (idx=0; idx<3; idx++) {
		hPort=CreateFile(szComPort, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
		if (NULL == hPort) {
			printf("Failed to open COM %s", szComPort);
			continue;
		}
		break;
	}
	if (idx >= 3) {
		printf("Can't open communication port - giving up.");
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
		printf("Unable to configure the serial port");
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
		printf("Failed to set port timeouts");
		return;
	}

	if (!SetCommMask(hPort, EV_RXCHAR)) {
		printf("Failed to set port event mask.");
		return;
	}

	// All done
	InterlockedExchange(&nCommTimeout, 0);

	// Most of the cases in this switch can be condensed into a function later, 
	// when this is actually working.
	for (;;) {
		static int nOldState = -999;

		// Overlapped isn't worth the effort here, and without
		// it, WaitCommEvent becomes fairly complex, since the
		// WriteFiles are synchronous. So hell with it. We poll.
		Sleep(55);	// We spend nearly 100% of our time in kernel without this!
		nCommTimeout++;

//		if (nOldState != nCommState) {
//			printf("STATE: %s -> %s\n", GetStateName(nOldState), GetStateName2(nCommState));
//			nOldState=nCommState;
//		}
	
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
			if (nCommTimeout > SleepTime) {
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
			if (nCommTimeout > HelloTimeout) {
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
			if (nCommTimeout > ISOTimeout) {
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
			if (nCommTimeout > ISOTimeout) {
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

			if (nCommTimeout > 1) {
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

			if (nCommTimeout > 1) {
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

			if (nCommTimeout > 1) {
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

			if (nCommTimeout > 1) {
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

			if (nCommTimeout > 1) {
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
			// write the data
			AddLog("Engine: %d RPM  Speed: %d km/h", ActualRPM, ActualSpeed);
			nCommState=0;
			break;

		case sDELAY:	// wait a second
			Sleep(1000);
			break;

		default: // oops.. how did we get here?
			AddWarn("Invalid state %d.|", nCommState);
			nCommState=sSLEEP;
			break;
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	MonitorSerialThread();

	return 0;
}

