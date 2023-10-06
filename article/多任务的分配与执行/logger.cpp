#include "Logger.h"
#include <stdio.h>
#include<ctime>

CFileMem::CFileMem()
{
	m_pMemBlock = NULL;
	m_dwMemBlockSize = 0;
	m_dwPos = 0;
	memset(m_szPathFile, 0, 256);
	::InitializeCriticalSection(&m_csMem);
}

CFileMem::~CFileMem()
{
	Close();
	::DeleteCriticalSection(&m_csMem);
}

bool CFileMem::setLogPath(std::string& ipath)
{
	if (ipath.empty())return 0;
	CFileMem::log_path = ipath.c_str();
	return 1;
}

// ���ڴ��ļ���dwMemBlockSizeΪ������С�����ڸ�ֵʱ����һ��IO����
BOOL CFileMem::Open(const char* pszPathFile, DWORD dwMemBlockSize)
{
	if (!pszPathFile)
		return FALSE;

	// �ر�֮ǰ�Ѵ��ڴ��
	Close();

	// ������־�ļ�ȫ·����
	strcpy_s(m_szPathFile, pszPathFile);

	if (dwMemBlockSize <= 0)
		return FALSE;
	m_pMemBlock = (char*)malloc(dwMemBlockSize);
	if (NULL == m_pMemBlock)
		return FALSE;

	memset(m_pMemBlock, 0, dwMemBlockSize);
	m_dwMemBlockSize = dwMemBlockSize;
	m_dwPos = 0;
	return TRUE;
}

/*
	 д���ݵ��ڴ��ļ���dwFileSize�����ص�ǰ�����ļ��Ĵ�С��
	 ��ֵ�������ⲿ�ж���־�ļ��Ƿ���󣬱��統dwFileSize���ڶ���Mʱ�����������ļ�
	 �Ӷ����ⵥ����־�ļ�����
*/
BOOL CFileMem::Write(const char* pszLog, DWORD& dwFileSize)
{
	return Write((const unsigned char*)pszLog, strlen(pszLog), dwFileSize);
}
BOOL CFileMem::Write(const unsigned char* pData, DWORD dwDataSize, DWORD& dwFileSize)
{
	dwFileSize = 0;
	if (NULL == pData || 0 == dwDataSize)
		return FALSE;

	// ����ڲ�û�п��ٻ���������ֱ��д�ļ�
	if (NULL == m_pMemBlock || 0 == m_dwMemBlockSize)
	{
		FILE* pFile;
		::fopen_s(&pFile,m_szPathFile, "ab+");
		if (NULL == pFile)
			return FALSE;
		::fwrite(pData, 1, dwDataSize, pFile);
		::fwrite("\r\n", 1, sizeof("\r\n"), pFile);

		// ��ȡ�����ļ���С
		::fseek(pFile, 0L, SEEK_END);
		dwFileSize = ::ftell(pFile);
		::fclose(pFile);
		return TRUE;
	}

	::EnterCriticalSection(&m_csMem);

	// ����ڴ����������д������ļ�
	DWORD dwTotalSize = m_dwPos + dwDataSize;
	if (dwTotalSize >= m_dwMemBlockSize)
	{
		FILE* pFile;
		::fopen_s(&pFile,m_szPathFile, "ab+");
		if (NULL == pFile)
		{
			::LeaveCriticalSection(&m_csMem);
			return FALSE;
		}

		// ����ǰ�ڴ�������д���ļ�
		::fwrite(m_pMemBlock, 1, m_dwPos, pFile);
		::fwrite(pData, 1, dwDataSize, pFile);
		::fwrite("\r\n", 1, sizeof("\r\n"), pFile);

		// ��ȡ�����ļ���С
		::fseek(pFile, 0L, SEEK_END);
		dwFileSize = ::ftell(pFile);
		::fclose(pFile);

		// ����ڴ��
		memset(m_pMemBlock, 0, m_dwMemBlockSize);
		m_dwPos = 0;
	}
	else
	{
		// ����ڴ�δ��������ǰ����д���ڴ�
		memcpy(m_pMemBlock + m_dwPos, pData, dwDataSize);
		m_dwPos += dwDataSize;
	}
	::LeaveCriticalSection(&m_csMem);
	return TRUE;
}

// ��������������д�����
BOOL CFileMem::Flush(DWORD& dwFileSize)
{
	// ����
	dwFileSize = 0;
	if (NULL == m_pMemBlock || 0 == m_dwMemBlockSize || 0 == m_dwPos)
		return TRUE;

	::EnterCriticalSection(&m_csMem);
	FILE* pFile;
	::fopen_s(&pFile,m_szPathFile, "ab+");
	if (NULL == pFile)
	{
		::LeaveCriticalSection(&m_csMem);
		return FALSE;
	}
	// �ڴ�����д������ļ�
	::fwrite(m_pMemBlock, 1, m_dwPos, pFile);

	// ��ȡ�����ļ���С
	::fseek(pFile, 0L, SEEK_END);
	dwFileSize = ::ftell(pFile);
	::fclose(pFile);

	// ����ڴ��
	memset(m_pMemBlock, 0, m_dwMemBlockSize);
	m_dwPos = 0;

	::LeaveCriticalSection(&m_csMem);

	return TRUE;
}

void CFileMem::Close()
{
	// ���ڴ����ļ�д�����
	DWORD dwFileSize = 0;
	Flush(dwFileSize);

	// �ͷ��ڴ��
	free(m_pMemBlock);
	m_pMemBlock = NULL;
	m_dwMemBlockSize = 0;
	m_dwPos = 0;
	memset(m_szPathFile, 0, 256);
}

// �������ļ�
void CFileMem::Rename(const char* pszOldPathFile, const char* pszNewPathFile)
{
	::EnterCriticalSection(&m_csMem);
	::rename(pszOldPathFile, pszNewPathFile);
	::LeaveCriticalSection(&m_csMem);
}

void CFileMem::LogTime()
{
	time_t start;
	time(&start);
	char buf[26];
	ctime_s(buf, 26, &start);
	DWORD dwFileSize;
	Write("Time:", dwFileSize);
	Write(buf, dwFileSize);
	Write("\n", dwFileSize);
}

void CFileMem::quickLog(const char* istr)
{
	DWORD dwFileSize;
	Write(istr, dwFileSize);
}
