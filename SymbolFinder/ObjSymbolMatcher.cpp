#include <windows.h>
#include <stdio.h>
#include "MapFileToMem.h"
#define		TYPE_NOT_CARE						0
#define		TYPE_EXTERN_GLOBAL_VAR				1
#define		TYPE_EXTERN_FUNC					2
#define		TYPE_LOCAL_FUNC						3
#define		TYPE_LOCAL_GLOBAL_VAR				4
#define		TYPE_LOCAL_GLOBAL_VAR_INSEC			5
#define		TYPE_LOCAL_STATIC_VAR				6
#define		TYPE_LOCAL_LABEL					7

typedef struct _STOBJFILEINFO
{
	PIMAGE_SECTION_HEADER	pFirstSecHeader;				//指向第一个区段的首部
	DWORD					dwSecNum;						//总的区段数
	PIMAGE_SYMBOL			pSymbolTable;					//指向符号表
	DWORD					dwSymbolNum;					//总共的符号数
	LPSTR					pSymbolStrTable;				//指向长名符号名表
}STOBJFILEINFO, *PSTOBJFILEINFO;


BOOL GetObjHeaderInfo(IN PIMAGE_FILE_HEADER pObjFile, OUT PSTOBJFILEINFO pstObjFileInfo)
{
	if ((pObjFile->Machine != IMAGE_FILE_MACHINE_I386))		//是OBJ文件且内存分配成功
	{
		return FALSE;
		//ErrorHandler(TEXT("读入的OBJ文件格式非法！"), ERROR_EXIT);
	}
	pstObjFileInfo->dwSecNum = pObjFile->NumberOfSections;
	pstObjFileInfo->dwSymbolNum = pObjFile->NumberOfSymbols;
	pstObjFileInfo->pSymbolTable = (PIMAGE_SYMBOL)((LPSTR)pObjFile + pObjFile->PointerToSymbolTable);
	pstObjFileInfo->pSymbolStrTable = (LPSTR)pstObjFileInfo->pSymbolTable + ((pstObjFileInfo->dwSymbolNum) * sizeof(IMAGE_SYMBOL));
}

LPSTR GetSymbolName(LPSTR lpNameOrIndex, LPSTR lpSymStrTable)
{
	if (*(DWORD*)lpNameOrIndex)		//短名符号,截断字符串手工末尾添加NULL,由于破坏了Value的值,由调用方提前保存
	{
		lpNameOrIndex[8] = (CHAR)NULL;
		return lpNameOrIndex;
	}
	else								//长名符号,直接查到符号名表返回其地址
	{
		return (LPSTR)(lpSymStrTable + (*(PUINT32)(lpNameOrIndex + 4)));
	}
}

DWORD CheckSymbolType(PIMAGE_SYMBOL pSymTable)			//构建符号项目并完成模块内函数VA和变量内存的分配
{
	if (pSymTable->StorageClass == IMAGE_SYM_CLASS_EXTERNAL)			//是外部符号
	{
		if (pSymTable->Type == IMAGE_SYM_DTYPE_FUNCTION << 4)			//为函数类型
		{
			if (pSymTable->SectionNumber == 0)							//引用其他OBJ中的函数
			{
				return TYPE_EXTERN_FUNC;
			}
			else if (pSymTable->SectionNumber > 0)						//本OBJ内定义的函数
			{
				return TYPE_LOCAL_FUNC;
			}
		}
		else if (pSymTable->Type == IMAGE_SYM_DTYPE_NULL)
		{
			if (pSymTable->SectionNumber == 0)
			{
				if (pSymTable->Value == 0)
				{
					return TYPE_EXTERN_GLOBAL_VAR;			//引用其他OBJ中的变量
				}
				else
				{
					return TYPE_LOCAL_GLOBAL_VAR;			//本OBJ内定义的变量,不属于任何区段
				}
			}
			if (pSymTable->SectionNumber > 0)
			{
				return TYPE_LOCAL_GLOBAL_VAR_INSEC;			 //本OBJ内定义的变量, 在数据段中
			}
		}
	}
	else if (pSymTable->StorageClass == IMAGE_SYM_CLASS_STATIC)
	{
		if (pSymTable->Type == IMAGE_SYM_DTYPE_NULL)
		{
			if (pSymTable->SectionNumber > 0)
			{
				//return TYPE_LOCAL_GLOBAL_VAR_INSEC;			//本OBJ内定义的变量,不属于任何区段
				return TYPE_LOCAL_STATIC_VAR;
			}
		}
	}
	else if (pSymTable->StorageClass == IMAGE_SYM_CLASS_LABEL)
	{
		//return TYPE_LOCAL_GLOBAL_VAR_INSEC;
		return TYPE_LOCAL_LABEL;
	}
	return 0;
}

DWORD BuildSymbolItemTable(PIMAGE_SYMBOL pSymTable, LPSTR lpSymStrTable)
{
	DWORD dwSymType = CheckSymbolType(pSymTable);

	DWORD dwValue = pSymTable->Value;	
	DWORD dwSection = pSymTable->SectionNumber;
	LPSTR lpszSymbolName = GetSymbolName((LPSTR)&(pSymTable->N), lpSymStrTable);
	switch (dwSymType)
	{
	case	TYPE_EXTERN_GLOBAL_VAR:					//引用其他OBJ中的变量
		printf("发现在模块外定义的全局变量:%s\n", lpszSymbolName);
		break;
	case	TYPE_EXTERN_FUNC:
		printf("发现在模块外定义的函数:%s\n", lpszSymbolName);
		break;
	case	TYPE_LOCAL_FUNC:						//本OBJ内定义的函数,算出其VA
		printf("发现在本模块中定义的函数:%s\n", lpszSymbolName);
		break;
	case	TYPE_LOCAL_GLOBAL_VAR:
		printf("发现单独定义的全局未初始化变量:%s\n", lpszSymbolName);
		break;
	case	TYPE_LOCAL_GLOBAL_VAR_INSEC:			//本OBJ内定义的变量,在数据段中,即便是为初始化的数据段在此时也被分配了空间,直接算其VA即可
	case	TYPE_LOCAL_STATIC_VAR:
	case	TYPE_LOCAL_LABEL:
		printf("发现在本模块数据段中的全局变量:%s\n", lpszSymbolName);
		break;
	}
	return dwSymType;
}

DWORD ResolveSymbolTable(PSTOBJFILEINFO pstObjFileInfo)
{
	DWORD			dwSymNum = pstObjFileInfo->dwSymbolNum;				//本模块的符号数
	PIMAGE_SYMBOL	pSymTable = pstObjFileInfo->pSymbolTable;			//本模块的符号表指针
	LPSTR			pSymStrTable = pstObjFileInfo->pSymbolStrTable;		//本模块的长名符号字符串表

	for (DWORD i = 0; i < dwSymNum; i++)									//遍历符号表,找出函数和变量
	{
		BuildSymbolItemTable(&pSymTable[i], pstObjFileInfo->pSymbolStrTable);
		i = i + pSymTable[i].NumberOfAuxSymbols;							//跳过辅助符号项
	}
	return 0;
}

BOOL HandleObjFile(LPTSTR pszObjFilePath)
{
	PSTMEMFILEHANDLE hObjFile = FileToMemMap(pszObjFilePath);
	STOBJFILEINFO stObjFileInfo;
	GetObjHeaderInfo((PIMAGE_FILE_HEADER)GetMemFilePointer(hObjFile), &stObjFileInfo);
	ResolveSymbolTable(&stObjFileInfo);
	CloseFileMap(hObjFile);
	return TRUE;
}