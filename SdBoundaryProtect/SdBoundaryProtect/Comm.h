#pragma once

#define MOD_COMM		L"通讯"
#define MEMORY_TAG_COMM 'MMOC'	// COMM

typedef struct _COMM_CONTEXT
{
	PFLT_PORT pSeverPort;
	PFLT_PORT pClientPort;
} COMM_CONTEXT, *PCOMM_CONTEXT, *LPCOMM_CONTEXT;

class CComm
{
public:
	CComm();

	~CComm();

	static COMM_CONTEXT ms_CommInfo;

	BOOLEAN
		Init(
		__in PFLT_FILTER pFlt
		);

	VOID
		Unload();

private:
	static
		NTSTATUS  
		CommKmConnectNotify(
		__in								PFLT_PORT		pClientPort,
		__in_opt							LPVOID			lpServerPortCookie,
		__in_bcount_opt(ulSizeOfContext)	LPVOID			lpConnectionContext,
		__in								ULONG			ulSizeOfContext,
		__deref_out_opt						LPVOID		*	pConnectionPortCookie
		);

	static
		VOID
		CommKmDisconnectNotify(
		__in_opt LPVOID lpConnectionCookie
		);

	static
		NTSTATUS
		CommKmMessageNotify(
		__in_opt																	LPVOID	lpPortCookie,
		__in_bcount_opt(ulInputBufferLength)										LPVOID	lpInputBuffer,
		__in																		ULONG	ulInputBufferLength,
		__out_bcount_part_opt(ulOutputBufferLength, * pulReturnOutputBufferLength)	LPVOID	lpOutputBuffer,
		__in																		ULONG	ulOutputBufferLength,
		__out																		PULONG	pulReturnOutputBufferLength
		);
};
