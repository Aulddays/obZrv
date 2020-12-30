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

#pragma once
#include <map>
#include <list>
#include <Windows.h>
#include <afxwin.h>

class AulddaysDpiHelper
{
public:
	AulddaysDpiHelper() {};
	virtual ~AulddaysDpiHelper() {};

	/////////////////
	// EASY APIs
	// Update the global settings and automatically affect most MFC elements
	/////////////////

	// update global settings according to the HWND of the main window
	static void updateGlobal(HWND hwnd)
	{
		if (!_inited)
			init();
		updateGlobal(AulddaysGetDpiForWindow(hwnd));
	}

	// update according to a specific dpi value
	static void updateGlobal(UINT dpi);

	// Get the dpi value according to the HWND of the main window
	// A return value of 0 indicates the current OS does not support Per-monitor DPI Awareness and
	// the values from the default AFX_GLOBAL_DATA should just be used
	static UINT getDpi(HWND hwnd)
	{
		if (!_inited)
			init();
		return AulddaysGetDpiForWindow(hwnd);
	}

	// Get the value to which elements' sizes should be scaled, according to the HWND of the main window
	// A return value of 0 indicates the current OS does not support Per-monitor DPI Awareness and
	// the values from the default AFX_GLOBAL_DATA should just be used
	static double getScale(HWND hwnd)
	{
		return getDpi(hwnd) / 96.0;
	}


	/////////////////
	// ADVANCED APIs
	// Manage metrics to use on different dpi settings, supporting multiple windows of a single process
	// on monitors with different dpi settings at the same time to scale correctly
	/////////////////

	// AulddaysDpiData keeps values similar to AFX_GLOBAL_DATA but for a specific dpi
	struct AulddaysDpiData
	{
		friend AulddaysDpiHelper;
		CFont fontRegular;
		CFont fontSmall;
		CFont fontTooltip;
		CFont fontUnderline;
		CFont fontBold;
		CFont fontMarlett;
		CFont fontVert;
		CFont fontVertCaption;
		~AulddaysDpiData() { release(); };
		void release();

	protected:
		double m_dblRibbonImageScale = 1;
		BOOL   m_bIsRibbonImageScale = TRUE;
	};

	// get the AulddaysDpiData values for a specific HWND 
	static AulddaysDpiData *get(HWND hwnd)
	{
		return get(AulddaysGetDpiForWindow(hwnd));
	}

	// get the AulddaysDpiData values for a specific dpi value
	static AulddaysDpiData *get(UINT dpi)
	{
		if (!_inited)
			init();
		if (!_dpimap[dpi])
			initdata(dpi);
		return _dpimap[dpi];
	}

protected:
	// Global initializer, managing pointers to dpi related winapis
	static bool _inited;
	static void init();
	static UINT (WINAPI *AulddaysGetDpiForWindow)(HWND hwnd);
	static BOOL (WINAPI *AulddaysSystemParametersInfoForDpi)(
		UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni, UINT dpi);
	static int (WINAPI *AulddaysGetSystemMetricsForDpi)(int nIndex, UINT dpi);

	// AulddaysDpiData internals
	static std::list<AulddaysDpiData> _dpidata;
	// _dpimap maps dpi value to pointers to AulddaysDpiData, which are stored in _dpidata.
	// _dpimap does not store concrete obj but pointers because gdi objs in AulddaysDpiData may not be copyable.
	static std::map<UINT, AulddaysDpiData *> _dpimap;
	static void initdata(UINT dpi);
};


/////////////////
// AulddaysToolBar
// To be used with EASY APIs
// The original CMFCToolBar does not scale well. Replace it with AulddaysToolBar 
/////////////////

class AulddaysToolBar : public CMFCToolBar
{
	DECLARE_DYNAMIC(AulddaysToolBar)

private:
	typedef CMFCToolBar super;

public:
	AulddaysToolBar() { };
	virtual ~AulddaysToolBar() { };

	void updateDpi();

protected:
	DECLARE_MESSAGE_MAP()

protected:
	virtual CSize CalcLayout(DWORD dwMode, int nLength = -1);
};

#ifndef WM_DPICHANGED
#	define WM_DPICHANGED 0x02E0
#endif