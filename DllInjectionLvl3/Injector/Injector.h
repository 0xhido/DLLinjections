#ifdef UNICODE
#define LOAD_LIBRARY_VERSION "LoadLibraryW"
#else
#define LOAD_LIBRARY_VERSION "LoadLibraryA"
#endif // UNICODE

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <tchar.h>

HANDLE GetSystemSnapshot();

int GetProcessID(HANDLE hSnapshot, TCHAR* processName);

int GetProcessThreadID(HANDLE hSnapshot, int ProcessID);

LPVOID WriteDllInProcess(HANDLE hProcess, TCHAR* dllPath);

int _tmain(int argc, TCHAR* argv[]);

