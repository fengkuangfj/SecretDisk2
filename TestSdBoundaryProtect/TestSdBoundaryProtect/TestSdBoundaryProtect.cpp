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
	ULONG			ulCount			= 0;
	ULONG			i				= 0;
	LPCOMM_INFO		lpCommInfo		= NULL;
	BOOLEAN			bResult			= FALSE;
	LPCOMM_INFO		lpCommInfoTemp	= NULL;

	CDriveControl	DriveControl;
	CComm			Comm;


	__try
	{
		DriveControl.Stop(L"SdBoundaryProtect");

// 		DriveControl.Delete(L"SdBoundaryProtect");
// 
// 		DriveControl.Install(L"SdBoundaryProtect", L"C:\\Users\\Administrator\\Desktop\\SdBoundaryProtect.sys");

		DriveControl.Start(L"SdBoundaryProtect");

		// 通讯初始化
		Comm.Init();

		// 通知驱动开始工作
		bResult = Comm.SendMsg(
			IOCTL_UM_START,
			NULL,
			NULL,
			0,
			NULL
			);

		// 拒绝访问c:\\1
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CopyMemory(CommInfo.Dir.wchFileName, L"c:\\1", wcslen(L"c:\\1") * sizeof(WCHAR));
		CommInfo.Dir.DirControlType = DIR_CONTROL_TYPE_ACCESS;

		bResult = Comm.SendMsg(
			IOCTL_UM_DIR_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 隐藏c:\\2
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CopyMemory(CommInfo.Dir.wchFileName, L"c:\\2", wcslen(L"c:\\2") * sizeof(WCHAR));
		CommInfo.Dir.DirControlType = DIR_CONTROL_TYPE_HIDE;

		bResult = Comm.SendMsg(
			IOCTL_UM_DIR_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 拒绝访问c:\\3，同时隐藏
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CopyMemory(CommInfo.Dir.wchFileName, L"c:\\3", wcslen(L"c:\\3") * sizeof(WCHAR));
		CommInfo.Dir.DirControlType = (DIR_CONTROL_TYPE)(DIR_CONTROL_TYPE_ACCESS | DIR_CONTROL_TYPE_HIDE);

		bResult = Comm.SendMsg(
			IOCTL_UM_DIR_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 获取所有保护目录
		bResult = Comm.SendMsg(
			IOCTL_UM_DIR_GET,
			NULL,
			NULL,
			0,
			&ulCount
			);
		if (!bResult)
		{
			lpCommInfo = (LPCOMM_INFO)calloc(1, sizeof(COMM_INFO) * ulCount);

			bResult = Comm.SendMsg(
				IOCTL_UM_DIR_GET,
				NULL,
				lpCommInfo,
				ulCount,
				&ulCount
				);
		}
		
		if (bResult)
		{
			for (; i < ulCount; i++)
			{
				lpCommInfoTemp = lpCommInfo + i;

				printf("%x, %S \n", lpCommInfoTemp->Dir.DirControlType, lpCommInfoTemp->Dir.wchFileName);
			}
		}

		// 隐藏c:\\1
// 		ZeroMemory(&CommInfo, sizeof(CommInfo));
// 		CopyMemory(CommInfo.Dir.wchFileName, L"c:\\1", wcslen(L"c:\\1") * sizeof(WCHAR));
// 		CommInfo.Dir.DirControlType = DIR_CONTROL_TYPE_HIDE;
// 
// 		Comm.SendMsg(
// 			IOCTL_UM_DIR_ADD,
// 			&CommInfo,
// 			NULL,
// 			0,
// 			NULL
// 			);
		
		// 拒绝访问c:\\2
// 		ZeroMemory(&CommInfo, sizeof(CommInfo));
// 		CopyMemory(CommInfo.Dir.wchFileName, L"c:\\2", wcslen(L"c:\\2") * sizeof(WCHAR));
// 		CommInfo.Dir.DirControlType = DIR_CONTROL_TYPE_ACCESS;
// 
// 		Comm.SendMsg(
// 			IOCTL_UM_DIR_ADD,
// 			&CommInfo,
// 			NULL,
// 			0,
// 			NULL
// 			);

		// 清理保护目录
		Comm.SendMsg(
			IOCTL_UM_DIR_CLEAR,
			NULL,
			NULL,
			0,
			NULL
			);

		// 添加白名单进程
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CommInfo.Proc.ulPid = 556;

		Comm.SendMsg(
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 添加白名单进程
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CommInfo.Proc.ulPid = 208;

		Comm.SendMsg(
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 添加白名单进程
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CommInfo.Proc.ulPid = 1744;

		Comm.SendMsg(
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 添加白名单进程
		ZeroMemory(&CommInfo, sizeof(CommInfo));
		CommInfo.Proc.ulPid = 2516;

		Comm.SendMsg(
			IOCTL_UM_PROC_ADD,
			&CommInfo,
			NULL,
			0,
			NULL
			);

		// 获取所有白名单进程
		bResult = Comm.SendMsg(
			IOCTL_UM_PROC_GET,
			NULL,
			NULL,
			0,
			&ulCount
			);
		if (!bResult)
		{
			if (lpCommInfo)
				free(lpCommInfo);

			lpCommInfo = (LPCOMM_INFO)calloc(1, sizeof(COMM_INFO) * ulCount);

			bResult = Comm.SendMsg(
				IOCTL_UM_PROC_GET,
				NULL,
				lpCommInfo,
				ulCount,
				&ulCount
				);
		}

		if (bResult)
		{
			for (i = 0 ; i < ulCount; i++)
			{
				lpCommInfoTemp = lpCommInfo + i;

				printf("%d \n", lpCommInfoTemp->Proc.ulPid);
			}
		}

		// 通知驱动停止工作
// 		bResult = Comm.SendMsg(
// 			IOCTL_UM_STOP,
// 			NULL,
// 			NULL,
// 			0,
// 			NULL
// 			);
	}
	__finally
	{
		if (lpCommInfo)
		{
			free(lpCommInfo);
			lpCommInfo = NULL;
		}
	}

	_getch();

	return 0;
}
