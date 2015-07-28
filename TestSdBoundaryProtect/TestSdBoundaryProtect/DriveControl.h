#pragma once

#define MOD_DRIVE_CONTROL L"Çý¶¯¿ØÖÆ"

class CDriveControl
{
public:
	BOOL
		Install(
		__in LPWSTR lpServiceName,
		__in LPWSTR lpPath
		);

	BOOL
		Start(
		__in LPWSTR lpServiceName
		);

	BOOL
		Stop(
		__in LPWSTR lpServiceName
		);

	BOOL
		Delete(
		__in LPWSTR lpServiceName
		);
};
