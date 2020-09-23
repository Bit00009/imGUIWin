#pragma once

#define imGUIWinAPI extern "C" __declspec(dllexport)

// namespace imGUIWin
// {
	typedef void(*OnRenderEvent)(void);
	typedef void* ViewHandle;

	imGUIWinAPI bool         InitializeGUIEngine();
	imGUIWinAPI ViewHandle   CreateNewViewWindow(const wchar_t* vName);
	imGUIWinAPI bool         SetViewWindowOnRenderEvent(ViewHandle vHwnd, OnRenderEvent vEvent);
	imGUIWinAPI bool         DisposeViewWindow(ViewHandle vHwnd);
	imGUIWinAPI bool         RenderViewWindow(ViewHandle vHwnd);
// }