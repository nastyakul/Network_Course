#pragma once
// ConsoleApplication1.cpp: 
//

#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <string>

#define BUFSIZE 10000 

static const char exit_phrase[] = "Terminate RCP";

enum
{
	WRITE_PIPE_THREAD,
	READ_PIPE_THREAD
};

class m_process;

struct client_type
{
	m_process* proc;
	std::thread m_threads[2];
	SOCKET socket;
};

class m_process
{
public:
	m_process();
	~m_process();

	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;

	SECURITY_ATTRIBUTES saAttr;

	void CreateChildProcess(void);
	void ErrorExit(PTSTR);

};

m_process::~m_process()
{
	if (!CloseHandle(g_hChildStd_IN_Rd))
		ErrorExit(TEXT("StdInWr CloseHandle"));
	if (!CloseHandle(g_hChildStd_IN_Wr))
		ErrorExit(TEXT("StdInWr CloseHandle"));
	if (!CloseHandle(g_hChildStd_OUT_Rd))
		ErrorExit(TEXT("StdInWr CloseHandle"));
	if (!CloseHandle(g_hChildStd_OUT_Wr))
		ErrorExit(TEXT("StdInWr CloseHandle"));

}

m_process::m_process()
{
	// Set the bInheritHandle flag so pipe handles are inherited. 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		ErrorExit(TEXT("StdoutRd CreatePipe"));

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(TEXT("Stdout SetHandleInformation"));


	// Create a pipe for the child process's STDIN. 
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		ErrorExit(TEXT("Stdin CreatePipe"));

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		ErrorExit(TEXT("Stdin SetHandleInformation"));

	// Create the child process. 
	CreateChildProcess();
}

void m_process::CreateChildProcess()
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
	TCHAR szCmdline[] = TEXT("cmd.exe");
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 

	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 

	bSuccess = CreateProcess(NULL,
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

					   // If an error occurs, exit the application. 
	if (!bSuccess)
		ErrorExit(TEXT("CreateProcess"));
	else
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
}

void m_process::ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box, 
// and exit from the application.
{
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
}

int ReadFromPipe(client_type* new_client)
{
	DWORD dwRead = 0, dwWritten = 0;
	CHAR chBuf[BUFSIZE] = {};
	BOOL bSuccess = FALSE;

	for (;;)
	{
		bSuccess = ReadFile(new_client -> proc -> g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess) break;

		printf("%.*s", dwRead, chBuf);

		if (strncmp(chBuf, exit_phrase, strlen(exit_phrase) - 1) == 0)
			break;

		if (new_client->socket != 0 && new_client->socket != INVALID_SOCKET)
		{
			send(new_client->socket, chBuf, dwRead, 0);
		}
	}
	
	closesocket(new_client->socket);
	new_client->socket = INVALID_SOCKET;

	new_client -> m_threads[READ_PIPE_THREAD].detach();
	return 0;
}

int WriteToPipe(client_type* new_client)
{
	DWORD dwRead, dwWritten;
	CHAR chBuf[BUFSIZE];
	CHAR terminate[] = "exit\r\n";
	BOOL bSuccess = FALSE;

	for (;;)
	{
		int iResult = recv(new_client->socket, chBuf, BUFSIZE, 0);

		if (iResult != SOCKET_ERROR && iResult != 0)
		{			
			bSuccess = WriteFile(new_client->proc->g_hChildStd_IN_Wr, chBuf, iResult, &dwWritten, NULL);
						
			if (!bSuccess || strncmp(chBuf, exit_phrase, strlen(exit_phrase) - 1) == 0)
			{
				bSuccess = WriteFile(new_client->proc->g_hChildStd_IN_Wr, terminate, strlen(terminate), &dwWritten, NULL);
				break;
			}
		}
		else			
			break;

	}

	closesocket(new_client->socket);
	new_client->socket = INVALID_SOCKET;

	new_client->m_threads[WRITE_PIPE_THREAD].detach();

	return 0;
}