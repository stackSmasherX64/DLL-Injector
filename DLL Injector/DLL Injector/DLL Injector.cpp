// DLL Injector.cpp : Defines the entry point for the console application.

#include "stdafx.h"
int InjectDLL(DWORD, char*);
int getDLLpath(char*);
int getPID(int*);
int getProc(HANDLE*, DWORD);
void enableSeDebug();

int main()
{
	//Escalate privlege 
	enableSeDebug();

	system("title Dll Injector");

	int PID = -1;
	char *dll = new char[255];

	getDLLpath(dll);
	getPID(&PID);

	InjectDLL(PID, dll);
	system("pause");

	return 0;
}

int getDLLpath(char* dll)
{
	cout << "Please enter the path to your DLL file\n";
	cin >> dll;
	return 1;
}

int getPID(int* PID)
{
	cout << "Please enter the PID to your target process\n";
	cin >> *PID;
	return 1;
}

int getProc(HANDLE* handleToProc, DWORD pid)
{

	//Create a handle to the process
	*handleToProc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	DWORD dwLastError = GetLastError();

	if (*handleToProc == NULL)
	{
		cout << "Unable to open process.\n";
		return -1;
	}
	else
	{
		cout << "process opened.\n";
		return 1;
	}
}


int InjectDLL(DWORD PID, char* dll)
{
	HANDLE handleToProc;
	LPVOID LoadLibAddr;
	LPVOID baseAddr;
	HANDLE remThread;
	
	//Get path length
	int dllLength = strlen(dll) + 1;

	//Get handle to process
	if (getProc(&handleToProc, PID) < 0)
		return -1;

	//Load kernel32 library
	LoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

	if (!LoadLibAddr)
		return -1;

	//Allocate memory for DLL injection
	baseAddr = VirtualAllocEx(handleToProc, NULL, dllLength, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (!baseAddr)
		return -1;

	//Write dll path
	if (!WriteProcessMemory(handleToProc, baseAddr, dll, dllLength, NULL))
		return -1;

	//Create remote thread
	remThread = CreateRemoteThread(handleToProc, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, baseAddr, 0, NULL);

	if (!remThread)
		return -1;

	//Wait untill DLL exits then deallocate memmory
	WaitForSingleObject(remThread, INFINITE);

	//Freing memmory
	VirtualFreeEx(handleToProc, baseAddr, dllLength, MEM_RELEASE);

	//Closing handles
	if (CloseHandle(remThread) == 0)
	{
		cout << "Failed to close handle to remote thread.\n";
		return -1;
	}

	if (CloseHandle(handleToProc) == 0)
	{
		cout << "Failed to close handle to target process.\n";
		return -1;
	}

	return 1;
}



void enableSeDebug()
{
	// Enable SeDebugPrivilege
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tokenPriv;
	LUID luidDebug;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken) != FALSE)
	{
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luidDebug) != FALSE)
		{
			tokenPriv.PrivilegeCount = 1;
			tokenPriv.Privileges[0].Luid = luidDebug;
			tokenPriv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, 0, NULL, NULL) != FALSE)
			{
				cout << "Successfully changed token privileges" << endl;
			}
			else
			{
				cout << "Failed to change token privileges, CODE: " << GetLastError() << endl;
			}
		}
	}
	CloseHandle(hToken);
}