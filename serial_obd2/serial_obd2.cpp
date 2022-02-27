// serial_obd2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>

DWORD nCnt;
HANDLE hPort;
unsigned char buf[128];
int bufidx, ncount;

int main(int argc, char* argv[])
{
	// open the serial port
	hPort=CreateFile("COM1:", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);
	if (NULL == hPort) {
		printf("Failed to open COM1\n");
		return 0;
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
	  printf("Unable to configure the serial port\n");
	  return 0;
	}

	// Set timeouts
	COMMTIMEOUTS cto;
	cto.ReadIntervalTimeout=MAXDWORD;	// timeout immediately on read
	cto.ReadTotalTimeoutConstant=0;
	cto.ReadTotalTimeoutMultiplier=0;
	cto.WriteTotalTimeoutConstant=0;	// No timeout on write
	cto.WriteTotalTimeoutMultiplier=0;
	if (!SetCommTimeouts(hPort, &cto)) {
		printf("Failed to set port timeouts\n");
		return 0;
	}

	if (!SetCommMask(hPort, EV_RXCHAR)) {
		printf("Failed to set port event mask.");
		return 0;
	}


	bufidx=0;
	ncount=0;

	for (;;) {
		DWORD dwRead=0;

		if ((ReadFile(hPort, &buf[bufidx], 128-bufidx, &dwRead, NULL))&& (dwRead > 0)) {
			for (DWORD x=0; x<dwRead; x++) {
				printf("0x%02.2x ", (int)buf[bufidx+x]);
			}
		
scanning:
			if (ncount == 0) {
				ncount=buf[0]&0xf;
				if (ncount == 0) {
					if (buf[bufidx] == 0x20) {		// connect request
						WriteFile(hPort, "\xff", 1, &nCnt, NULL);
						printf("\nCONNECT - return 0xff");
						bufidx=0;
						dwRead=0;
					}
					// else, ignore
					printf("\n");
					continue;
				}
			}

			ncount-=dwRead;
			if (ncount < 1) {	// full request packet
				// else, ignore
				printf("\n");
				
				switch (buf[0]) {
				case 0x06:
				case 0x07:
					switch (buf[1]) {
					case 0x68:	// ISO Scantool request
						// the header in this case will be 686AF1
						// 68 - Priority/Type
						// 6A - Target address
						// F1 - Scantool address
						// The next bytes are the message itself. The
						// car will respond with 486Bxx
						// 48 - Priority/type
						// 6B - Target address
						// xx - ECU address (?)
						//
						// Next byte is the Mode (1-9)
						// Next is the PID request
						// The rest varies by request/response
						//
						// Last byte is the checksum (ISO) or CRC (others)
						// This includes all bytes except the count at the beginning
						// Note that replies don't include the length count - you
						// determine the end by timing out! (55ms recommended)
						//
						switch (buf[4]) {		// Mode select (1-9)
						case 1:		// Mode 1
							printf("MODE 1 REQUEST - ");
							switch (buf[5]) {	// following bytes are A B C D (if present)
							case 0x00:		// determine PIDs supported (bitmask 32 bits)
								// change this if we add more! :)
								printf("Request for supported PIDs");
								// Return the PIDs we use here
								WriteFile(hPort, "\x48\x6b\x99\x41\x00\x98\x18\x00\x10\x4d", 10, &nCnt, NULL);
								break;

							case 0x01:		// trouble code information (bitmask 32 bits)
								printf("Request for available test information");
								// A = LCCCCCCC, where L indicates Lamp is lit, and C represents the number of codes
								// B = Continuous tests, the low nibble is support, high nibble means incomplete
								//	   RCFM RCFM
								//     R - reserved
								//     C - Components
								//     F - fuel system
								//     M - misfire
								// C - Non-continuous tests, support
								//     EHOA 2VYC
								//	   E - EGR SystemC7
								//	   H - Oxygen Sensor Heater
								//     O - Oxygen Sensor
								//     A - A/C Refrigerant
								//     2 - Secondary Air system
								//     V - Evaporative system
								//     Y - Heated catalyst
								//	   C - Catalyst
								// D - non-continuous tests, incomplete
								//     Same as C, except the EGR bit is not used
								// Return no problems
								WriteFile(hPort, "\x48\x6b\x99\x41\x01\x00\x00\x00\x00\x98", 10, &nCnt, NULL);
								break;

							case 0x04:		// calculated engine load (A*100/255)
								printf("Engine load");	// 50%
								WriteFile(hPort, "\x48\x6b\x99\x41\x04\x80\x11", 7, &nCnt, NULL);
								break;

							case 0x05:		// coolant temp C (A-40)
								printf("Coolant temp");	// 88c
								WriteFile(hPort, "\x48\x6b\x99\x41\x05\x80\x12", 7, &nCnt, NULL);
								break;

							case 0x0c:		// Engine RPM (2 bytes) (.25*(A*256+B))
								printf("Engine RPM");	// 1000 RPM
								WriteFile(hPort, "\x48\x6b\x99\x41\x0c\x0F\xA0\x48", 8, &nCnt, NULL);
								break;

							case 0x0d:		// Speed km/h (1 byte) (A)
								printf("Speed");	// 100 km/h
								WriteFile(hPort, "\x48\x6b\x99\x41\x0d\x64\xfe", 7, &nCnt, NULL);
								break;

							case 0x1c:		// OBD requirements supported
								printf("OBD Design Requirements");
								// 1 - OBD II California ARB
								// 2 - OBD Federal EPA
								// 3 - OBD and OBD II
								// 4 - OBD 1
								// 5 - no OBD
								// 6 - EOBD Europe
								// Return OBD II
								WriteFile(hPort, "\x48\x6b\x99\x41\x1c\x01\xb1", 7, &nCnt, NULL);
								break;

							// unconfirmed
							case 0x20:		// Specifies a second bank of settings 	// determine PIDs supported (bitmask 32 bits)
								// TODO - 0x00 last bit says if it's supprted
								break;

							case 0x23:		// Fuel rail pressure (1=10kPa) (?)
								// TODO
								break;

							case 0x40:		// Specifies a third bank - 0x20 last bit says if it's supported
								// TODO (determine PIDs supported)
								break;

							case 0x42:		// Battery voltage?
								break;
							
							default:	
								printf("Unsupported request");
								break;
							}
							break;

						case 2:
							printf("MODE 2 REQUEST - ");
							switch (buf[5]) {	// following bytes are A B C D (if present)
							case 0x00:		// determine PIDs supported (bitmask 32 bits)
								// change this if we add more! :)
								printf("Request for supported PIDs");
								// Return the PIDs we use here
								WriteFile(hPort, "\x48\x6b\x99\x42\x00\x40\x00\x00\x00\xce", 10, &nCnt, NULL);
								break;

							case 0x02:
								// Freeze frame trouble code - means a problem occurred, else 0,0
								// Returns A and B for the last code, encoded like so:
								// LL11 2222 3333 4444
								// L is the category letter:
								//    00 - P
								//    01 - C
								//    10 - B
								//    11 - U
								// 1 is the first digit, 0-3
								// 2-4 are the second through fourth digits, 0-9
								// We'll send back no error for now. The car is allowed to ignore this
								// if there are no errors!
								printf("Freeze frame trouble code");
								WriteFile(hPort, "\x48\x6b\x99\x42\x02\x00\x91\x00\x21", 9, &nCnt, NULL);
								//                                    ^^^^ I don't know what this byte is for!
								break;

							default:
								printf("Unsupported.");
								break;
							}
							break;

						case 3:
							printf("MODE 3 REQUEST - ");
							break;

						case 4:
							printf("MODE 4 REQUEST - ");
							break;

						case 5:
							printf("MODE 5 REQUEST - ");
							break;

						case 6:
							printf("MODE 6 REQUEST - ");
							break;

						case 7:
							printf("MODE 7 REQUEST - ");
							break;

						case 8:
							printf("MODE 8 REQUEST - ");
							break;

						case 9:
							printf("MODE 9 REQUEST - ");
							// Mostly manufacturer specific stats, but the VIN is standard (not supported on the Neon though)
							if (buf[5] == 0x00) {
								// request for VIN
								printf("VIN");
								WriteFile(hPort, "\x48\x6b\x99\x49\x00\x01\x02\x03\x04\x9f", 10, &nCnt, NULL);
							} else {
								printf("Unsupported");
							}
							break;

						default:
							printf("UNKNOWN MODE");
							break;
						}

						printf("\n");
					}
					break;

				// Contrary to the chip data sheet, two communications are required. First,
				// a 0x41, 0x00 sets the chip mode to VPW, then a 0x42, 0x02 seems to request
				// the ISO variant. (Hardware and software modes?)
				// Looks like the 0x4x sets the mode, and the following data (x bytes)..?
				case 0x41:
				case 0x42:
					// select protocol
					switch (buf[1]) {
					case 0:	printf("REQUESTED PROTOCOL VPW\n"); break;
					case 1:	printf("REQUESTED PROTOCOL PWM\n"); break;
					case 2:	printf("REQUESTED PROTOCOL ISO 9141\n"); break;
					default:printf("REQUESTED PROTOCOL unknown %d\n", buf[1]); break; 
					}
					// Reply with a status response
					// We reply with ISO key 8, like the Neon does
					WriteFile(hPort, "\x01\x08", 2, &nCnt, NULL);

					if (bufidx+dwRead > 2) {
						memmove(buf, &buf[2], bufidx+dwRead-2);
						dwRead-=2;
						bufidx-=2;
						bufidx+=dwRead;
						goto scanning;
					}
					break;
				}

				ncount=0;
				bufidx=0;
				dwRead=0;
			}
		} else {
			Sleep(0);
		}
				
		bufidx+=dwRead;
	}

	return 0;

}
