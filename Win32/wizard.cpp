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

		LRESULT OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
		{
			return TRUE;
		}
		
		int yPos = 0;
		SCROLLINFO si = {};

		/// https://msdn.microsoft.com/en-us/library/hh298421%28v=vs.85%29.aspx
		void AdjustPos(SCROLLINFO& si, UINT code)
		{
			switch (code)
			{

				// User clicked the HOME keyboard key.
			case SB_TOP:
				si.nPos = si.nMin;
				break;

				// User clicked the END keyboard key.
			case SB_BOTTOM:
				si.nPos = si.nMax;
				break;

				// User clicked the top arrow.
			case SB_LINEUP:
				si.nPos -= 1;
				break;

				// User clicked the bottom arrow.
			case SB_LINEDOWN:
				si.nPos += 1;
				break;

				// User clicked the scroll bar shaft above the scroll box.
			case SB_PAGEUP:
				si.nPos -= si.nPage;
				break;

				// User clicked the scroll bar shaft below the scroll box.
			case SB_PAGEDOWN:
				si.nPos += si.nPage;
				break;

				// User dragged the scroll box.
			case SB_THUMBTRACK:
				si.nPos = si.nTrackPos;
				break;

			default:
				break;
			}
		}

		LRESULT OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
		{
			// Get all the vertial scroll bar information.
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(hwnd, SB_VERT, &si);

			// Save the position for comparison later on.
			yPos = si.nPos;
			AdjustPos(si, code);

			// Set the position and then retrieve it.  Due to adjustments
			// by Windows it may not be the same as the value set.
			si.fMask = SIF_POS;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
			GetScrollInfo(hwnd, SB_VERT, &si);

			// If the position has changed, scroll window and update it.
			if (si.nPos != yPos)
			{
				ScrollWindow(hwnd, 0, 10 * (yPos - si.nPos), NULL, NULL);
				UpdateWindow(hwnd);
			}
			return TRUE;
		}

		LRESULT OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
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

		LRESULT OnInitDialog(HWND hWnd, HWND hwndFocus, LPARAM)
		{
			for (auto& fun : g_DlgItemsInitCalls)
				fun(hWnd);
			return TRUE;
		}

		INT_PTR OnSize(HWND hwnd, UINT state, int cx, int cy)
		{
			SCROLLINFO si;
			// Set the vertical scrolling range and page size
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE;// | SIF_PAGE;
			si.nMin = 0;
			si.nMax = 500;//LINES - 1;
			si.nPage = cy;// / 10;
			SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

			// Set the horizontal scrolling range and page size. 
			si.cbSize = sizeof(si);
			si.fMask = SIF_RANGE;// | SIF_PAGE;
			si.nMin = 0;
			si.nMax = 200;//2 + xClientMax / xChar;
			si.nPage = cx / 10;
			SetScrollInfo(hwnd, SB_HORZ, &si, TRUE); 
			return TRUE;
		}

		INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			LONG scrollpos = 0;
			switch (uMsg)
			{
			//case WM_VSCROLL:
			//{
			//	//SCROLLINFO si;
			//	//GetScrollInfo(hWnd, SB_VERT, &si);
			//	WORD cp = HIWORD(wParam);
			//	ScrollWindow(hWnd, 0, scrollpos - cp, NULL, NULL);
			//	SetScrollPos(hWnd, SB_VERT, cp, FALSE);
			//	scrollpos = cp;
			//	//UpdateWindow(hWnd);
			//	break;
			//}
			//case WM_SIZE:
			//	// Make the edit control the size of the window's client area. 

			//	MoveWindow(hwndEdit,
			//		0, 0,                  // starting x- and y-coordinates 
			//		LOWORD(lParam),        // width of client area 
			//		HIWORD(lParam),        // height of client area 
			//		TRUE);                 // repaint window 
			//	return 0; 
			//case WM_DESTROY:
			//{
			//	PostQuitMessage(0);
			//	break;
			//}
//			HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
			HANDLE_MSG(hWnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hWnd, WM_HSCROLL, OnHScroll);
			HANDLE_MSG(hWnd, WM_VSCROLL, OnVScroll);
			HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hWnd, WM_SIZE, OnSize);
			}
			return FALSE;
		}

		size_t add_controls(DialogTemplateHelper& h)
		{
			using namespace boost;
			using namespace boost::program_options;
			typedef shared_ptr<option_description> od;
			const std::vector< od >& options = i2p::config::m_OptionsDesc.options();
			size_t cnt = 0;
			LPDLGITEMTEMPLATEEX lpdit = NULL;
			short y = 4;
			DWORD id = 100;
			g_ControlsMap.clear();
			g_DlgItemsInitCalls.clear();

			for (od o : options)
			{
				if (0 == o->long_name().compare("help"))
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
				lpdit->style = WS_CHILD | WS_VISIBLE;
				lpdit->dwExtendedStyle = 0;
				lpdit->id = -1;
				*h.Allocate<WORD>() = 0xFFFF;
				*h.Allocate<WORD>() = 0x0082;        // static class
				h.Write(o->description().c_str());
				*h.Allocate<WORD>() = 0; // No creation data


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
					else if (t == boost::typeindex::type_id<char>())
					{
						std::string val("x"); val[0] = any_cast<char>(v);
						lpdit->dwExtendedStyle = WS_EX_CLIENTEDGE;
						*h.Allocate<WORD>() = 0xFFFF;
						*h.Allocate<WORD>() = 0x0081;        // edit class
						h.Write(val.c_str());
					}
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
			lpdt->cx = 500; lpdt->cy = 300;
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

			size_t cnt = add_controls(h);

			LPDLGITEMTEMPLATEEX lpdit = h.Allocate<DLGITEMTEMPLATEEX>();
			lpdit->helpID = 0;
			lpdit->x = 10; lpdit->y = ((short)cnt+1) * 11;
			lpdit->cx = 20; lpdit->cy = 14;
			lpdit->dwExtendedStyle = 0;
			lpdit->id = IDOK;       // OK button identifier
			lpdit->style = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_DEFPUSHBUTTON;

			*h.Allocate<WORD>() = 0xFFFF;
			*h.Allocate<WORD>() = 0x0080;        // Button class
			h.Write("&Ok");
			*h.Allocate<WORD>() = 0; // No creation data

			INT_PTR result = DialogBoxIndirect(g_hInst, h.GetBuffer(), GetDesktopWindow(), DlgProc);
			if (-1 == result)
				ErrorExit(TEXT("DialogBoxIndirect"));
			return false;
		}
	}
	}
}
