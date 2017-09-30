#include <stdio.h>
#include "MapFileToMem.h"
#include "Resolver.h"

int main()
{
	LPTSTR pszTextObjPath = TEXT("G:\\G\\毕业设计\\测试用例\\BasicType.obj");
	HandleObjFile(pszTextObjPath);

	HandleLibFile("G:\\G\\毕业设计\\测试用例\\LIBC.LIB", "printf");
	
	return 0;
}