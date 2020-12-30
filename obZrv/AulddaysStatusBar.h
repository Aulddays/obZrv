// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020 Aulddays (https://dev.aulddays.com/). All rights reserved.
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

#pragma once

// AulddaysStatusBar

class AulddaysStatusBar : public CMFCStatusBar
{
	DECLARE_DYNAMIC(AulddaysStatusBar)

public:
	AulddaysStatusBar();
	virtual ~AulddaysStatusBar();
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz)
	{
		ASSERT_VALID(this);

		// recalculate based on font height + icon height + borders
		TEXTMETRIC tm;
		{
			CClientDC dcScreen(NULL);
			HFONT hFont = GetCurrentFont();

			HGDIOBJ hOldFont = dcScreen.SelectObject(hFont);
			VERIFY(dcScreen.GetTextMetrics(&tm));
			dcScreen.SelectObject(hOldFont);
		}

		int cyIconMax = 0;
		CMFCStatusBarPaneInfo* pSBP = (CMFCStatusBarPaneInfo*)m_pData;
		for (int i = 0; i < m_nCount; i++, pSBP++)
		{
			cyIconMax = max(cyIconMax, pSBP->cyIcon);
		}

		CRect rectSize;
		rectSize.SetRectEmpty();
		CalcInsideRect(rectSize, bHorz);    // will be negative size

		// sizeof text + 1 or 2 extra on top, 2 on bottom + borders
		int t1 = rectSize.Height();
		int t2 = max(cyIconMax, tm.tmHeight) + AFX_CY_BORDER * 4 - rectSize.Height();
		return CSize(32767, max(cyIconMax, tm.tmHeight) + 8 - rectSize.Height());
	}

protected:
	DECLARE_MESSAGE_MAP()
};


