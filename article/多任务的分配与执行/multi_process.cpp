#include"multi_process.h"
#include<iostream>
using namespace std;
_CRT_SECURE_NO_DEPRECATE
Multi_Process* Multi_Process::p_instance = nullptr;


Multi_Process* Multi_Process::get_Instance(int num_processe/*=0*/)
{
	if (!p_instance) return new Multi_Process(num_processe);
	else return p_instance;
}

Multi_Process::~Multi_Process()
{
#ifdef _WIN32
    for (auto it:m_vec_pids)
    {
        // Close process and thread handles. 
        CloseHandle(it.hProcess);
        CloseHandle(it.hThread);
    }
	m_vec_pids.clear();
#endif
}

Multi_Process::Multi_Process(int num_processes)
{
	p_instance = this;
	while (num_processes--)
	{
#ifdef _WIN32
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        LPCWSTR start_exe(L"C:\\Bolg\\article\\多任务的分配与执行\\hello.exe");


        // Start the child process. 
        if (!CreateProcess(start_exe,   // No module name (use command line)
            NULL,        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi)           // Pointer to PROCESS_INFORMATION structure
            )
        {
            printf("CreateProcess failed (%d).\n", GetLastError());
            return;
        }

        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, INFINITE);

		m_vec_pids.emplace_back(pi);
#else if __linux__
		pid_t pid = fork();
		if (pid == -1)
		{
			//perror("fork");
			exit(-1);
		}
		m_vec_pids.emplace_back(pid);
#endif
	}
}


