#include "wizard.h"
#include "Win32App.h"
#include "resource.h"
#include <exception>
#include "../Config.h"
#include <CommCtrl.h>
#include <boost/asio.hpp>
#include <windowsx.h>

namespace i2p
{
	namespace win32
	{
#define I2PD_WIZARD_CLASSNAME "i2pd wizard"

		HINSTANCE g_hInst = GetModuleHandle(NULL);
		//HWND hwndMain = NULL;

		SetupWizard::SetupWizard()
		{
			INITCOMMONCONTROLSEX ic;
			ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
			ic.dwICC = ICC_STANDARD_CLASSES | ICC_INTERNET_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS;
			InitCommonControlsEx(&ic);
			// register main window
			auto hInst = GetModuleHandle(NULL);
			WNDCLASSEX wclx;
			memset(&wclx, 0, sizeof(wclx));
			wclx.cbSize = sizeof(wclx);
			wclx.style = 0;
			wclx.lpfnWndProc = WndProc;
			wclx.cbClsExtra = 0;
			wclx.cbWndExtra = 0;
			wclx.hInstance = hInst;
			wclx.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(MAINICON));
			wclx.hCursor = LoadCursor(NULL, IDC_ARROW);
			wclx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
			wclx.lpszMenuName = NULL;
			wclx.lpszClassName = I2PD_WIZARD_CLASSNAME;
			RegisterClassEx(&wclx);
			// create new window
			//hwndMain = CreateWindowEx(0, I2PD_WIZARD_CLASSNAME, TEXT("i2pd configuration"),
			//	WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL | WS_EX_CONTROLPARENT,
			//	100, 100, 450, 350, NULL, NULL, hInst, NULL);
			//i2p::win32::hwndMain = hwndMain;
			hwndMain = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, WndProc);
			if (!hwndMain)
			{
				MessageBox(NULL, "Failed to create main window", TEXT("Warning!"), MB_ICONERROR | MB_OK | MB_TOPMOST);
				throw std::exception("TODO");
			}
		}

		bool SetupWizard::Show()
		{
			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0))
			{
				if (!IsDialogMessage(hwndMain, &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			return msg.wParam;
		}

		int CALLBACK SetupWizard::EditWordBreakProc(
			_In_ LPTSTR lpch,
			_In_ int    ichCurrent,
			_In_ int    cch,
			_In_ int    code
			)
		{
			switch (code) {
			case WB_ISDELIMITER:
				return lpch[ichCurrent] == '\n';
			}
			return 0;
		}

		HWND SetupWizard::CreateTabs(HWND hwndParent)
		{
			HWND hwndTab = CreateWindow(WC_TABCONTROL, "",
				WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
				0, 0, 0, 0,
				hwndParent, NULL, g_hInst, NULL);
			TCITEM tci;
			memset(&tci, 0, sizeof(TCITEM));
			tci.mask = TCIF_TEXT;
			tci.pszText = "Main";
			TabCtrl_InsertItem(hwndTab, 0, &tci);
			return hwndTab;
		}

		std::map<HWND, std::string> g_EditWindows;

		LRESULT OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
			switch (id)
			{
			case IDOK:
			case IDCANCEL:
				DestroyWindow(hwnd);
				return id - 1;
				//	case ID_ABOUT:
				//	{
				//		MessageBox(hWnd, TEXT("i2pd"), TEXT("About"), MB_ICONINFORMATION | MB_OK);
				//		return 0;
				//	}
				//	case ID_EXIT:
				//	{
				//		PostMessage(hWnd, WM_CLOSE, 0, 0);
				//		return 0;
				//	}
				//	case ID_CONSOLE:
				//	{
				//		char buf[30];
				//		std::string httpAddr; i2p::config::GetOption("http.address", httpAddr);
				//		uint16_t    httpPort; i2p::config::GetOption("http.port", httpPort);
				//		snprintf(buf, 30, "http://%s:%d", httpAddr.c_str(), httpPort);
				//		ShellExecute(NULL, "open", buf, NULL, NULL, SW_SHOWNORMAL);
				//		return 0;
				//	}
				//	case ID_APP:
				//	{
				//		ShowWindow(hWnd, SW_SHOW);
				//		return 0;
				//	}
			}
		}

		LRESULT OnDestroy(HWND hwnd)
		{
			PostQuitMessage(0);
			return TRUE;
		}

		LRESULT CALLBACK SetupWizard::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			HWND hwndEdit = NULL;
			HFONT hFont = NULL;
			LONG scrollpos = 0;
#define ID_EDITCHILD 100
			switch (uMsg)
			{
			case WM_INITDIALOG:
			case WM_CREATE:
			{
//				hwndEdit = CreateTabs(hWnd);
				//g_EditWindows
				//HDC hDC;
				//hDC = GetDC(hwndEdit);
				//HFONT hFont;
				//hFont = (HFONT)SendMessage(hwndEdit, WM_GETFONT, 0, 0L);
				//// If the font used is not the system font, select it.
				//if (hFont != NULL)
				//	SelectObject(hDC, hFont);
				//TEXTMETRIC tm;
				//GetTextMetrics(hDC, &tm);
				//ReleaseDC(hwndEdit, hDC);
				//SendMessage(hwndEdit, EM_SETWORDBREAKPROC, 0, (LPARAM)EditWordBreakProc);
				using namespace boost;
				using namespace boost::program_options;
				typedef shared_ptr<option_description> od;
				const std::vector< od >& options = i2p::config::m_OptionsDesc.options();
//				std::stringstream ss;
				LONG dlu = ::GetDialogBaseUnits();
				LONG dlux = LOWORD(dlu);
				LONG dluy = HIWORD(dlu);
				int y = dlux;
				NONCLIENTMETRICS ncm;
				ncm.cbSize = sizeof(ncm);
				SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
				hFont = CreateFontIndirect(&(ncm.lfMessageFont));
				//hFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, \
				//	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, \
				//	DEFAULT_PITCH | FF_SWISS, "Microsoft Sans Serif");//"Tahoma");
				for (od o : options)
				{
					HWND hwndLabel = CreateWindowEx(0, WC_STATIC, NULL,	WS_CHILD | WS_VISIBLE | SS_RIGHT,
						dlux, y, 19*dlux, MulDiv(2, dluy, 2), hWnd, NULL, g_hInst, NULL);
					HWND hwndDesc = CreateWindowEx( 0, WC_STATIC, NULL, WS_CHILD | WS_VISIBLE,
						41*dlux, y, 60*dlux, MulDiv(2, dluy, 2), hWnd, NULL, g_hInst, NULL);
					if (0 == o->long_name().compare("help"))
						continue;
					SendMessage(hwndLabel, WM_SETTEXT, 0, (LPARAM)o->long_name().c_str());
					SendMessage(hwndLabel, WM_SETFONT, WPARAM(hFont), TRUE);
					variables_map::iterator it = i2p::config::m_Options.find(o->long_name());
					if (it != i2p::config::m_Options.end())
					{
						any& v = it->second.value();
						const typeindex::type_info& t = v.type();
						size_t hash_code = t.hash_code();
//						if (hash_code == boost::typeindex::type_id<std::string>().hash_code())
						if (t == boost::typeindex::type_id<std::string>())
						{
							std::string val = any_cast<std::string>(v);
							boost::system::error_code ec;
							boost::asio::ip::address a = boost::asio::ip::address::from_string(val, ec);
							if (ec)
							{
								hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | WS_TABSTOP,
									170, y, 150, 20, hWnd, NULL, g_hInst, NULL);
								SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)val.c_str());
							}
							else
							{
								hwndEdit = CreateWindowEx(0, WC_IPADDRESS, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP,
									170, y, 150, 20, hWnd, NULL, g_hInst, NULL);
								SendMessage(hwndEdit, IPM_SETADDRESS, 0, (LPARAM)a.to_v4().to_ulong());
							}
						}
						else if (hash_code == boost::typeindex::type_id<uint16_t>().hash_code())
						{
							std::string val = std::to_string(any_cast<uint16_t>(v));
							hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_NUMBER | WS_TABSTOP,
								170, y, 150, 20, hWnd, NULL, g_hInst, NULL);
							SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)val.c_str());
						}
						else if (hash_code == boost::typeindex::type_id<bool>().hash_code())
						{
							bool val = any_cast<bool>(v);
							hwndEdit = CreateWindowEx(0, WC_BUTTON, NULL, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
								170, y, 150, 20, hWnd, NULL, g_hInst, NULL);
							SendMessage(hwndEdit, BM_SETCHECK, val ? BST_CHECKED : BST_UNCHECKED, 0);
						}
						else if (hash_code == boost::typeindex::type_id<char>().hash_code())
						{
							std::string val("x"); val[0] = any_cast<char>(v);
							hwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT | WS_TABSTOP,
								170, y, 150, 20, hWnd, NULL, g_hInst, NULL);
							SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)val.c_str());
						}
						else
						{
							throw std::exception(std::string("not implemented for ").append(t.name()).c_str());
						}
					}
					SendMessage(hwndDesc, WM_SETTEXT, 0, (LPARAM)o->description().c_str());
					SendMessage(hwndDesc, WM_SETFONT, WPARAM(hFont), TRUE);
					if (NULL != hwndEdit)
						SendMessage(hwndEdit, WM_SETFONT, WPARAM(hFont), TRUE);
					y += MulDiv(dluy, 6, 4);
					//if (o->semantic()->max_tokens() < 1)
					//	continue;
					//ss << o->long_name();
					//ss << " - ";
					//ss << o->description();
					//ss << "\r\n";
				}
				//SetScrollRange(hWnd, SB_CTL| SB_VERT, 0, y, TRUE);
				RECT r;
				GetWindowRect(hWnd, &r);
				//Setwindo
//				SetWindowPos(hWnd, NULL, r.left, r.top, 42*dlux, y, 0);
				//SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)ss.str().c_str());
				// Add text to the window. 
				//SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)lpszLatin); 
				return TRUE;
			}
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
			HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
			HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		SetupWizard::~SetupWizard()
		{
			UnregisterClass(I2PD_WIZARD_CLASSNAME, GetModuleHandle(NULL));
		}
	}
}
