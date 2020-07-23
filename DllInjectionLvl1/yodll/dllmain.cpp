// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    MessageBox(NULL, TEXT("YoU hAvE bEeN PwNed"), TEXT("PwNedEd!"), MB_OK | MB_ICONWARNING);

    return TRUE;
}

