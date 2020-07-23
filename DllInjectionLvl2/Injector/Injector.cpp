// Injector.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <tchar.h>
#include "Injector.h"

using namespace std;

void ExitWithError(const char* errorMessage, int exitCode = -1) {
	printf("%s\n\tError number: %d", errorMessage, GetLastError() || exitCode);
	ExitProcess(exitCode);
}

int _tmain(int argc, TCHAR* argv[])
{
	if (argc < 3) {
		ExitWithError("Usage: Injector.exe <ProcessName> <DLLPath>");
	}

	TCHAR* processName = argv[1];
	TCHAR* dllPath = argv[2];

	cout << "Taking system snapshot" << endl;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		ExitWithError("Could not get system snapshot");
	}

	HANDLE hVictimProcess = NULL;
	PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };

	_tprintf(TEXT("Looking for %s"), processName);
	BOOL didGotProcessFromSnapshot = Process32First(hSnapshot, &processEntry);
	if (didGotProcessFromSnapshot == false) {
		ExitWithError("Could not find any process inside the system snapshot");
	}

	while (wcscmp(processEntry.szExeFile, processName) != 0) {
		Process32Next(hSnapshot, &processEntry);
	}

	int pid = processEntry.th32ProcessID;

	// Allocate memory for the Thread object we gonna find
	tagTHREADENTRY32* ptrThreadEntry = (tagTHREADENTRY32*)calloc(1, sizeof(tagTHREADENTRY32));
	ptrThreadEntry->dwSize = sizeof(tagTHREADENTRY32);

	// Saving the process threads - up to 30
	int tidList[30];
	SecureZeroMemory(tidList, sizeof(int) * 30);

	cout << "Looking for threads of the process" << endl;
	if (Thread32First(hSnapshot, ptrThreadEntry) != TRUE) {
		ExitWithError("Could get the first thread from the snapshot");
	}

	int i = 0;
	while (i < 30) {
		if (ptrThreadEntry->th32OwnerProcessID == pid) {
			tidList[i] = ptrThreadEntry->th32ThreadID;
			i++;
		}

		if (Thread32Next(hSnapshot, ptrThreadEntry) != TRUE) {
			_tprintf(TEXT("Thread lookup on Process %d finished"), pid);
			break;
		}
	}

	if (!tidList[0]) {
		ExitWithError("Could not find any threads for the process");
	}

	cout << "Getting handles..." << endl;
	HANDLE hOpenThread;
	HANDLE hOpenProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, pid);
	if (hOpenProcess == NULL) {
		ExitWithError("Could not get a handle to the process");
	}

	int injectedDllPathLen = _tcslen(dllPath) * sizeof(TCHAR) + sizeof(TCHAR);
	
	LPVOID ptrDll = VirtualAllocEx(hOpenProcess, NULL, injectedDllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (ptrDll == NULL) {
		ExitWithError("Could not allocate memory for the dll");
	}

	BOOL writeResult = WriteProcessMemory(
		hOpenProcess,
		ptrDll,
		dllPath,
		injectedDllPathLen,
		NULL
	);
	if (writeResult == FALSE) {
		ExitWithError("Could not write dll data at target process memory");
	}

	HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (hKernel32 == NULL) {
		ExitWithError("Could not get handle for kernel32.dll");
	}

	LPVOID ptrLoadLibrary = (LPVOID)GetProcAddress(hKernel32, LOAD_LIBRARY_VERSION);
	if (ptrLoadLibrary == NULL) {
		ExitWithError("Could not find LoadLibrary at kernel32.dll");
	}

	cout << "Registering ACPs to process threads" << endl;
	i = 0;
	while (i < 30 && tidList[i] != 0) {
		hOpenThread = OpenThread(THREAD_ALL_ACCESS, NULL, tidList[i]);
		if (hOpenThread == NULL) {
			ExitWithError("Could not open handle for process thead");
		}
		else if (QueueUserAPC((PAPCFUNC)ptrLoadLibrary, hOpenThread, (ULONG_PTR)ptrDll)) {
			_tprintf(TEXT("Inserted APC to thread id %d\n"), tidList[i]);
		}
		else {
			_tprintf(TEXT("Could not add APC to the queue of thread %d\n"), tidList[i]);
		}
		i++;
	}

	cout << "Done!" << endl;
}
