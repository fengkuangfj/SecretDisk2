#include "stdafx.h"
#include "Comm.h"

COMM_CONTEXT CComm::ms_CommContext = {INVALID_HANDLE_VALUE, NULL};

VOID
	CComm::Stop()
{
	if (ms_CommContext.hCompletionPort)
	{
		CloseHandle(ms_CommContext.hCompletionPort);
		ms_CommContext.hCompletionPort = NULL;
	}

	if (INVALID_HANDLE_VALUE != ms_CommContext.hServerPort)
	{
		CloseHandle(ms_CommContext.hServerPort);
		ms_CommContext.hServerPort = INVALID_HANDLE_VALUE;
	}

	return;
}

/*
*
*函数简介：应用层向内核层发送控制命令
*
*参数介绍：ucCmdType -------控制命令具体类型，输入参数，不为空
*
*          pCmdContent------存放控制命令缓存区首地址，输入参数，不为空
*
*		   usCmdLen---------控制命令长度，以字节为单位，输入参数，不为空
*
*          pCmdResult-------返回命令包被处理结果，输出参数，可为空
*                           
*返回值：如果控制命令发送成功，返回TRUE，否则返回FALSE
*
*/
BOOL
	CComm::SendMsg(
	__in		ULONG		ulType,
	__in_opt	LPCOMM_INFO	lpInCommInfo,
	__out_opt	LPCOMM_INFO	lpOutCommInfo,
	__in_opt	ULONG		ulOutCommInfoCount,
	__out_opt	PULONG		pulRetCount
	)
{
	BOOL			bRet				= FALSE;

	HRESULT			hResult				= S_FALSE;
	REQUEST_PACKET	RequstPacket		= {0};
	LPREPLY_PACKET	lpReplyPacket		= NULL;
	DWORD			dwRet				= 0;
	ULONG			ulRequestPacketSize = 0;
	ULONG			ulReplyPacketSize	= 0;
	ULONG			i					= 0;
 	

	__try
	{
		if (!ulType)
		{
			printf("input argument error. \n");
			__leave;
		}

		ulRequestPacketSize = sizeof(REQUEST_PACKET);
		RequstPacket.ulVersion = REQUEST_PACKET_VERSION;
		RequstPacket.ulType = ulType;

		switch (RequstPacket.ulType)
		{
		case IOCTL_UM_START:
		case IOCTL_UM_STOP:
		case IOCTL_UM_DIR_CLEAR:
		case IOCTL_UM_PROC_CLEAR:
			break;
		case IOCTL_UM_DIR_ADD:
		case IOCTL_UM_DIR_DELETE:
		case IOCTL_UM_PROC_ADD:
		case IOCTL_UM_PROC_DELETE:
			{
				if (!lpInCommInfo)
				{
					printf("lpInCommInfo error. \n");
					__leave;
				}

				CopyMemory(&RequstPacket.CommInfo, lpInCommInfo, sizeof(COMM_INFO));

				break;
			}
		case IOCTL_UM_DIR_GET:
		case IOCTL_UM_PROC_GET:
			{
				if (!pulRetCount)
				{
					printf("pulRetCount error. \n");
					__leave;
				}

				if (ulOutCommInfoCount)
				{
					ulReplyPacketSize = sizeof(REPLY_PACKET) + sizeof(COMM_INFO) * (ulOutCommInfoCount - 1);

					lpReplyPacket = (LPREPLY_PACKET)calloc(1, ulReplyPacketSize);
					if (!lpReplyPacket)
					{
						printf("calloc failed. (%d) \n", GetLastError());
						__leave;
					}

					lpReplyPacket->ulVersion = REPLY_PACKET_VERSION;
					lpReplyPacket->ulCount = ulOutCommInfoCount;
				}
	
				break;
			}
		default:
			{
				printf("RequstPacket.ulType error. (0x%08x) \n", RequstPacket.ulType);
				__leave;
			}
		}
	
		hResult = FilterSendMessage(
			ms_CommContext.hServerPort,
			&RequstPacket,
			ulRequestPacketSize,
			lpReplyPacket,
			ulReplyPacketSize,
			&dwRet
			);
		if (S_OK != hResult)
		{
			printf("FilterSendMessage failed. (0x%08x) \n", hResult);
			__leave;
		}

		switch (RequstPacket.ulType)
		{
		case IOCTL_UM_START:
		case IOCTL_UM_STOP:
		case IOCTL_UM_DIR_ADD:
		case IOCTL_UM_DIR_DELETE:
		case IOCTL_UM_DIR_CLEAR:
		case IOCTL_UM_PROC_ADD:
		case IOCTL_UM_PROC_DELETE:
		case IOCTL_UM_PROC_CLEAR:
			break;
		case IOCTL_UM_DIR_GET:
		case IOCTL_UM_PROC_GET:
			{
				if (!pulRetCount)
				{
					printf("pulRetCount error. \n");
					__leave;
				}

				*pulRetCount = dwRet;
				if (!lpOutCommInfo || (ulOutCommInfoCount < *pulRetCount))
					__leave;

				for (; i < ulOutCommInfoCount; i++)
					CopyMemory(lpOutCommInfo + i, lpReplyPacket->CommInfo + i, sizeof(COMM_INFO));

				break;
			}			
		default:
			{
				printf("RequstPacket.ulType error. (0x%08x) \n", RequstPacket.ulType);
				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		if (lpReplyPacket)
		{
			free(lpReplyPacket);
			lpReplyPacket = NULL;
		}
	}

	return bRet;
}

/*
*
*函数简介：应用层初始化通信
*
*参数介绍：无
*                           
*返回值：如果初始化成功，返回TRUE，否则返回FALSE
*
*/
BOOL
	CComm::Init()
{
	BOOL	bRet	= FALSE;

	HRESULT	hResult	= S_FALSE;


	__try
	{
		hResult = FilterConnectCommunicationPort(
			g_lpCommPortName,
			0,
			g_lpConnectionCtx,
			wcslen(g_lpConnectionCtx) * sizeof(WCHAR),
			NULL,
			&ms_CommContext.hServerPort
			);
		if (S_OK != hResult)
		{
			printf("FilterConnectCommunicationPort failed. (0x%08x) \n", hResult);
			__leave;
		}

		if (INVALID_HANDLE_VALUE == ms_CommContext.hServerPort)
		{
			printf("m_Context.ServerPort error. \n");
			__leave;
		}

		ms_CommContext.hCompletionPort = CreateIoCompletionPort(
			ms_CommContext.hServerPort,
			NULL,
			0,
			0
			);
		if (!ms_CommContext.hCompletionPort)
		{
			printf("CreateIoCompletionPort failed. (%d) \n", GetLastError());
			__leave;
		}
		
		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
		{
			if (ms_CommContext.hCompletionPort)
			{
				CloseHandle(ms_CommContext.hCompletionPort);
				ms_CommContext.hCompletionPort = NULL;
			}

			if (INVALID_HANDLE_VALUE != ms_CommContext.hServerPort)
			{
				CloseHandle(ms_CommContext.hServerPort);
				ms_CommContext.hServerPort = INVALID_HANDLE_VALUE;
			}
		}
	}

	return bRet;
}
