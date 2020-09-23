// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the IMGUIWIN_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// IMGUIWIN_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef IMGUIWIN_EXPORTS
#define IMGUIWIN_API __declspec(dllexport)
#else
#define IMGUIWIN_API __declspec(dllimport)
#endif

// This class is exported from the dll
class IMGUIWIN_API CimGUIWin {
public:
	CimGUIWin(void);
	// TODO: add your methods here.
};

extern IMGUIWIN_API int nimGUIWin;

IMGUIWIN_API int fnimGUIWin(void);
