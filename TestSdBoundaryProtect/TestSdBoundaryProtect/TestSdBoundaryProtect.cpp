// TestSdBoundaryProtect.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

BOOL
	TestDriveControl()
{
	BOOL			bRet			= FALSE;

	CDriveControl	DriveControl;


	__try
	{
		if (!DriveControl.Install(L"SdBoundaryProtect", L"C:\\Users\\Administrator\\Desktop\\SdBoundaryProtect.sys"))
		{
			printf("Install failed \n");
			__leave;
		}
		else
			printf("Install succeed \n");

		if (!DriveControl.Start(L"SdBoundaryProtect"))
		{
			printf("Start failed \n");
			__leave;
		}
		else
			printf("Start succeed \n");

		if (!DriveControl.Stop(L"SdBoundaryProtect"))
		{
			printf("Stop failed \n");
			__leave;
		}
		else
			printf("Stop succeed \n");

		if (!DriveControl.Delete(L"SdBoundaryProtect"))
		{
			printf("Delete failed \n");
			__leave;
		}
		else
			printf("Delete succeed \n");

		bRet = TRUE;
	}
	__finally
	{
		;
	}
	
	return bRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	COMM_INFO		CommInfo		= {0};

	CDriveControl	DriveControl;
	CCommWithDevice CommWithDevice;


	__try
	{
		DriveControl.Install(L"SdBoundaryProtect", L"C:\\Users\\Administrator\\Desktop\\SdBoundaryProtect.sys");

		DriveControl.Start(L"SdBoundaryProtect");

		CommWithDevice.SendMessage(
			L"\\\\.\\SdBoundaryProtect",
			IOCTL_UM_START,
			&CommInfo,
			sizeof(CommInfo)
			);

		CopyMemory(CommInfo.Dir.wchFileName, L"c:\\1", wcslen(L"c:\\1") * sizeof(WCHAR));
		CommInfo.Dir.DirControlType = DIR_CONTROL_TYPE_ACCESS;

		CommWithDevice.SendMessage(
			L"\\\\.\\Device\\SdBoundaryProtect",
			IOCTL_UM_DIR_ADD,
			&CommInfo,
			sizeof(CommInfo)
			);

		CommInfo.Proc.ulPid = 2;

		CommWithDevice.SendMessage(
			L"\\Device\\SdBoundaryProtect",
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			sizeof(CommInfo)
			);

		CommWithDevice.SendMessage(
			L"Device\\SdBoundaryProtect",
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			sizeof(CommInfo)
			);

		CommWithDevice.SendMessage(
			L"SdBoundaryProtect",
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			sizeof(CommInfo)
			);
	}
	__finally
	{
		;
	}

	_getch();

	return 0;
}
