#include "stdafx.h"
#include "ZVisualManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(ObZrvVisualManager, CMFCVisualManagerWindows)

void ObZrvVisualManager::OnFillBarBackground(CDC* pDC, CBasePane* pBar, CRect rectClient, CRect rectClip, BOOL bNCArea)
{
	ASSERT_VALID(pBar);
	ASSERT_VALID(pDC);
	CMFCVisualManagerOfficeXP::OnFillBarBackground(pDC, pBar, rectClient, rectClip, bNCArea);
}