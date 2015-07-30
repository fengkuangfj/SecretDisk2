#pragma once

#define MOD_COMM L"通讯"

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
		__in		ULONG		ulType,
		__in_opt	LPCOMM_INFO	lpInCommInfo,
		__out_opt	LPCOMM_INFO	lpOutCommInfo,
		__in_opt	ULONG		ulOutCommInfoCount,
		__out_opt	PULONG		pulRetCount
		);

	VOID
		Stop();

private:
 	static COMM_CONTEXT ms_CommContext;
};
