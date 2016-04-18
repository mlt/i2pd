/*
* Copyright (c) 2013-2016, The PurpleI2P Project
*
* This file is part of Purple i2pd project and licensed under BSD3
*
* See full license text in LICENSE file at top of project tree
*/
#include "wizard.h"
#include <windows.h>
#include "Win32App.h"
#include "resource.h"
#include <exception>
#include "../Config.h"
#include <commctrl.h>
#include <boost/asio.hpp>
#include <windowsx.h>
#include "DialogTemplateHelper.h"
#include <strsafe.h> // error message formatting

namespace i2p
{
	namespace win32
	{
		namespace config_editor
		{
		HINSTANCE g_hInst = GetModuleHandle(NULL);

		// could use vector - sequential numbering with known offset
		std::map<DWORD, std::string> g_ControlsMap; //!< dialog id to option mapping

		INT_PTR OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
		{
			return TRUE;
		}
		
		/// https://msdn.microsoft.com/en-us/library/hh298421%28v=vs.85%29.aspx
		void AdjustPos(SCROLLINFO& si, UINT code)
		{
			switch (code)
			{
			case SB_TOP:        si.nPos  =      si.nMin; break; // HOME key
			case SB_BOTTOM:     si.nPos  =      si.nMax; break; // END key
			case SB_LINEUP:     si.nPos -=           11; break; // UP key
			case SB_LINEDOWN:   si.nPos +=           11; break; // DOWN key
			case SB_PAGEUP:	    si.nPos -=     si.nPage; break; // User clicked the scroll bar shaft above the scroll box.
			case SB_PAGEDOWN:   si.nPos +=     si.nPage; break; // User clicked the scroll bar shaft below the scroll box.
			case SB_THUMBTRACK: si.nPos  = si.nTrackPos; break; // User dragged the scroll box.
			}
		}

		INT_PTR OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
		{
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &si);

			int yPos = si.nPos;
			AdjustPos(si, code);

			si.fMask = SIF_POS;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			GetScrollInfo(hwnd, SB_VERT, &si); // may not be the same

			if (si.nPos != yPos)
			{
				ScrollWindow(hwnd, 0, yPos - si.nPos, NULL, NULL);
				//UpdateWindow(hwnd);
			}
			return TRUE;
		}

		INT_PTR OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
		{
			return OnVScroll(hwnd, NULL, zDelta>0 ? SB_PAGEUP : SB_PAGEDOWN, 0);
			//return FALSE;
		}

		INT_PTR OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
		{
			switch (id)
			{
			case IDOK:
			case IDCANCEL:
				EndDialog(hwnd, 0);
				return id - 1;
			}
		}

		std::vector<std::function<void(HWND)> > g_DlgItemsInitCalls;
		std::vector<std::function<void(HWND)> > g_DlgItemsHintCalls;
		int content_height = 0;
		int content_width = 0;
		HFONT hfont = NULL;

		INT_PTR OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM)
		{
			for (auto& fun : g_DlgItemsInitCalls)
				fun(hwnd);
			for (auto& fun : g_DlgItemsHintCalls)
				fun(hwnd);
			RECT r = { 0, 0, 360, content_height };
			MapDialogRect(hwnd, &r);
			content_height = r.bottom;
			content_width = r.right;
			HWND w = GetDlgItem(hwnd, 2001);
			SendMessage(w, WM_SETFONT, (WPARAM)hfont, FALSE);
			return TRUE;
		}

		// https://msdn.microsoft.com/en-us/library/windows/desktop/hh298368%28v=vs.85%29.aspx
		// We don't care about this child window to be destroyed http://stackoverflow.com/q/4850794/673826
		void CreateToolTip(HWND hDlg, int toolID, PCTSTR pszText)
		{
			if (!toolID || !hDlg || !pszText)
				return;

			// Get the window of the tool.
			HWND hwndTool = GetDlgItem(hDlg, toolID);

			// Create the tooltip. g_hInst is the global instance handle.
			HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				hDlg, NULL,
				g_hInst, NULL);

			if (!hwndTool || !hwndTip)
				return;

