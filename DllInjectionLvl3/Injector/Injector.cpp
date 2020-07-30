#include "Injector.h"

using namespace std;

void ExitWithError(const char* errorMessage, int exitCode = -1) {
	printf("%s\n\tError number: %d", errorMessage, GetLastError() || exitCode);
	ExitProcess(exitCode);
}

HANDLE GetSystemSnapshot() {
	cout << "Taking system snapshot" << endl;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD, NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		ExitWithError("Could not get system snapshot");
	}
	return hSnapshot;
}

int GetProcessID(HANDLE hSnapshot, TCHAR* processName) {
	PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };

	_tprintf(TEXT("Looking for %s\n"), processName);
	BOOL didGotProcessFromSnapshot = Process32First(hSnapshot, &processEntry);
	if (didGotProcessFromSnapshot == false) {
		ExitWithError("Could not find any process inside the system snapshot");
	}

	while (wcscmp(processEntry.szExeFile, processName) != 0) {
		Process32Next(hSnapshot, &processEntry);
	}

	return processEntry.th32ProcessID;
}

int GetProcessThreadID(HANDLE hSnapshot, int ProcessID) {
	// Allocate memory for the Thread object we gonna find
	tagTHREADENTRY32* ptrThread = (tagTHREADENTRY32*)calloc(1, sizeof(tagTHREADENTRY32));
	ptrThread->dwSize = sizeof(tagTHREADENTRY32);
	int tid;

	cout << "Looking for threads of the process" << endl;
	if (Thread32First(hSnapshot, ptrThread) != TRUE) {
		ExitWithError("Could get the first thread from the snapshot");
	}

	while (Thread32Next(hSnapshot, ptrThread)) {
		if (ptrThread->th32OwnerProcessID == ProcessID) {
			return ptrThread->th32ThreadID;
		}
	}

	free(ptrThread);
	ExitWithError("Could not find any thread inside the process");
}

LPVOID WriteDllInProcess(HANDLE hProcess, TCHAR* dllPath) {
	cout << "Write DLL into process's memory" << endl;
	int injectedDllPathLen = _tcslen(dllPath) * sizeof(TCHAR) + sizeof(TCHAR);

	LPVOID ptrDll = VirtualAllocEx(hProcess, NULL, injectedDllPathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (ptrDll == NULL) {
		ExitWithError("Could not allocate memory for the dll");
	}

	BOOL result = WriteProcessMemory(
		hProcess,
		ptrDll,
		dllPath,
		injectedDllPathLen,
		NULL
	);
	if (result == FALSE) {
		ExitWithError("Could not write dll data at target process memory");
	}

	return ptrDll;
}

void InjectToStack(HANDLE hProcess, HANDLE hThread, LPCONTEXT lpContext, LPVOID ptrDll, LPVOID ptrLoadLibrary) {
	// Insert LoadLibrary parameters
	LPVOID ptrDllAddress = (LPVOID)((LPBYTE)lpContext->Esp - (LPBYTE)(sizeof(DWORD)));
	BOOL result = WriteProcessMemory(hProcess, ptrDllAddress, &ptrDll, sizeof(DWORD), NULL);
	if (result == FALSE) {
		ExitWithError("Could not write LoadLibrary address to thread stack");
	}

	// Insert return address
	LPVOID ptrOldEipAddress = (LPVOID)((LPBYTE)lpContext->Esp - (LPBYTE)(2 * sizeof(DWORD)));
	result = WriteProcessMemory(hProcess, ptrOldEipAddress, (LPVOID)&(lpContext->Eip), sizeof(DWORD), NULL);
	if (result == FALSE) {
		ExitWithError("Could not write old EIP address to thread stack");
	}

	// Set next instraction to be LoadLibrary
	lpContext->Eip = (DWORD)ptrLoadLibrary;
	// Update ESP to the head of the stack
	lpContext->Esp = (DWORD)ptrOldEipAddress;

	result = SetThreadContext(hThread, lpContext);
	if (result == FALSE) {
		ExitWithError("Could not set thread context");
	}
}


int _tmain(int argc, TCHAR* argv[])
{
	if (argc < 3) {
		ExitWithError("Usage: Injector.exe <ProcessName> <DLLPath>");
	}

	TCHAR* processName = argv[1];
	TCHAR* dllPath = argv[2];
	
	// Get Process and Thread IDs
	HANDLE hSnapshot = GetSystemSnapshot();
	int ProcessID = GetProcessID(hSnapshot, processName);
	int ThreadID = GetProcessThreadID(hSnapshot, ProcessID);

	// Open Process and Thread handles
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, ProcessID);
	if (hProcess == INVALID_HANDLE_VALUE) {
		ExitWithError("Could not open process handle");
	}
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, NULL, ThreadID);
	if (hThread == INVALID_HANDLE_VALUE) {
		ExitWithError("Could not open thread handle");
	}

	// Load DLL into process's memory
	LPVOID ptrDll = WriteDllInProcess(hProcess, dllPath);

	// Get LoadLibrary address 
	HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (hKernel32 == NULL) {
		ExitWithError("Could not get handle for kernel32.dll");
	}
	LPVOID ptrLoadLibrary = (LPVOID)GetProcAddress(hKernel32, LOAD_LIBRARY_VERSION);
	if (ptrLoadLibrary == NULL) {
		ExitWithError("Could not find LoadLibrary at kernel32.dll");
	}

	// Suspend thread
	DWORD suspendCount = SuspendThread(hThread);
	if (suspendCount == (DWORD)-1) {
		ExitWithError("Could not suspend process's thread");
	}

	// Inject DLL
	LPCONTEXT lpThreadContext = (LPCONTEXT)calloc(1, sizeof(CONTEXT));
	lpThreadContext->ContextFlags = CONTEXT_ALL;

	BOOL result = GetThreadContext(hThread, lpThreadContext);
	if (result == FALSE) {
		ExitWithError("Could not get thread context");
	}

	InjectToStack(hProcess, hThread, lpThreadContext, ptrDll, ptrLoadLibrary);
	ResumeThread(hThread);

	// Cleanup
	CloseHandle(hSnapshot);
	CloseHandle(hProcess);
	CloseHandle(hThread);

	system("pause");
	return 0;
}