#pragma once
#include	<tchar.h>
#include	"ErrorHandler.h"

#define		INVALID_HANDLE		-1
#define		INVALID_FILE		-2


typedef struct _STMEMFILEHANDLE			//FileToMemMap返回的结构类型
{
	LPTSTR		lpszFilePath;			//文件全路径，包含文件名
	HANDLE		hFile;					//文件句柄
	HANDLE		hMapFile;				//文件内存映射句柄
	LPSTR		lpFile;					//文件指针
	DWORD		dwFileSize;				//文件大小
}STMEMFILEHANDLE, *PSTMEMFILEHANDLE;


PSTMEMFILEHANDLE	FileToMemMap(LPTSTR lpszFilePath);
VOID				CloseFileMap(PSTMEMFILEHANDLE pHMemHandle);
DWORD	GetMemFilePointer(PSTMEMFILEHANDLE pMemFileHandle);
LPTSTR				SelectFile(PDWORD	dwExtNamePos); 
