#define _LITTLE_ENDIAN
#include "staticlib.h"
#include <windows.h>

BOOL HandleLibFile(LPSTR pszObjFilePath, LPSTR pszDstSymbol)
{
	ArchParser parser;
	if (!parser.load(pszObjFilePath))
	{
		printf("cannot parse file corretly\n");
		return FALSE;
	}

	LPSTR objName = parser.findSymbol(pszDstSymbol);
	if (objName == NULL)
		printf("cannot find\n");
	else
		printf("symbol %s in obj %s\n", pszDstSymbol, objName);

	return TRUE;
}