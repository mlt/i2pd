#include "wizard.h"
#include "Win32App.h"
#include "resource.h"
#include <exception>
#include "../Config.h"
#include <CommCtrl.h>

namespace i2p
{
	namespace win32
	{
#define I2PD_WIZARD_CLASSNAME "i2pd wizard"

		HINSTANCE g_hInst = GetModuleHandle(NULL);

		SetupWizard::SetupWizard()
		{
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
			wclx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
			wclx.lpszMenuName = NULL;
			wclx.lpszClassName = I2PD_WIZARD_CLASSNAME;
			RegisterClassEx(&wclx);
			// create new window
			hwndMain = CreateWindow(I2PD_WIZARD_CLASSNAME, TEXT("i2pd"), WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL,
				100, 100, 450, 350, NULL, NULL, hInst, NULL);
			if (!hwndMain)
			{
				MessageBox(NULL, "Failed to create main window", TEXT("Warning!"), MB_ICONERROR | MB_OK | MB_TOPMOST);
				throw std::exception("TODO");
			}
		}

		bool SetupWizard::Show()
		{
			RunWin32App();
			return false;
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

		LRESULT CALLBACK SetupWizard::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			static HWND hwndEdit;
#define ID_EDITCHILD 100
			switch (uMsg)
			{
			case WM_CREATE:
			{
//				hwndEdit = CreateTabs(hWnd);
				//g_EditWindows
				int y = 10;
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
				for (od o : options)
				{
					HWND hwndLabel = CreateWindowEx(
						0, TEXT("EDIT"),   // predefined class 
						NULL,         // no window title 
						WS_CHILD | WS_VISIBLE | ES_LEFT,
						10, y, 150, 16,   // don't set size in WM_SIZE message 
						hWnd,         // parent window 
						NULL,//(HMENU)ID_EDITCHILD,   // edit control ID 
						g_hInst,
						NULL);        // pointer not needed 
					hwndEdit = CreateWindowEx(
						0, TEXT("EDIT"),   // predefined class 
						NULL,         // no window title 
						WS_CHILD | WS_VISIBLE | ES_LEFT,
						170, y, 150, 16,   // don't set size in WM_SIZE message 
						hWnd,         // parent window 
						NULL,//(HMENU)ID_EDITCHILD,   // edit control ID 
						g_hInst,
						NULL);        // pointer not needed 
					HWND hwndDesc = CreateWindowEx(
						0, TEXT("EDIT"),   // predefined class 
						NULL,         // no window title 
						WS_CHILD | WS_VISIBLE | ES_LEFT,
						330, y, 150, 16,   // don't set size in WM_SIZE message 
						hWnd,         // parent window 
						NULL,//(HMENU)ID_EDITCHILD,   // edit control ID 
						g_hInst,
						NULL);        // pointer not needed 
					if (0 == o->long_name().compare("help"))
						continue;
					SendMessage(hwndLabel, WM_SETTEXT, 0, (LPARAM)o->long_name().c_str());
					variables_map::iterator it = i2p::config::m_Options.find(o->long_name());
					if (it != i2p::config::m_Options.end())
					{
						any& v = it->second.value();
						std::string val;
						try
						{
							val = any_cast<std::string>(v);
						}
						catch (bad_any_cast& e) {
							try
							{
								val = any_cast<bool>(v) ? "yes" : "no";
							}
							catch (bad_any_cast& e) {
							}
						}
						SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)val.c_str());
					}
					SendMessage(hwndDesc, WM_SETTEXT, 0, (LPARAM)o->description().c_str());
					y += 22;
					//if (o->semantic()->max_tokens() < 1)
					//	continue;
					//ss << o->long_name();
					//ss << " - ";
					//ss << o->description();
					//ss << "\r\n";
				}

				//SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)ss.str().c_str());
				// Add text to the window. 
				//SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)lpszLatin); 
				return 0;
			}
			//case WM_SIZE:
			//	// Make the edit control the size of the window's client area. 

			//	MoveWindow(hwndEdit,
			//		0, 0,                  // starting x- and y-coordinates 
			//		LOWORD(lParam),        // width of client area 
			//		HIWORD(lParam),        // height of client area 
			//		TRUE);                 // repaint window 
			//	return 0; 
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				break;
			}
			//case WM_COMMAND:
			//{
			//	switch (LOWORD(wParam))
			//	{
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
			//	}
			//	break;
			//}
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		SetupWizard::~SetupWizard()
		{
			UnregisterClass(I2PD_WIZARD_CLASSNAME, GetModuleHandle(NULL));
		}
	}
}
