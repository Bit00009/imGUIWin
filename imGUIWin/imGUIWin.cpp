#include "pch.h"
#include "framework.h"
#include "imGUIWin.h"


// This is an example of an exported variable
IMGUIWIN_API int nimGUIWin = 0;

// This is an example of an exported function.
IMGUIWIN_API int fnimGUIWin(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CimGUIWin::CimGUIWin()
{
    return;
}
