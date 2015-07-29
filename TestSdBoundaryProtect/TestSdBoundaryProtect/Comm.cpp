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
	__in		ULONG	ulType,
	__in		LPVOID	lpInBuffer,
	__in		ULONG	ulInBufferSizeB,
	__in_opt	LPVOID	lpOutBuffer,
	__in_opt	ULONG	ulOutBufferSizeB
	)
{
	BOOL			bRet			= FALSE;

	HRESULT			hResult			= S_FALSE;
 	REQUEST_PACKET	RequstPacket	= {0};
	REPLY_PACKET	ReplyPacket		= {0};
 	DWORD			dwRet			= 0;
 	

	__try
	{
		if (!ulType || !lpInBuffer || !ulInBufferSizeB || (lpOutBuffer && !ulOutBufferSizeB))
		{
			printf("input arguments error. \n");
			__leave;
		}

		RequstPacket.ulVersion = REQUEST_PACKET_VERSION;
		RequstPacket.ulType = ulType;

		if (REQUEST_CONTENT_LENGTH < ulInBufferSizeB)
		{
			printf("ulInBufferSizeB too large. \n");
			__leave;
		}

		CopyMemory(RequstPacket.chContent, (LPSTR)lpInBuffer, ulInBufferSizeB);

		ReplyPacket.ulVersion = REPLY_PACKET_VERSION;
	
		hResult = FilterSendMessage(
			ms_CommContext.hServerPort, 
			&RequstPacket,
			REQUEST_PACKET_SIZE,
			&ReplyPacket,
			REPLY_PACKET_SIZE,
			&dwRet
			);
		if (S_OK != hResult)
		{
			printf("FilterSendMessage failed. (0x%08x) \n", hResult);
			__leave;
		}

		if (lpOutBuffer)
		{
			if (ulOutBufferSizeB < ReplyPacket.ulValidContentSizeB)
			{
				printf("ulValidContentSizeB too large. \n");
				__leave;
			}

			CopyMemory(lpOutBuffer, ReplyPacket.chContent, ReplyPacket.ulValidContentSizeB);
		}

		bRet = TRUE;
	}
	__finally
	{
		;
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
			printf("CreateIoCompletionPort failed. (%d) \n", GetLastError);
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
