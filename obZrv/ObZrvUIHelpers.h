#pragma once
#include <afxtoolbarmenubutton.h>
#include "../AulddaysDpiHelper/AulddaysDpiHelper.h"

// Fixes default MFC UI promblems

// Fix CMFCToolBarMenuButton bMenuOnly not working when the button has an ID
class ObZrvToolBarMenuButton : public CMFCToolBarMenuButton
{
	typedef CMFCToolBarMenuButton super;
	DECLARE_SERIAL(ObZrvToolBarMenuButton)
public:
	ObZrvToolBarMenuButton() : super() {}
	ObZrvToolBarMenuButton(UINT uiID, HMENU hMenu, int iImage, LPCTSTR lpszText = NULL, BOOL bUserButton = FALSE)
		: super(uiID, hMenu, iImage, lpszText, bUserButton) {}
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnClick(CWnd* pWnd, BOOL bDelay);
};

// Disable toolbar customization
class ObZrvToolBar : public AulddaysToolBar
{
	typedef AulddaysToolBar super;
	DECLARE_DYNAMIC(ObZrvToolBar)
public:
	virtual BOOL LoadState(LPCTSTR lpszProfileName, int nIndex, UINT uiID);
	virtual BOOL SaveState(LPCTSTR lpszProfileName, int nIndex, UINT uiID);
};

