#ifndef WIZARD_H__
#define WIZARD_H__

#include <windows.h>

namespace i2p
{
	namespace win32
	{
		class SetupWizard
		{
		public:
			SetupWizard();
			bool Show();
			virtual ~SetupWizard();
		private:
			HWND hwndMain;
			HWND hwndTab;
			static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			static HWND CreateTabs(HWND hwndParent);
			static int CALLBACK EditWordBreakProc(
				_In_ LPTSTR lpch,
				_In_ int    ichCurrent,
				_In_ int    cch,
				_In_ int    code
				);
		};
	}
}

#endif
