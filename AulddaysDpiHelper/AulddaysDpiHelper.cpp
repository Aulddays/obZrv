// BSD 3 - Clause License
// 
// Copyright(c) 2020, Aulddays, https://dev.aulddays.com/
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and / or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// AulddaysDpiHelper
// A utility to help applications to better support Per-Monitor DPI Awareness (V2).
// That is, scale screen elements correctly on systems with multiple monitors of
// different DPI settings.
// For detailed documentation, visit:
// https://dev.aulddays.com/article/mfc-per-monitor-dpi-v2-blurry-text.htm

#include "stdafx.h"
#include "AulddaysDpiHelper.h"

std::list<AulddaysDpiHelper::AulddaysDpiData> AulddaysDpiHelper::_dpidata;
std::map<UINT, AulddaysDpiHelper::AulddaysDpiData *> AulddaysDpiHelper::_dpimap;
bool AulddaysDpiHelper::_inited = false;
UINT(WINAPI *AulddaysDpiHelper::AulddaysGetDpiForWindow)(HWND hwnd) = NULL;
BOOL(WINAPI *AulddaysDpiHelper::AulddaysSystemParametersInfoForDpi)(
	UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
int (WINAPI *AulddaysDpiHelper::AulddaysGetSystemMetricsForDpi)(int nIndex, UINT dpi);

// A helper to modify protected members of AFX_GLOBAL_DATA
struct AFX_GLOBAL_DATA_HELPER : public AFX_GLOBAL_DATA
{
	static void set_dblRibbonImageScale(double v)
	{
		((AFX_GLOBAL_DATA_HELPER *)GetGlobalData())->m_dblRibbonImageScale = v;
	}
};

void AulddaysDpiHelper::init()
{
	HMODULE user32 = LoadLibraryW(L"user32");
	AulddaysGetDpiForWindow = (decltype(AulddaysGetDpiForWindow))GetProcAddress(user32, "GetDpiForWindow");
	AulddaysSystemParametersInfoForDpi = (decltype(AulddaysSystemParametersInfoForDpi))
		GetProcAddress(user32, "SystemParametersInfoForDpi");
	AulddaysGetSystemMetricsForDpi = (decltype(AulddaysGetSystemMetricsForDpi))
		GetProcAddress(user32, "GetSystemMetricsForDpi");
	if (!AulddaysGetDpiForWindow || !AulddaysGetSystemMetricsForDpi || !AulddaysSystemParametersInfoForDpi)
		AulddaysGetDpiForWindow = [](HWND hwnd) -> UINT { return 0; };
	_inited = true;
}

void AulddaysDpiHelper::initdata(UINT dpi)
{
	if (_dpimap.count(dpi) && _dpimap[dpi])
		return;
	_dpidata.emplace_back();
	AulddaysDpiData &data = _dpidata.back();
	_dpimap[dpi] = &data;

	LOGFONT lf = { 0 };

	if (dpi == 0)
	{
		// just copy from AFX_GLOBAL_DATA
		GetGlobalData()->fontRegular.GetLogFont(&lf);
		data.fontRegular.CreateFontIndirect(&lf);
		GetGlobalData()->fontSmall.GetLogFont(&lf);
		data.fontSmall.CreateFontIndirect(&lf);
		GetGlobalData()->fontTooltip.GetLogFont(&lf);
		data.fontTooltip.CreateFontIndirect(&lf);
		GetGlobalData()->fontUnderline.GetLogFont(&lf);
		data.fontUnderline.CreateFontIndirect(&lf);
		GetGlobalData()->fontBold.GetLogFont(&lf);
		data.fontBold.CreateFontIndirect(&lf);
		GetGlobalData()->fontMarlett.GetLogFont(&lf);
		data.fontMarlett.CreateFontIndirect(&lf);
		GetGlobalData()->fontVert.GetLogFont(&lf);
		data.fontVert.CreateFontIndirect(&lf);
		GetGlobalData()->fontVertCaption.GetLogFont(&lf);
		data.fontVertCaption.CreateFontIndirect(&lf);
		data.m_bIsRibbonImageScale = GetGlobalData()->IsRibbonImageScaleEnabled();
		data.m_dblRibbonImageScale = GetGlobalData()->GetRibbonImageScale();
		return;
	}

	// get dpi metrics
	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);
	AulddaysSystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, info.cbSize, &info, 0, dpi);

	// Regular font
	GetGlobalData()->fontRegular.GetLogFont(&lf);
	lf.lfHeight = info.lfMenuFont.lfHeight;
	// Adjust regular font size:
	int nFontHeight = lf.lfHeight < 0 ?
		-lf.lfHeight : lf.lfHeight;
	if (nFontHeight <= 12)
		nFontHeight = 11;
	else if (!GetGlobalData()->m_bDontReduceFontHeight)
		nFontHeight--;
	lf.lfHeight = (lf.lfHeight < 0) ? -nFontHeight : nFontHeight;
	data.fontRegular.CreateFontIndirect(&lf);
	// Small font:
	LONG lfHeightSaved = lf.lfHeight;
	lf.lfHeight = (long)((1. + abs(lf.lfHeight)) * 2 / 3);
	if (lfHeightSaved < 0)
		lf.lfHeight = -lf.lfHeight;
	data.fontSmall.CreateFontIndirect(&lf);
	lf.lfHeight = lfHeightSaved;
	// Tooltip font:
	lf.lfItalic = info.lfStatusFont.lfItalic;
	lf.lfWeight = info.lfStatusFont.lfWeight;
	data.fontTooltip.CreateFontIndirect(&lf);
	lf.lfItalic = info.lfMenuFont.lfItalic;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	// "underline" font:
	lf.lfUnderline = TRUE;
	data.fontUnderline.CreateFontIndirect(&lf);
	lf.lfUnderline = FALSE;
	// Bold font:
	lf.lfWeight = FW_BOLD;
	data.fontBold.CreateFontIndirect(&lf);
	// Marlett font
	GetGlobalData()->fontMarlett.GetLogFont(&lf);
	lf.lfHeight = AulddaysGetSystemMetricsForDpi(SM_CYMENUCHECK, dpi) - 1;
	data.fontMarlett.CreateFontIndirect(&lf);

	// Vertical font:
	GetGlobalData()->fontVert.GetLogFont(&lf);
	lf.lfHeight = info.lfMenuFont.lfHeight;
	data.fontVert.CreateFontIndirect(&lf);
	lf.lfEscapement = 900;
	data.fontVertCaption.CreateFontIndirect(&lf);

	// ribbon image scale
	data.m_bIsRibbonImageScale = GetGlobalData()->IsRibbonImageScaleEnabled();
	data.m_dblRibbonImageScale = dpi / 96.0;
	if (data.m_dblRibbonImageScale > 1. && data.m_dblRibbonImageScale < 1.1)
		data.m_dblRibbonImageScale = 1.;
}

