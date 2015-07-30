#include "stdafx.h"
#include "Comm.h"

COMM_CONTEXT CComm::ms_CommInfo = {0};

CComm::CComm()
{
	;
}

CComm::~CComm()
{
	;
}

/*
* 函数说明：
*		初始化通讯
*
* 参数：
*		pFlt
*                           
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*
*/
BOOLEAN
	CComm::Init(
	__in PFLT_FILTER pFlt
	)
{
	BOOLEAN					bRet		= FALSE;

	CKrnlStr				PortName;

	NTSTATUS				ntStatus	= STATUS_UNSUCCESSFUL;
	PSECURITY_DESCRIPTOR	pSd			= NULL;
	OBJECT_ATTRIBUTES		Ob			= {0};

	CLog					Log;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		if (!pFlt)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER) && ms_CommInfo.pSeverPort)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Already CCommKm::Init");
			__leave;
		}

		if (!PortName.Set(g_lpCommPortName, wcslen(g_lpCommPortName)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"PortName.Set failed");
			__leave;
		}

		ntStatus = FltBuildDefaultSecurityDescriptor(&pSd, FLT_PORT_ALL_ACCESS);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltBuildDefaultSecurityDescriptor failed (%x)", ntStatus);
			__leave;
		}

		InitializeObjectAttributes(
			&Ob,
			PortName.Get(),
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
			NULL,
			pSd
			);

		ntStatus = FltCreateCommunicationPort(
			pFlt,
			&ms_CommInfo.pSeverPort,
			&Ob,
			NULL,
			(PFLT_CONNECT_NOTIFY)CommKmConnectNotify,
			(PFLT_DISCONNECT_NOTIFY)CommKmDisconnectNotify,
			(PFLT_MESSAGE_NOTIFY)CommKmMessageNotify,
			1
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateCommunicationPort failed (%x)", ntStatus);				
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pSd)
		{
			FltFreeSecurityDescriptor(pSd);
			pSd = NULL;
		}

		if (!bRet)
		{
			if (ms_CommInfo.pSeverPort)
			{
				FltCloseCommunicationPort(ms_CommInfo.pSeverPort);
				ms_CommInfo.pSeverPort = NULL;
			}
		}
	}
	
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

/*
* 函数说明：
*		卸载通讯模块
*
* 参数：
*		无
*
* 返回值：
*		无
*
* 备注：
*
*/
VOID
	CComm::Unload()
{
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	if (!ms_CommInfo.pSeverPort)
		KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Not CCommKm::Init or Already CCommKm::Unload");
	else
	{
		FltCloseCommunicationPort(ms_CommInfo.pSeverPort);
		ms_CommInfo.pSeverPort = NULL;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");
}

/*
* 函数说明：
*		通信连接回调函数
*
* 参数：
*		ClientPort
*		ServerPortCookie
*		ConnectionContext
*		SizeOfContext
*		ConnectionPortCookie		Pointer to information that uniquely identifies this client port. 
*
* 返回值：
*		Status
*
* 备注：
*
*/
NTSTATUS
	CComm::CommKmConnectNotify(
	__in								PFLT_PORT		pClientPort,
	__in_opt							LPVOID			lpServerPortCookie,
	__in_bcount_opt(ulSizeOfContext)	LPVOID			lpConnectionContext,
	__in								ULONG			ulSizeOfContext,
	__deref_out_opt						LPVOID		*	pConnectionPortCookie
	)
{
	NTSTATUS		Status		= STATUS_UNSUCCESSFUL;

	ULONG			ulPid		= 0;

	CKrnlStr		ProcPath;
	CKrnlStr		LogDir;

	CFileName		FileName;
	CLog			Log;
	CProcWhiteList	ProcWhiteList;


	UNREFERENCED_PARAMETER(lpServerPortCookie);
	UNREFERENCED_PARAMETER(ulSizeOfContext);
	UNREFERENCED_PARAMETER(pConnectionPortCookie);


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		ulPid = (ULONG)PsGetCurrentProcessId();

		if (!RtlEqualMemory(lpConnectionContext, g_lpConnectionCtx, wcslen(g_lpConnectionCtx) * sizeof(WCHAR)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RtlEqualMemory failed");
			__leave;
		}

		ms_CommInfo.pClientPort = pClientPort;

		if (!ProcWhiteList.GetProcPath(ulPid, &ProcPath, TRUE))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ProcWhiteList.GetProcessPath failed. Pid(%d)",
				ulPid);

			__leave;
		}

		if (!FileName.GetParentPath(&ProcPath, &LogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.GetParentPath failed. Pid(%d) Path(%wZ)",
				ulPid, ProcPath.Get());

			__leave;
		}

		if (!LogDir.Append(L"\\log", wcslen(L"\\log")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"LogDir.Append failed. Pid(%d) Dir(%wZ)",
				ulPid, LogDir.Get());

			__leave;
		}

		if (!Log.SetLogDir(&LogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Log.SetLogDir failed. Pid(%d) Dir(%wZ)",
				ulPid, LogDir.Get());

			__leave;
		}

		Status = STATUS_SUCCESS;
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return Status;
}

