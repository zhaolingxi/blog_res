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

// 打开内存文件，dwMemBlockSize为缓存块大小，大于该值时进行一次IO操作
BOOL CFileMem::Open(const char* pszPathFile, DWORD dwMemBlockSize)
{
	if (!pszPathFile)
		return FALSE;

	// 关闭之前已打开内存块
	Close();

	// 保存日志文件全路径名
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
	 写数据到内存文件，dwFileSize将返回当前磁盘文件的大小，
	 该值可用于外部判断日志文件是否过大，比如当dwFileSize大于多少M时，可重命名文件
	 从而避免单个日志文件过大
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

	// 如果内部没有开辟缓冲区，则直接写文件
	if (NULL == m_pMemBlock || 0 == m_dwMemBlockSize)
	{
		FILE* pFile;
		::fopen_s(&pFile,m_szPathFile, "ab+");
		if (NULL == pFile)
			return FALSE;
		::fwrite(pData, 1, dwDataSize, pFile);
		::fwrite("\r\n", 1, sizeof("\r\n"), pFile);

		// 获取磁盘文件大小
		::fseek(pFile, 0L, SEEK_END);
		dwFileSize = ::ftell(pFile);
		::fclose(pFile);
		return TRUE;
	}

	::EnterCriticalSection(&m_csMem);

	// 如果内存块已满，则写入磁盘文件
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

		// 将当前内存中数据写入文件
		::fwrite(m_pMemBlock, 1, m_dwPos, pFile);
		::fwrite(pData, 1, dwDataSize, pFile);
		::fwrite("\r\n", 1, sizeof("\r\n"), pFile);

		// 获取磁盘文件大小
		::fseek(pFile, 0L, SEEK_END);
		dwFileSize = ::ftell(pFile);
		::fclose(pFile);

		// 清空内存块
		memset(m_pMemBlock, 0, m_dwMemBlockSize);
		m_dwPos = 0;
	}
	else
	{
		// 如果内存未满，将当前数据写入内存
		memcpy(m_pMemBlock + m_dwPos, pData, dwDataSize);
		m_dwPos += dwDataSize;
	}
	::LeaveCriticalSection(&m_csMem);
	return TRUE;
}

// 将缓冲区的内容写入磁盘
BOOL CFileMem::Flush(DWORD& dwFileSize)
{
	// 参数
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
	// 内存数据写入磁盘文件
	::fwrite(m_pMemBlock, 1, m_dwPos, pFile);

	// 获取磁盘文件大小
	::fseek(pFile, 0L, SEEK_END);
	dwFileSize = ::ftell(pFile);
	::fclose(pFile);

	// 清空内存块
	memset(m_pMemBlock, 0, m_dwMemBlockSize);
	m_dwPos = 0;

	::LeaveCriticalSection(&m_csMem);

	return TRUE;
}

void CFileMem::Close()
{
	// 将内存中文件写入磁盘
	DWORD dwFileSize = 0;
	Flush(dwFileSize);

	// 释放内存块
	free(m_pMemBlock);
	m_pMemBlock = NULL;
	m_dwMemBlockSize = 0;
	m_dwPos = 0;
	memset(m_szPathFile, 0, 256);
}

// 重命名文件
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
