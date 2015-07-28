#include "stdafx.h"
#include "DriveControl.h"

BOOL
	CDriveControl::Install(
	__in LPWSTR lpServiceName,
	__in LPWSTR lpPath
	)
{
	BOOL			bRet				= FALSE;

	SC_HANDLE		hScManager			= NULL;
	SC_HANDLE		hService			= NULL;
	WCHAR			wchTemp[MAX_PATH]	= {0};
	HKEY			hkResult			= NULL;
	DWORD			dwData				= 0;
	LONG			lResult				= 0;


	__try
	{
		hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hScManager) 
		{
			printf("OpenSCManager failed. (%d) \n", GetLastError());
			__leave;
		}

		hService = CreateService(
			hScManager,
			lpServiceName,       
			NULL,      
			SERVICE_ALL_ACCESS,      
			SERVICE_FILE_SYSTEM_DRIVER, 
			SERVICE_AUTO_START,     
			SERVICE_ERROR_NORMAL,  
			lpPath,         
			L"FSFilter Activity Monitor",
			NULL, 
			L"FltMgr",               
			NULL, 
			NULL
			);
		if (!hService)
		{
			printf("CreateService failed. (%d) \n", GetLastError());
			__leave;
		}

		wcscat_s(wchTemp, _countof(wchTemp), L"SYSTEM\\CurrentControlSet\\services\\");
		wcscat_s(wchTemp, _countof(wchTemp), lpServiceName);
		wcscat_s(wchTemp, _countof(wchTemp), L"\\Instances");

		lResult = RegCreateKeyEx(
			HKEY_LOCAL_MACHINE,
			wchTemp,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hkResult,
			NULL
			);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegCreateKeyEx failed. (%d) \n", lResult);
			__leave;
		}

		if (!hkResult)
			__leave;

 		lResult = RegFlushKey(hkResult);
 		if (ERROR_SUCCESS != lResult)
		{
			printf("RegFlushKey failed. (%d) \n", lResult);
			__leave;
		}

		ZeroMemory(wchTemp, sizeof(wchTemp));
		wcscat_s(wchTemp, _countof(wchTemp), lpServiceName);
		wcscat_s(wchTemp, _countof(wchTemp), L" Instance");

		lResult = RegSetValueEx(
			hkResult,
			L"DefaultInstance",
			0,
			REG_SZ,
			(const BYTE*)wchTemp,
			wcslen(wchTemp) * sizeof(WCHAR)
			);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegSetValueEx failed. (%d) \n", lResult);
			__leave;
		}

		lResult = RegFlushKey(hkResult);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegFlushKey failed. (%d) \n", lResult);
			__leave;
		}

		lResult = RegCloseKey(hkResult);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegCloseKey failed. (%d) \n", lResult);
			__leave;
		}

		hkResult = NULL;

		ZeroMemory(wchTemp, sizeof(wchTemp));
		wcscat_s(wchTemp, _countof(wchTemp), L"SYSTEM\\CurrentControlSet\\services\\");
		wcscat_s(wchTemp, _countof(wchTemp), lpServiceName);
		wcscat_s(wchTemp, _countof(wchTemp), L"\\Instances\\");
		wcscat_s(wchTemp, _countof(wchTemp), lpServiceName);
		wcscat_s(wchTemp, _countof(wchTemp), L" Instance");

		lResult = RegCreateKeyEx(
			HKEY_LOCAL_MACHINE,
			wchTemp,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hkResult,
			NULL
			);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegCreateKeyEx failed. (%d) \n", lResult);
			__leave;
		}

		if (!hkResult)
			__leave;

		lResult = RegFlushKey(hkResult);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegFlushKey failed. (%d) \n", lResult);
			__leave;
		}

		ZeroMemory(wchTemp, sizeof(wchTemp));
		wcscat_s(wchTemp, _countof(wchTemp), L"370030");

		lResult = RegSetValueEx(
			hkResult,
			L"Altitude",
			0,
			REG_SZ,
			(const BYTE*)wchTemp,
			wcslen(wchTemp) * sizeof(WCHAR)
			);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegSetValueEx failed. (%d) \n", lResult);
			__leave;
		}

		lResult = RegFlushKey(hkResult);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegFlushKey failed. (%d) \n", lResult);
			__leave;
		}

		dwData = 0x0;

		lResult = RegSetValueEx(
			hkResult,
			L"Flags",
			0,
			REG_DWORD,
			(const BYTE*)&dwData,
			sizeof(DWORD)
			);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegSetValueEx failed. (%d) \n", lResult);
			__leave;
		}

		lResult = RegFlushKey(hkResult);
		if (ERROR_SUCCESS != lResult)
		{
			printf("RegFlushKey failed. (%d) \n", lResult);
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hkResult)
		{
			RegCloseKey(hkResult);
			hkResult = NULL;
		}

		if (hService)
		{
			CloseServiceHandle(hService);
			hService = NULL;
		}

		if (hScManager)
		{
			CloseServiceHandle(hScManager);
			hScManager = NULL;
		}
	}

	return bRet;
}

