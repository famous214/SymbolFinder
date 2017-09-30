#include "MapFileToMem.h"

/*
弹出打开文件对话框，供用户选择OBJ文件。
2016.04.12	修复Bug：stOpenFileName.lpstrFile传入的必须是全空字符串
*/
LPTSTR	SelectFile(PDWORD	dwExtNamePos)
{
	OPENFILENAME	stOpenFileName = { 0 };
	LPTSTR	lpFilePath = (LPTSTR)malloc(MAX_PATH * sizeof(TCHAR));
	if (!lpFilePath)
	{
		ErrorHandler(TEXT("无法分配用于存储OBJ路径的内存空间，请重新运行程序！\n"), MY_ERROR_EXIT);
	}
	RtlZeroMemory(lpFilePath, MAX_PATH * sizeof(TCHAR));

	stOpenFileName.lpstrFilter = TEXT("Obj Files(*.obj)\0*.obj\0All Files(*.*)\0*.*\0\0");
	stOpenFileName.lStructSize = sizeof(OPENFILENAME);
	stOpenFileName.lpstrFile = lpFilePath;
	stOpenFileName.nMaxFile = MAX_PATH - 1;
	stOpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	while (!GetOpenFileName(&stOpenFileName))
	{
		ErrorHandler(TEXT("您未选中任何OBJ文件，请重试！\n"), MY_ERROR_RETRY);
	}
	if (dwExtNamePos)
	{
		*dwExtNamePos = stOpenFileName.nFileExtension;
	}
	return lpFilePath;
}

PSTMEMFILEHANDLE FileToMemMap(LPTSTR lpszFilePath)
{
	PSTMEMFILEHANDLE pstMemFileHandle = NULL;
	LPTSTR	lpFilePath = NULL;
	if (lpszFilePath)
	{
		pstMemFileHandle = (PSTMEMFILEHANDLE)malloc(sizeof(STMEMFILEHANDLE));
		UINT	dwPathLen = _tcslen(lpszFilePath);
		lpFilePath = (LPTSTR)calloc((dwPathLen + 1), sizeof(LPTSTR));

		if (pstMemFileHandle && lpFilePath)
		{
			RtlCopyMemory(lpFilePath, lpszFilePath, dwPathLen * sizeof(LPTSTR));
			pstMemFileHandle->lpszFilePath = lpFilePath;
		}
		else
		{
			if (pstMemFileHandle)
			{
				free(pstMemFileHandle);
			}
			if (lpFilePath)
			{
				free(lpFilePath);
			}
			ErrorHandler(TEXT("将OBJ载入内存时发生错误，没有足够的内存空间！"), MY_ERROR_EXIT);
		}
	}
	else
	{
		return NULL;
	}
	HANDLE	hFile = CreateFile(lpszFilePath, GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		free(pstMemFileHandle);
		free(lpFilePath);
		ErrorHandler(TEXT("将OBJ载入内存时发生错误，无法打开OBJ文件！"), MY_ERROR_EXIT);
	}
	pstMemFileHandle->dwFileSize = GetFileSize(hFile, NULL);

	HANDLE	hMapFile = CreateFileMapping(hFile, NULL, PAGE_WRITECOPY, 0, 0, NULL);
	if (NULL == hMapFile)
	{
		CloseHandle(hFile);
		free(pstMemFileHandle);
		free(lpFilePath);
		ErrorHandler(TEXT("将OBJ载入内存时发生错误，无法建立内存映射！"), MY_ERROR_EXIT);
	}

	LPSTR	lpFile = (LPSTR)MapViewOfFile(hMapFile, FILE_MAP_COPY, 0, 0, 0);
	if (NULL == lpFile)
	{
		CloseHandle(hMapFile);
		CloseHandle(hFile);
		free(pstMemFileHandle);
		free(lpFilePath);
		ErrorHandler(TEXT("将OBJ载入内存时发生错误，无法访问该文件的内存映射！"), MY_ERROR_EXIT);
	}

	pstMemFileHandle->hFile = hFile;
	pstMemFileHandle->hMapFile = hMapFile;
	pstMemFileHandle->lpFile = lpFile;
	return pstMemFileHandle;
}

DWORD	GetMemFilePointer(PSTMEMFILEHANDLE pMemFileHandle)
{
	if (pMemFileHandle)
		return (DWORD)pMemFileHandle->lpFile;
	else
		return NULL;
}

VOID	CloseFileMap(PSTMEMFILEHANDLE pstMemFileHandle)
{
	UnmapViewOfFile(pstMemFileHandle->lpFile);
	CloseHandle(pstMemFileHandle->hMapFile);
	CloseHandle(pstMemFileHandle->hFile);
	free(pstMemFileHandle->lpszFilePath);
	free(pstMemFileHandle);
}