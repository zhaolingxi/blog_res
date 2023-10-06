#include <windows.h>	// for CRITICAL_SECTION
#include<string>
//#define _CRT_SECURE_NO_WARNINGS

// 缺省内存块大小，大于该值时将缓存数据写入磁盘文件
#define SIZE_DEFAULT_MEM	(1024*64)		// 64K

class CFileMem
{
public:
	CFileMem();
	virtual ~CFileMem();

	const char* log_path;

public:
	bool setLogPath(std::string &ipath);

	// 打开内存文件，dwMemBlockSize为缓存块大小，大于该值时进行一次IO操作
	BOOL Open(const char* pszPathFile, DWORD dwMemBlockSize = SIZE_DEFAULT_MEM);

	/*
	 写数据到内存文件，dwFileSize将返回当前磁盘文件的大小，
	 该值可用于外部判断日志文件是否过大，比如当dwFileSize大于多少M时，可重命名文件
	 从而避免单个日志文件过大
	*/
	BOOL Write(const char* pszLog, DWORD& dwFileSize);
	BOOL Write(const unsigned char* pData, DWORD dwDataSize, DWORD& dwFileSize);

	// 将内存数据写入磁盘
	BOOL Flush(DWORD& dwFileSize);

	// 关闭内存文件
	void Close();

	// 重命名文件
	void Rename(const char* pszOldPathFile, const char* pszNewPathFile);

	//LogTIme(简单计时|计时功能请使用tinytools-measureTask)
	void LogTime();

	void quickLog(const char* istr);

protected:
	CRITICAL_SECTION m_csMem;		// 内存池锁
	char  m_szPathFile[256];		// 日志文件路径名
	char* m_pMemBlock;				// 内存块，用于存储文件数据
	DWORD m_dwMemBlockSize;			// 内存块大小
	DWORD m_dwPos;					// 内存块的当前位置
};