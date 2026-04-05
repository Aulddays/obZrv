#include "stdafx.h"
#include <afxregpath.h>
#include "ObZrvUIHelpers.h"

IMPLEMENT_SERIAL(ObZrvToolBarMenuButton, CMFCToolBarMenuButton, VERSIONABLE_SCHEMA | 1)

void ObZrvToolBarMenuButton::Serialize(CArchive& ar)
{
	super::Serialize(ar);

	if (ar.IsLoading())
	{
		ar >> m_bMenuOnly;
	}
	else
	{
		ar << m_bMenuOnly;
	}
}

BOOL ObZrvToolBarMenuButton::OnClick(CWnd* pWnd, BOOL bDelay)
{
	// Fix: super::OnClick() does not respect m_bMenuOnly if the button has an ID and not in m_bMenuMode.
	// Temporarily set m_bMenuMode to true if m_bMenuOnly is true for workaround
	BOOL menuModeSuper = m_bMenuMode;
	m_bMenuMode = m_bMenuOnly || menuModeSuper;
	BOOL ret = super::OnClick(pWnd, bDelay);
	m_bMenuMode = menuModeSuper;
	return ret;
}

#define AFX_MFC_TOOLBAR_PROFILE _T("MFCToolBars")
#define AFX_REG_SECTION_FMT _T("%TsMFCToolBar-%d")
#define AFX_REG_SECTION_FMT_EX _T("%TsMFCToolBar-%d%x")
#define AFX_REG_ENTRY_BUTTONS _T("Buttons")
IMPLEMENT_DYNAMIC(ObZrvToolBar, AulddaysToolBar)

BOOL ObZrvToolBar::LoadState(LPCTSTR lpszProfileName, int nIndex, UINT uiID)
{
	BOOL bResult = CPane::LoadState(lpszProfileName, nIndex, uiID);
	AdjustLayout();
	if (m_pParentDockBar != NULL && m_pDockBarRow != NULL)
	{
		ASSERT_VALID(m_pParentDockBar);
		ASSERT_VALID(m_pDockBarRow);

		CSize sizeCurr = CalcFixedLayout(TRUE, IsHorizontal());
		m_pParentDockBar->ResizeRow(m_pDockBarRow, IsHorizontal() ? sizeCurr.cy : sizeCurr.cx);
	}
	return bResult;
}

BOOL ObZrvToolBar::SaveState(LPCTSTR lpszProfileName, int nIndex, UINT uiID)
{
	return CPane::SaveState(lpszProfileName, nIndex, uiID);
}