void AulddaysDpiHelper::AulddaysDpiData::release()
{
	if (fontRegular.GetSafeHandle())
		::DeleteObject(fontRegular.Detach());
	if (fontSmall.GetSafeHandle())
		::DeleteObject(fontSmall.Detach());
	if (fontTooltip.GetSafeHandle())
		::DeleteObject(fontTooltip.Detach());
	if (fontUnderline.GetSafeHandle())
		::DeleteObject(fontUnderline.Detach());
	if (fontBold.GetSafeHandle())
		::DeleteObject(fontBold.Detach());
	if (fontMarlett.GetSafeHandle())
		::DeleteObject(fontMarlett.Detach());
	if (fontVert.GetSafeHandle())
		::DeleteObject(fontVert.Detach());
	if (fontVertCaption.GetSafeHandle())
		::DeleteObject(fontVertCaption.Detach());
}

void AulddaysDpiHelper::updateGlobal(UINT dpi)
{
	if (dpi == 0)	// no permointor dpi v2 support, just behave default
		return;

	// get dpi metrics
	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);
	AulddaysSystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, info.cbSize, &info, 0, dpi);

	// fonts
	LOGFONT lf = { 0 };
	GetGlobalData()->fontRegular.GetLogFont(&lf);	// Inherit from the original font but only change the height 
	lf.lfHeight = info.lfMenuFont.lfHeight;
	// Adjust regular font size:
	int nFontHeight = lf.lfHeight < 0 ?
		-lf.lfHeight : lf.lfHeight;
	if (nFontHeight <= 12)
		nFontHeight = 11;
	else if (!GetGlobalData()->m_bDontReduceFontHeight)
		nFontHeight--;
	lf.lfHeight = (lf.lfHeight < 0) ? -nFontHeight : nFontHeight;
	// this sets regular, bold, underline fonts, and also calls UpdateTextMetrics (m_nTextHeightHorz etc)
	GetGlobalData()->SetMenuFont(&lf, TRUE);
	// Small font:
	LONG lfHeightSaved = lf.lfHeight;
	lf.lfHeight = (long)((1. + abs(lf.lfHeight)) * 2 / 3);
	if (lfHeightSaved < 0)
		lf.lfHeight = -lf.lfHeight;
	GetGlobalData()->fontSmall.DeleteObject();
	GetGlobalData()->fontSmall.CreateFontIndirect(&lf);
	lf.lfHeight = lfHeightSaved;
	// Tooltip font:
	lf.lfItalic = info.lfStatusFont.lfItalic;
	lf.lfWeight = info.lfStatusFont.lfWeight;
	GetGlobalData()->fontTooltip.DeleteObject();
	GetGlobalData()->fontTooltip.CreateFontIndirect(&lf);
	lf.lfItalic = info.lfMenuFont.lfItalic;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	// Marlett font:
	GetGlobalData()->fontMarlett.GetLogFont(&lf);
	lf.lfHeight = AulddaysGetSystemMetricsForDpi(SM_CYMENUCHECK, dpi) - 1;
	GetGlobalData()->fontMarlett.DeleteObject();
	GetGlobalData()->fontMarlett.CreateFontIndirect(&lf);
	// vertical font
	GetGlobalData()->fontVert.GetLogFont(&lf);
	lf.lfHeight = info.lfMenuFont.lfHeight;
	GetGlobalData()->SetMenuFont(&lf, FALSE);

	// Toolbar button image scale
	double imageScale = dpi / 96.0;
	if (imageScale > 1. && imageScale < 1.1)
		imageScale = 1.;
	AFX_GLOBAL_DATA_HELPER::set_dblRibbonImageScale(imageScale);
}


