#pragma once
#include <windows.h>

#define		MY_ERROR_EXIT			0
#define		MY_ERROR_RETRY			1

void	ErrorHandler(LPTSTR lpszErrorMsg, DWORD	 dwOperation);