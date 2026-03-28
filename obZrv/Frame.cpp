// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020, 2021 Aulddays (https://dev.aulddays.com/). All rights reserved.
//
// This file is part of obZrv.
// 
// obZrv is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// obZrv is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with obZrv. If not, see <https://www.gnu.org/licenses/>.

// Frame.cpp : implementation of the ObZrvFrm class
//

#include "stdafx.h"
#include "obZrv.h"

#include "Frame.h"
#include "ZVisualManager.h"

#include "../AulddaysDpiHelper/AulddaysDpiHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ObZrvFrm

IMPLEMENT_DYNCREATE(ObZrvFrm, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(ObZrvFrm, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &ObZrvFrm::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &ObZrvFrm::OnToolbarCreateNew)
	ON_WM_SETTINGCHANGE()
	ON_MESSAGE(WM_DPICHANGED, &ObZrvFrm::OnDpichanged)
	ON_COMMAND(ID_INDICATOR_IMAGEINFO, NULL)	// status bar pane will be grayed out if missing this
END_MESSAGE_MAP()

// ObZrvFrm construction/destruction

ObZrvFrm::ObZrvFrm()
{
	// TODO: add member initialization code here
}

ObZrvFrm::~ObZrvFrm()
{
}

int ObZrvFrm::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	AulddaysDpiHelper::updateGlobal(GetSafeHwnd());
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	static UINT indicators[] =
	{
		//ID_SEPARATOR,           // status line indicator
		ID_INDICATOR_IMAGEINFO,
		//ID_INDICATOR_CAPS,
		//ID_INDICATOR_NUM,
		//ID_INDICATOR_SCRL,
	};
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
	m_wndStatusBar.SetPaneInfo(0, ID_INDICATOR_IMAGEINFO, SBPS_NORMAL | SBPS_STRETCH, 1);

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);


	// set the visual manager and style based on persisted value
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(ObZrvVisualManager));
	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	//if (CMFCToolBar::GetUserImages() == NULL)
	//{
	//	// load user-defined toolbar images
	//	if (m_UserImages.Load(_T(".\\UserImages.bmp")))
	//	{
	//		CMFCToolBar::SetUserImages(&m_UserImages);
	//	}
	//}

	updateDpi();

	return 0;
}

BOOL ObZrvFrm::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

// ObZrvFrm diagnostics

#ifdef _DEBUG
void ObZrvFrm::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void ObZrvFrm::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// ObZrvFrm message handlers

void ObZrvFrm::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT ObZrvFrm::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}


BOOL ObZrvFrm::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}


	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}

void ObZrvFrm::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);

	// TODO: Add your message handler code here
	updateDpi();
}

afx_msg LRESULT ObZrvFrm::OnDpichanged(WPARAM wParam, LPARAM lParam)
{
	updateDpi();
	return 0;
}

void ObZrvFrm::updateDpi()
{
	// Resize of the whole window

	// update AFX_GLOBAL_DATA
	AulddaysDpiHelper::updateGlobal(GetSafeHwnd());

	// update child windows
	m_wndMenuBar.AdjustLayout();
	m_wndToolBar.updateDpi();
	m_wndStatusBar.AdjustLayout();
}

void ObZrvFrm::SetInfoText(const wchar_t *text)
{
	m_wndStatusBar.SetPaneText(0, text);
}

