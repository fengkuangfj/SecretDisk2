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

// 		if (!DriveControl.Start(L"SdBoundaryProtect"))
// 		{
// 			printf("Start failed \n");
// 			__leave;
// 		}
// 		else
// 			printf("Start succeed \n");
// 
// 		if (!DriveControl.Stop(L"SdBoundaryProtect"))
// 		{
// 			printf("Stop failed \n");
// 			__leave;
// 		}
// 		else
// 			printf("Stop succeed \n");
// 
// 		if (!DriveControl.Delete(L"SdBoundaryProtect"))
// 		{
// 			printf("Delete failed \n");
// 			__leave;
// 		}
// 		else
// 			printf("Delete succeed \n");

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
	TestDriveControl();

	_getch();

	return 0;
}
