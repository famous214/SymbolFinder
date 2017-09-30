#include "ErrorHandler.h"

void ErrorHandler(LPTSTR lpszErrorMsg, DWORD	dwOperation)
{
	MessageBox(NULL, lpszErrorMsg, TEXT("Error!"), MB_OK | MB_ICONERROR);
	if (MY_ERROR_EXIT == dwOperation)
	{
		exit(0);
	}
	else
	{
		return;
	}
}