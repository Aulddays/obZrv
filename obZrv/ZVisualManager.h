#pragma once
#include <afxvisualmanagerwindows.h>
class ObZrvVisualManager : public CMFCVisualManagerWindows
{
	DECLARE_DYNCREATE(ObZrvVisualManager)
	typedef CMFCVisualManagerWindows super;

public:
	ObZrvVisualManager(BOOL bIsTemporary = FALSE) : super(bIsTemporary) { }
	virtual ~ObZrvVisualManager() {};

	virtual void OnFillBarBackground(CDC* pDC, CBasePane* pBar, CRect rectClient, CRect rectClip, BOOL bNCArea = FALSE);
};

