#include <windows.h>	// for CRITICAL_SECTION
#include<string>
//#define _CRT_SECURE_NO_WARNINGS

// ȱʡ�ڴ���С�����ڸ�ֵʱ����������д������ļ�
#define SIZE_DEFAULT_MEM	(1024*64)		// 64K

class CFileMem
{
public:
	CFileMem();
	virtual ~CFileMem();

	const char* log_path;

public:
	bool setLogPath(std::string &ipath);

	// ���ڴ��ļ���dwMemBlockSizeΪ������С�����ڸ�ֵʱ����һ��IO����
	BOOL Open(const char* pszPathFile, DWORD dwMemBlockSize = SIZE_DEFAULT_MEM);

	/*
	 д���ݵ��ڴ��ļ���dwFileSize�����ص�ǰ�����ļ��Ĵ�С��
	 ��ֵ�������ⲿ�ж���־�ļ��Ƿ���󣬱��統dwFileSize���ڶ���Mʱ�����������ļ�
	 �Ӷ����ⵥ����־�ļ�����
	*/
	BOOL Write(const char* pszLog, DWORD& dwFileSize);
	BOOL Write(const unsigned char* pData, DWORD dwDataSize, DWORD& dwFileSize);

	// ���ڴ�����д�����
	BOOL Flush(DWORD& dwFileSize);

	// �ر��ڴ��ļ�
	void Close();

	// �������ļ�
	void Rename(const char* pszOldPathFile, const char* pszNewPathFile);

	//LogTIme(�򵥼�ʱ|��ʱ������ʹ��tinytools-measureTask)
	void LogTime();

	void quickLog(const char* istr);

protected:
	CRITICAL_SECTION m_csMem;		// �ڴ����
	char  m_szPathFile[256];		// ��־�ļ�·����
	char* m_pMemBlock;				// �ڴ�飬���ڴ洢�ļ�����
	DWORD m_dwMemBlockSize;			// �ڴ���С
	DWORD m_dwPos;					// �ڴ��ĵ�ǰλ��
};