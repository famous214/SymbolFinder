#include <tchar.h>  
#include <Windows.h>  
#include <Shlwapi.h>  
#pragma comment( lib, "Shlwapi.lib" )  
#include <strsafe.h>  

BOOL FindAllFilesInDirectory(LPCTSTR pszDirPath)
{
	// 递归边界条件：路径为NULL或不存在，返回FALSE
	if (NULL == pszDirPath || FALSE == PathFileExists(pszDirPath))
	{
		return FALSE;
	}

	// 遍历本路径下的所有文件  
	WIN32_FIND_DATA FindData;
	TCHAR Directory[MAX_PATH];
	StringCbPrintf(Directory, sizeof(Directory), TEXT("%s\\*.*"), pszDirPath);

	// 开始迭代查询本路径下的所有文件与文件夹
	HANDLE hFile = FindFirstFile(Directory, &FindData);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	TCHAR szSubItemPath[MAX_PATH];
	do
	{
		// 过滤掉当前目录和上级目录的文件夹 
		if (TEXT('.') == FindData.cFileName[0])
		{
			continue;
		}

		// 得到当前项的完整路径
		StringCbPrintf(szSubItemPath, sizeof(szSubItemPath), TEXT("%s\\%s"), pszDirPath, FindData.cFileName);

		// 判断当前找到的是文件还是文件夹，若是文件，则执行查找策略
		if (0 == (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			_tprintf(TEXT("find file: %s size:%d bytes.\n"), szSubItemPath, FindData.nFileSizeLow);
		}
		// 若是文件夹，则递归遍历其该子目录
		else
		{
			_tprintf(TEXT("find directory: %s.\n"), szSubItemPath);
			//继续递归这个文件夹  
			if (false == FindAllFilesInDirectory(szSubItemPath))
			{
				return FALSE;
			}
		}
	} while (FALSE != FindNextFile(hFile, &FindData));
	//检查退出循环条件是否是没有文件了否则出错  
	if (ERROR_NO_MORE_FILES != GetLastError())
	{
		return FALSE;
	}
	FindClose(hFile);
	return TRUE;
}