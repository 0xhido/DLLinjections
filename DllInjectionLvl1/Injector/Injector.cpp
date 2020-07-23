// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <tchar.h>
#include <processthreadsapi.h>
#include "Injector.h"

int _tmain(int argc, TCHAR* argv[]) {
	if (argc < 2) {
		_tprintf(TEXT("Usage: injector <ProcessID>\r\n"));
		return 1;
	}

	int pid = _tstoi(argv[1]);

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);
	if (hProcess == NULL) {
		_tprintf(TEXT("Cannot open process, probably the wrong PID\r\n"));
		return -1;
	}

	int injectedDllSize = _tcslen(TEXT("C:\\Users\\13768J\\Desktop\\Code\\DllInjections\\DllInjectionLvl1\\yodll\\x64\\Debug\\yodll.dll")) * sizeof(TCHAR) + sizeof(TCHAR);

	LPVOID ptrDll = VirtualAllocEx(hProcess, NULL, injectedDllSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (ptrDll == NULL) {
		_tprintf(TEXT("Could not allocate memory for injection\r\n"));
		return -1;
	}

	BOOL writeResult = WriteProcessMemory(
		hProcess,
		ptrDll,
		TEXT("C:\\Users\\13768J\\Desktop\\Code\\DllInjections\\DllInjectionLvl1\\yodll\\x64\\Debug\\yodll.dll"),
		injectedDllSize,
		NULL);
	if (writeResult == FALSE) {
		_tprintf(TEXT("Could not write dll to process memory\r\n"));
		return -1;
	}

	HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (hKernel32 == NULL) {
		_tprintf(TEXT("Could not get handle for kernel32.dll\r\n"));
		return -1;
	}

	LPVOID ptrLoadLibrary = (LPVOID)GetProcAddress(hKernel32, LOAD_LIBRARY_VERSION);
	if (ptrLoadLibrary == NULL) {
		_tprintf(TEXT("Could not find LoadLibrary at kernel32.dll\r\n"));
		return -1;
	}

	DWORD tid;

	HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)ptrLoadLibrary, ptrDll, 0, &tid);
	if (hRemoteThread == NULL) {
		std::cout << GetLastError();
		_tprintf(TEXT("Could not create remote thread\r\n"));
		return -1;
	}

	_tprintf(TEXT("Remote thread has been created. ThreadID = %d\r\n"), tid);
}