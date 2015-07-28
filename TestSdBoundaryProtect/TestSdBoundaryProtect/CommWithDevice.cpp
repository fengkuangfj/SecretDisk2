#include "stdafx.h"
#include "CommWithDevice.h"

BOOL
	CCommWithDevice::SendMessage(
	__in LPWSTR	lpDeviceName,
	__in ULONG	ulIoControlCode,
	__in LPVOID	lpInBuffer,
	__in ULONG	ulInBufferSizeB
	)
{
	BOOL	bRet	= FALSE;

	HANDLE	hDevice = INVALID_HANDLE_VALUE;


	__try
	{
		if (!lpDeviceName || !ulIoControlCode || !lpInBuffer || !ulInBufferSizeB)
			__leave;

		hDevice = CreateFile(
			lpDeviceName,
			SYNCHRONIZE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == hDevice)
		{
			printf("CreateFile failed. (%d) \n", GetLastError());
			__leave;
		}

		CopyMemory(((LPCOMM_INFO)lpInBuffer)->wchCheck, L"38f873n39873jof783jo9823hjoi2h9hjnf9", wcslen(L"38f873n39873jof783jo9823hjoi2h9hjnf9") * sizeof(WCHAR));

		bRet = DeviceIoControl(
			hDevice,
			ulIoControlCode,
			lpInBuffer,
			ulInBufferSizeB,
			NULL,
			0,
			NULL,
			NULL
			);
		if (!bRet)
		{
			printf("DeviceIoControl failed. (%d) \n", GetLastError());
			__leave;
		}
	}
	__finally
	{
		if (hDevice)
		{
			CloseHandle(hDevice);
			hDevice = INVALID_HANDLE_VALUE;
		}
	}

	return bRet;
}