BOOL
	CDriveControl::Start(
	__in LPWSTR lpServiceName
	)
{
	BOOL		bRet		= FALSE;

	SC_HANDLE	hScManager	= NULL;
	SC_HANDLE	hService	= NULL;


	__try
	{
		hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hScManager)
		{
			printf("OpenSCManager failed. (%d) \n", GetLastError);
			__leave;
		}

		hService = OpenService(hScManager, lpServiceName, SERVICE_ALL_ACCESS);
		if (!hService)
		{
			printf("OpenService failed. (%d) \n", GetLastError);
			__leave;
		}

		if (!StartService(hService, 0, NULL))
		{
			printf("StartService failed. (%d) \n", GetLastError());
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hService)
		{
			CloseServiceHandle(hService);
			hService = NULL;
		}

		if (hScManager)
		{
			CloseServiceHandle(hScManager);
			hScManager = NULL;
		}
	}

	return bRet;
}

BOOL
	CDriveControl::Stop(
	__in LPWSTR lpServiceName
	)
{
	BOOL			bRet			= FALSE;

	SC_HANDLE		hScManager		= NULL;
	SC_HANDLE		hService		= NULL;
	SERVICE_STATUS	ServiceStatus	= {0};


	__try
	{
		hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hScManager)
		{
			printf("OpenSCManager failed. (%d) \n", GetLastError());
			__leave;
		}

		hService = OpenService(hScManager, lpServiceName, SERVICE_ALL_ACCESS);
		if (!hService)
		{
			printf("OpenService failed. (%d) \n", GetLastError());
			__leave;
		}

		if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus))
		{
			printf("ControlService failed. (%d) \n", GetLastError());
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hService)
		{
			CloseServiceHandle(hService);
			hService = NULL;
		}

		if (hScManager)
		{
			CloseServiceHandle(hScManager);
			hScManager = NULL;
		}
	}

	return bRet;
}

BOOL
	CDriveControl::Delete(
	__in LPWSTR lpServiceName
	)
{
	BOOL		bRet		= FALSE;

	SC_HANDLE	hScManager	= NULL;
	SC_HANDLE	hService	= NULL;


	__try
	{
		hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (!hScManager)
		{
			printf("OpenSCManager failed. (%d) \n", GetLastError());
			__leave;
		}

		hService = OpenService(hScManager, lpServiceName, SERVICE_ALL_ACCESS);
		if (!hService)
		{
			printf("OpenService failed. (%d) \n", GetLastError());
			__leave;
		}

		if (!DeleteService(hService))
		{
			printf("DeleteService failed. (%d) \n", GetLastError());
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hService)
		{
			CloseServiceHandle(hService);
			hService = NULL;
		}

		if (hScManager)
		{
			CloseServiceHandle(hScManager);
			hScManager = NULL;
		}
	}

	return bRet;
}
