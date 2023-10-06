#pragma once
#include<vector>
#include<sys/types.h>
//����ʹ��windowsģ��fork�����ȽϷ���������ʹ�ü򵥵�createprocess���棬������Ҫ���������Ĳ���

#ifdef _WIN32
#include<windows.h>
#define PRD _PROCESS_INFORMATION
#define _CRT_SECURE_NO_DEPRECATE
#else if __linux__
#include<unistd.h>
#include<sys/wait.h>
#define PRD pid_t
#endif

class Multi_Process final
{
    public:
        static Multi_Process* get_Instance(int num_processe=0);
        virtual ~Multi_Process();
        void run();

    private:
        static Multi_Process* p_instance;
        std::vector<PRD> m_vec_pids;
        Multi_Process(){};
        Multi_Process(int num_processes);

        void process_thread();
};