			// Associate the tooltip with the tool.
			TOOLINFO toolInfo = { 0 };
			toolInfo.cbSize = sizeof(toolInfo);
			toolInfo.hwnd = hDlg;
			toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS | TTF_CENTERTIP;
			toolInfo.uId = (UINT_PTR)hwndTool;
			toolInfo.lpszText = const_cast<PTSTR>(pszText);
			SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
		}

		INT_PTR OnSizing(HWND hwnd, WPARAM edge, LPRECT rect)
		{
			RECT r = {0, 0, content_width, content_height};
			DWORD ws = GetWindowStyle(hwnd);
			AdjustWindowRect(&r, ws, FALSE);
			LONG maxy = r.bottom - r.top + rect->top;
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE | SIF_RANGE;
			GetScrollInfo(hwnd, SB_VERT, &si);
			LONG maxx = r.right - r.left + rect->left + GetSystemMetrics(SM_CXVSCROLL) * (si.nMax > si.nPage);
			if (rect->bottom > maxy)
				rect->bottom = maxy;
			//if (rect->right < maxx)
				rect->right = maxx;
			return TRUE;
		}

		INT_PTR OnSize(HWND hwnd, UINT state, int cx, int cy)
		{
			switch (state)
			{
			case SIZE_RESTORED:
			case SIZE_MAXIMIZED:
				break;
			default:
				return FALSE;
			}

			SCROLLINFO si;
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
			GetScrollInfo(hwnd, SB_VERT, &si);
			bool bottom = (si.nPos > 0) && (si.nPos + si.nPage >= si.nMax + 1);

			si.fMask = SIF_RANGE | SIF_PAGE; // SIF_POS | 
			si.nMin = 0;
			si.nPage = cy >> 2;
			si.nMax = std::max(0, content_height - cy + static_cast<int>(si.nPage) -1);
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			bool bottom2 = (si.nPos > 0) && (si.nPos + si.nPage >= si.nMax + 1);

			int delta = - static_cast<int>(si.nMax) + si.nPos + si.nPage - 1;
//			int delta = cy - content_height + si.nPos;
			if (bottom && delta > 0)// || state == SIZE_MAXIMIZED)
			{
				ScrollWindow(hwnd, 0, delta, NULL, NULL);
				//char buf[1024];
				//sprintf(buf, "delta = %d\n", delta);
				//::OutputDebugString(buf);
			} else if (bottom2 && state == SIZE_MAXIMIZED)
				// FIXME: why WM_SIZE is sent twice on max/restore???
				ScrollWindow(hwnd, 0, si.nPos >> 1, NULL, NULL);
			return TRUE;
		}

		INT_PTR OnCtlColorStatic(HWND hwnd, HDC hdc, HWND hwndChild, int type)
		{
			int id = GetDlgCtrlID(hwndChild);
			if (2001 == id) {
				SetTextColor(hdc, RGB(100, 100, 250));
				SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
				return (INT_PTR) GetSysColorBrush(COLOR_BTNFACE);
			}
			return FALSE;
		}

		INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{ // keep 'em sorted for jmp table optimization though no one cares
			HANDLE_MSG(hWnd, WM_SIZE, OnSize);
			HANDLE_MSG(hWnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hWnd, WM_HSCROLL, OnHScroll);
			HANDLE_MSG(hWnd, WM_VSCROLL, OnVScroll);
			HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, OnCtlColorStatic);
			HANDLE_MSG(hWnd, WM_MOUSEWHEEL, OnMouseWheel);
			case (WM_SIZING): return OnSizing(hWnd, wParam, (LPRECT)lParam);
			}
			return FALSE;
		}

		size_t add_controls(DialogTemplateHelper& h, short& y)
		{
			using namespace boost;
			using namespace boost::program_options;
			typedef shared_ptr<option_description> od;
			const std::vector< od >& options = i2p::config::m_OptionsDesc.options();
			size_t cnt = 0;
			LPDLGITEMTEMPLATEEX lpdit = NULL;
			DWORD id = 1001;
			g_ControlsMap.clear();
			g_DlgItemsInitCalls.clear();
			g_DlgItemsHintCalls.clear();

			std::set< std::string > blacklist;
			blacklist.insert("help");
			blacklist.insert("conf");
			blacklist.insert("svcctl");
			blacklist.insert("pidfile");
			blacklist.insert("daemon");
			blacklist.insert("service");

			for (od o : options)
			{
//				if (0 == o->long_name().compare("help"))
				if (blacklist.find(o->long_name()) != blacklist.end())
					continue;

				lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
				lpdit->helpID = 0;
				lpdit->x = 4; lpdit->cx = 76;
				lpdit->y = y; lpdit->cy = 9;
				lpdit->style = WS_CHILD | WS_VISIBLE | SS_RIGHT;
				lpdit->dwExtendedStyle = 0;
				lpdit->id = -1;
				*h.Allocate<WORD>() = 0xFFFF;
				*h.Allocate<WORD>() = 0x0082;        // static class
				h.Write(o->long_name().c_str());
				*h.Allocate<WORD>() = 0; // No creation data

				lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
				lpdit->helpID = 0;
				lpdit->x = 156; lpdit->cx = 200;
				lpdit->y = y; lpdit->cy = 9;
				lpdit->style = WS_CHILD | WS_VISIBLE | SS_ENDELLIPSIS;
				lpdit->dwExtendedStyle = 0;
				lpdit->id = -1;
				*h.Allocate<WORD>() = 0xFFFF;
				*h.Allocate<WORD>() = 0x0082;        // static class
				h.Write(o->description().c_str());
				*h.Allocate<WORD>() = 0; // No creation data

				// We heavily hope options won't change after tooltips are created. Otherwise pointers would be a mess.
				g_DlgItemsHintCalls.push_back( std::bind(CreateToolTip, std::placeholders::_1, id,  o->description().c_str()) );


				lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
				lpdit->helpID = 0;
				lpdit->x = 84; lpdit->cx = 68;
				lpdit->y = y; lpdit->cy = 10;
				lpdit->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
				lpdit->dwExtendedStyle = 0;
				lpdit->id = id;

				g_ControlsMap[id] = o->long_name();

				variables_map::iterator it = i2p::config::m_Options.find(o->long_name());
				if (it != i2p::config::m_Options.end())
				{
					any& v = it->second.value();
					const typeindex::type_info& t = v.type();
					if (t == boost::typeindex::type_id<std::string>())
					{
						std::string val = any_cast<std::string>(v);
						boost::system::error_code ec;
						boost::asio::ip::address a = boost::asio::ip::address::from_string(val, ec);
						if (ec)
						{
							lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
							*h.Allocate<WORD>() = 0xFFFF;
							*h.Allocate<WORD>() = 0x0081;        // edit class
							h.Write(val.c_str());
						}
						else
						{
							h.Write(WC_IPADDRESS);
							*h.Allocate<WORD>() = 0;
							g_DlgItemsInitCalls.push_back( std::bind(SendDlgItemMessage, std::placeholders::_1, id, IPM_SETADDRESS, 0, a.to_v4().to_ulong()) );
						}
					}
					else if (t == boost::typeindex::type_id<uint16_t>())
					{
						std::string val = std::to_string(any_cast<uint16_t>(v));
						lpdit->style |= ES_NUMBER;
						lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
						*h.Allocate<WORD>() = 0xFFFF;
						*h.Allocate<WORD>() = 0x0081;        // edit class
						h.Write(val.c_str());
					}
					else if (t == boost::typeindex::type_id<bool>())
					{
						bool val = any_cast<bool>(v);
						lpdit->style |= BS_AUTOCHECKBOX;
						*h.Allocate<WORD>() = 0xFFFF;
						*h.Allocate<WORD>() = 0x0080;        // button class
						*h.Allocate<WORD>() = 0;
						g_DlgItemsInitCalls.push_back( std::bind(SendDlgItemMessage, std::placeholders::_1, id, BM_SETCHECK, val ? BST_CHECKED : BST_UNCHECKED, 0) );
					}
					// bandwidth is a string now
					//else if (t == boost::typeindex::type_id<char>())
					//{
					//	std::string val("x"); val[0] = any_cast<char>(v);
					//	lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
					//	*h.Allocate<WORD>() = 0xFFFF;
					//	*h.Allocate<WORD>() = 0x0081;        // edit class
					//	h.Write(val.c_str());
					//}
					else
					{
						throw std::invalid_argument(std::string("not implemented for ").append(t.name()).c_str());
					}
				}
				*h.Allocate<WORD>() = 0; // No creation data
				y += 11;
				cnt++;
				id++;
			}
			return cnt;
		}

		void ErrorExit(LPTSTR lpszFunction)
		{
			// Retrieve the system error message for the last-error code

			LPVOID lpMsgBuf;
			LPVOID lpDisplayBuf;
			DWORD dw = GetLastError();

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				dw,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&lpMsgBuf,
				0, NULL);

			// Display the error message and exit the process

			lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
				(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
			StringCchPrintf((LPTSTR)lpDisplayBuf,
				LocalSize(lpDisplayBuf) / sizeof(TCHAR),
				TEXT("%s failed with error %d: %s"),
				lpszFunction, dw, lpMsgBuf);
			MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

			LocalFree(lpMsgBuf);
			LocalFree(lpDisplayBuf);
		}

		bool show()
		{
			INITCOMMONCONTROLSEX ic;
			ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
			ic.dwICC = ICC_STANDARD_CLASSES | ICC_INTERNET_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS;
			InitCommonControlsEx(&ic);
			DialogTemplateHelper h;
			//LPDLGTEMPLATE lpdt = h.Allocate<DLGTEMPLATE>();
			LPDLGTEMPLATEEX lpdt = h.Allocate<DLGTEMPLATEEX>();
			lpdt->dlgVer = 1;
			lpdt->signature = 0xFFFF;
			lpdt->helpID = 0;
			lpdt->style = WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | WS_THICKFRAME | DS_SETFONT | WS_MAXIMIZEBOX | WS_VSCROLL;// | DS_CENTER;
			//WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | WS_THICKFRAME;// | DS_SETFONT;
			lpdt->dwExtendedStyle = 0;
			lpdt->cdit = 1;         // Number of controls
			lpdt->x = 10;  lpdt->y = 10;
			lpdt->cx = 360 + 10; lpdt->cy = 300;
			*h.Allocate<WORD>() = 0; // no menu
			*h.Allocate<WORD>() = 0; // default class
			h.Write("i2pd settings");

			NONCLIENTMETRICS ncm;
			ncm.cbSize = sizeof(ncm);
			HDC hdc = GetDC(NULL);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
			// https://msdn.microsoft.com/en-us/library/windows/desktop/dd145037%28v=vs.85%29.aspx
			if (ncm.lfMessageFont.lfHeight < 0) {
				ncm.lfMessageFont.lfHeight = -MulDiv(ncm.lfMessageFont.lfHeight,
					72, GetDeviceCaps(hdc, LOGPIXELSY));
			}
			ReleaseDC(NULL, hdc);
			*h.Allocate<WORD>() = (WORD)ncm.lfMessageFont.lfHeight; // point
			*h.Allocate<WORD>() = (WORD)ncm.lfMessageFont.lfWeight; // weight
			*h.Allocate<BYTE>() = ncm.lfMessageFont.lfItalic; // Italic
			*h.Allocate<BYTE>() = ncm.lfMessageFont.lfCharSet; // CharSet
			h.Write(ncm.lfMessageFont.lfFaceName, LF_FACESIZE);

			short y = 4;
			ncm.lfMessageFont.lfHeight *= 2;//MulDiv(ncm.lfMessageFont.lfHeight, 3, 2);
			ncm.lfMessageFont.lfWeight *= 2;
			hfont = CreateFontIndirect(&ncm.lfMessageFont);
			LPDLGITEMTEMPLATEEX lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
			lpdit->helpID = 0;
			lpdit->x = 10; lpdit->y = y;
			lpdit->cx = 340; lpdit->cy = 11;
			lpdit->dwExtendedStyle = 0;
			lpdit->id = 2001;
			lpdit->style = WS_CHILD | WS_VISIBLE | SS_CENTER;
			*h.Allocate<WORD>() = 0xFFFF;
			*h.Allocate<WORD>() = 0x0082;        // static class
			h.Write("Let's change some defaults if you feel like before we begin!");
			*h.Allocate<WORD>() = 0; // No creation data

			y += 14;

			size_t cnt = add_controls(h, y);

			lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
			lpdit->helpID = 0;
			lpdit->x = 270; lpdit->y = y;
			lpdit->cx = 40; lpdit->cy = 14;
			lpdit->dwExtendedStyle = 0;
			lpdit->id = IDOK;
			lpdit->style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_DEFPUSHBUTTON;
			*h.Allocate<WORD>() = 0xFFFF;
			*h.Allocate<WORD>() = 0x0080;        // Button class
			h.Write("&OK");
			*h.Allocate<WORD>() = 0; // No creation data

			lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
			lpdit->helpID = 0;
			lpdit->x = 315; lpdit->y = y;
			lpdit->cx = 40; lpdit->cy = 14;
			lpdit->dwExtendedStyle = 0;
			lpdit->id = IDCANCEL;
			lpdit->style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP;
			*h.Allocate<WORD>() = 0xFFFF;
			*h.Allocate<WORD>() = 0x0080;        // Button class
			h.Write("&Cancel");
			*h.Allocate<WORD>() = 0; // No creation data

			content_height = y + 14 + 4;

			INT_PTR result = DialogBoxIndirect(g_hInst, h.GetBuffer(), GetDesktopWindow(), DlgProc);
			::DeleteObject(hfont);
			if (-1 == result)
				ErrorExit(TEXT("DialogBoxIndirect"));
			return false;
		}
	}
	}
}
