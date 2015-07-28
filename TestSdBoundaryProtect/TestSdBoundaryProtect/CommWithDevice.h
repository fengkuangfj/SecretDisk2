#pragma once

#define MOD_COMM_WITH_DEVICE L"与设备通讯"

// #define DEVICE_NAME L"\\\\.\\HelloWDM"


class CCommWithDevice
{
public:
	BOOL
		SendMessage(
		__in LPWSTR	lpDeviceName,
		__in ULONG	ulType,
		__in LPVOID	lpInputBuf,
		__in ULONG	ulInputBufLenB
		);
};