/*
* 函数说明：
*		通信断开回调函数
*
* 参数：
*		ConnectionCookie
*
* 返回值：
*		无
*
* 备注：
*
*/
VOID 
	CComm::CommKmDisconnectNotify(
	__in_opt LPVOID lpConnectionCookie
	)
{
	UNREFERENCED_PARAMETER(lpConnectionCookie);


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		if (!CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"!ms_CommInfo.pFlt");
			__leave;
		}

		if (!ms_CommInfo.pClientPort)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"!ms_CommInfo.ulClientId || !ms_CommInfo.pClientPort");
			__leave;
		}

		FltCloseClientPort(CMinifilter::ms_pMfIns->m_pFltFilter, &ms_CommInfo.pClientPort);
		ms_CommInfo.pClientPort = NULL;
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return;
}

/*
* 函数说明：
*		R0处理R3发来请求回调函数
*
* 参数：
*		PortCookie
*		InputBuffer
*		InputBufferLength
*		OutputBuffer
*		OutputBufferLength
*		ReturnOutputBufferLength
*		
* 返回值：
*		Status
*
* 备注：
*
*/
NTSTATUS
	CComm::CommKmMessageNotify(
	__in_opt																	LPVOID	lpPortCookie,
	__in_bcount_opt(ulInputBufferLength)										LPVOID	lpInputBuffer,
	__in																		ULONG	ulInputBufferLength,
	__out_bcount_part_opt(ulOutputBufferLength, * pulReturnOutputBufferLength)	LPVOID	lpOutputBuffer,
	__in																		ULONG	ulOutputBufferLength,
	__out																		PULONG	pulReturnOutputBufferLength
	)
{
	NTSTATUS			ntStatus			= STATUS_UNSUCCESSFUL;

	PREQUEST_PACKET		pRequestPacket		= NULL;

	REGISTER_DIR_INFO	RegisterDirInfo;

	CKrnlStr			DirAppName;
	CKrnlStr			DirDevName;

	CProcWhiteList		ProcWhiteList;
	CDirControlList		DirControlList;


	__try
	{
		RtlZeroMemory(&RegisterDirInfo, sizeof(RegisterDirInfo));

		if (!CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER) || !ms_CommInfo.pSeverPort || !ms_CommInfo.pClientPort)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"comm argument error");
			__leave;
		}

		*pulReturnOutputBufferLength = ulOutputBufferLength;

		// 转换结构
		pRequestPacket = (PREQUEST_PACKET)lpInputBuffer;
		if (!pRequestPacket)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"lpInputBuffer error");
			__leave;
		}

		switch (pRequestPacket->ulType)
		{
		case IOCTL_UM_START:
			{
				CMinifilter::ms_pMfIns->AllowFltWork();
				break;
			}
		case IOCTL_UM_STOP:
			{
				CMinifilter::ms_pMfIns->DisallowFltWork();
				break;
			}
		case IOCTL_UM_DIR_ADD:
			{
				RegisterDirInfo.Type = ((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.DirControlType;

				if (!DirAppName.Set(
					((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.wchFileName,
					wcslen(((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.wchFileName)))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_ADD] DirAppName.Set failed. Dir(%lS)",
						((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.wchFileName);

					__leave;
				}

				if (!CFileName::ToDev(&DirAppName, &RegisterDirInfo.FileName))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_ADD] CFileName::ToDev failed. Dir(%wZ)",
						DirAppName.Get());

					__leave;
				}

				if (!DirControlList.Insert(&RegisterDirInfo))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_ADD] DirControlList.Insert failed. Dir(%wZ)",
						RegisterDirInfo.FileName.Get());

					__leave;
				}

				break;
			}
		case IOCTL_UM_DIR_DELETE:
			{
				if (!DirAppName.Set(
					((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.wchFileName,
					wcslen(((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.wchFileName)))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_DELETE] DirAppName.Set failed. Dir(%lS)",
						((LPCOMM_INFO)(pRequestPacket->chContent))->Dir.wchFileName);

					__leave;
				}

				if (!CFileName::ToDev(&DirAppName, &DirDevName))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_DELETE] CFileName::ToDev failed. Dir(%wZ)",
						DirAppName.Get());

					__leave;
				}

				if (!DirControlList.Delete(&DirDevName))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_DELETE] DirControlList.Delete failed. Dir(%wZ)",
						DirDevName.Get());

					__leave;
				}

				break;
			}
		case IOCTL_UM_DIR_CLEAR:
			{
				if (!DirControlList.Clear())
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_DIR_CLEAR] DirControlList.Clear failed");
					__leave;
				}

				break;
			}
		case IOCTL_UM_PROC_ADD:
			{
				if (!ProcWhiteList.Insert(((LPCOMM_INFO)(pRequestPacket->chContent))->Proc.ulPid))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_PROC_ADD] ProcWhiteList.Insert failed. Pid(%d)",
						((LPCOMM_INFO)(pRequestPacket->chContent))->Proc.ulPid);

					__leave;
				}

				break;
			}
		case IOCTL_UM_PROC_DELETE:
			{
				if (!ProcWhiteList.Delete(((LPCOMM_INFO)(pRequestPacket->chContent))->Proc.ulPid))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_PROC_DELETE] ProcWhiteList.Delete failed. Pid(%d)",
						((LPCOMM_INFO)(pRequestPacket->chContent))->Proc.ulPid);

					__leave;
				}

				break;
			}
		case IOCTL_UM_PROC_CLEAR:
			{
				if (!ProcWhiteList.Clear())
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IOCTL_UM_PROC_CLEAR] ProcWhiteList.Clear failed");

					__leave;
				}

				break;
			}
		default:
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IoControlCode error. IoControlCode(%x)",
					pRequestPacket->ulType);

				__leave;
			}
		}
		

		ntStatus = STATUS_SUCCESS;
	}
	__finally
	{
		;
	}

	return ntStatus;
}
