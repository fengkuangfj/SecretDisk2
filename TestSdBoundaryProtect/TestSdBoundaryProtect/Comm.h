#pragma once

#define MOD_COMM L"ͨѶ"

typedef struct _COMM_CONTEXT
{
	HANDLE	hServerPort;
	HANDLE	hCompletionPort;
} COMM_CONTEXT, *PCOMM_CONTEXT, *LPCOMM_CONTEXT;

class CComm
{
public:
	BOOL
		Init();

	BOOL
		SendMsg(
		__in		ULONG	ulType,
		__in		LPVOID	lpInBuffer,
		__in		ULONG	ulInBufferSizeB,
		__in_opt	LPVOID	lpOutBuffer,
		__in_opt	ULONG	ulOutBufferSizeB
		);

	VOID
		Stop();

private:
 	static COMM_CONTEXT ms_CommContext;
};
