#include <Windows.h>
#include <iostream>

#include "imGUIWinLib.h"
#pragma comment(lib, "..\\build\\imGUIWinLib.lib")

using namespace std;

int main()
{
	cout << "imGUIWin.Demo" << endl;

	InitializeGUIEngine();
	cout << "GUI Engine has been initialized." << endl;

	ViewHandle view01 = CreateNewViewWindow(L"imGUI View 01");
    cout << "view01 has been created, Handle : " << hex << view01 << endl;

    ViewHandle view02 = CreateNewViewWindow(L"imGUI View 02");
    cout << "view02 has been created, Handle : " << hex << view02 << endl;

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT)
    {
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        RenderViewWindow(view01);
        RenderViewWindow(view02);
    }
}