// AulddaysToolBar

#ifndef AFX_TOOLBAR_BUTTON_MARGIN
#	define AFX_TOOLBAR_BUTTON_MARGIN 6
#endif

IMPLEMENT_DYNAMIC(AulddaysToolBar, CMFCToolBar)

BEGIN_MESSAGE_MAP(AulddaysToolBar, CMFCToolBar)
END_MESSAGE_MAP()

CSize AulddaysToolBar::CalcLayout(DWORD dwMode, int nLength)
{
	CSize ret = super::CalcLayout(dwMode, nLength);
	return ret;
}

void AulddaysToolBar::updateDpi()
{
	// If we were on a high dpi previously, the toolbar images should have been scaled up,
	// and changing back to a low dpi will cause the images to be scale back down, causing blurred images.
	// Calling m_Images.OnSysColorChange forces the toolbar images to be reloaded and the scale level
	// changed back to 1 (At least MFC versions in VS2013 to VS2019 all act like this, which may be a bug that
	// a real SysColorChange event may cause the toolbar images to be in incorrect size if on high dpi).
	// Then the upcoming SetSizes() will set the right button size as well as scale the reloaded images
	// to the right level.
	m_Images.OnSysColorChange();

	// Calculate new button sizes and image sizes
	CSize sizeImage(m_sizeImage);
	CSize sizeButton(sizeImage.cx + AFX_TOOLBAR_BUTTON_MARGIN, sizeImage.cy + AFX_TOOLBAR_BUTTON_MARGIN);
	BOOL bDontScaleImages = m_bLocked ? m_bDontScaleLocked : m_bDontScaleImages;
	if (!bDontScaleImages)
	{
		double dblImageScale = GetGlobalData()->GetRibbonImageScale();
		sizeButton = CSize((int)(.5 + sizeButton.cx * dblImageScale), (int)(.5 + sizeButton.cy * dblImageScale));
	}
	if (m_bLocked)
		SetLockedSizes(sizeButton, sizeImage);
	else
		SetSizes(sizeButton, sizeImage);

	AdjustLayout();
}
