// Dashboard.h : main header file for the DASHBOARD application
//

#if !defined(AFX_DASHBOARD_H__1F122039_2C8B_47B5_976F_EE9715993ADB__INCLUDED_)
#define AFX_DASHBOARD_H__1F122039_2C8B_47B5_976F_EE9715993ADB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDashboardApp:
// See Dashboard.cpp for the implementation of this class
//

class CDashboardApp : public CWinApp
{
public:
	CDashboardApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDashboardApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDashboardApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DASHBOARD_H__1F122039_2C8B_47B5_976F_EE9715993ADB__INCLUDED_)
