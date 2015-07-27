/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		ProcWhiteList.cpp
*
* Abstract:
*
*		无
*
* Environment:
*
*		Kernel mode
*
* Version:
*
*		无
*
* Author:
*
*		岳翔
*
* Complete Time:
*
*		无
*
* Modify Record:
*
*		无
*
--*/

#include "stdafx.h"
#include "ProcWhiteList.h"

LIST_ENTRY	CProcWhiteList::ms_ListHead	= {0};
ERESOURCE	CProcWhiteList::ms_Lock		= {0};
KSPIN_LOCK	CProcWhiteList::ms_SpLock	= 0;

CProcWhiteList::CProcWhiteList()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

CProcWhiteList::~CProcWhiteList()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

BOOLEAN
	CProcWhiteList::Init()
{
	BOOLEAN bRet = FALSE;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		InitializeListHead(&ms_ListHead);
		ExInitializeResourceLite(&ms_Lock);
		KeInitializeSpinLock(&ms_SpLock);

		if (!Insert(0))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Insert failed. Pid(0)");
			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

BOOLEAN
	CProcWhiteList::Unload()
{
	BOOLEAN				bRet				= FALSE;

	LPPROC_WHITE_LIST	lpProcProtectInfo	= NULL;
	PLIST_ENTRY			pNode				= NULL;		


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		while (!IsListEmpty(&ms_ListHead))
		{
			lpProcProtectInfo = CONTAINING_RECORD(ms_ListHead.Flink, PROC_WHITE_LIST, List);
			if (!lpProcProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Pid(%d)",
				lpProcProtectInfo->ulPid);

			RemoveEntryList(&lpProcProtectInfo->List);
			delete lpProcProtectInfo;
			lpProcProtectInfo = NULL;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		ExDeleteResourceLite(&ms_Lock);
		RtlZeroMemory(&ms_Lock, sizeof(ms_Lock));
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

BOOLEAN
	CProcWhiteList::Clear()
{
	BOOLEAN				bRet				= FALSE;

	LPPROC_WHITE_LIST	lpProcProtectInfo	= NULL;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		while (!IsListEmpty(&ms_ListHead))
		{
			lpProcProtectInfo = CONTAINING_RECORD(ms_ListHead.Flink, PROC_WHITE_LIST, List);
			if (!lpProcProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Pid(%d)",
				lpProcProtectInfo->ulPid);

			RemoveEntryList(&lpProcProtectInfo->List);
			delete lpProcProtectInfo;
			lpProcProtectInfo = NULL;
		}
	}
	__finally
	{
		FreeLock();
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}

#pragma warning(push)
#pragma warning(disable: 28167)
VOID
	CProcWhiteList::GetLock()
{
	__try
	{
		// 判断当前锁引用计数是否合法
		if (m_LockRef < 0)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"m_LockRef < 0 (%d)", m_LockRef);
			__leave;
		}

		// 判断需不需要加锁
		if (m_LockRef++)
			__leave;

		// 加锁
		m_Irql = KeGetCurrentIrql();
		if (m_Irql == DISPATCH_LEVEL)
			KeAcquireSpinLock(&ms_SpLock, &m_Irql);
		else if (m_Irql <= APC_LEVEL)
		{
#pragma warning(push)
#pragma warning(disable: 28103)
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&ms_Lock, TRUE);
#pragma warning(pop)
		}
		else
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IRQL > DISPATCH_LEVEL");
	}
	__finally
	{
		;
	}

	return;
}
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable: 28167)
VOID
	CProcWhiteList::FreeLock()
{
	__try
	{
		// 判断当前锁引用计数是否合法
		if (m_LockRef <= 0)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"m_LockRef <= 0 (%d)", m_LockRef);
			__leave;
		}

		// 判断需不需要解锁
		if (--m_LockRef)
			__leave;

		// 解锁
		if (m_Irql == DISPATCH_LEVEL)
			KeReleaseSpinLock(&ms_SpLock, m_Irql);
		else if (m_Irql <= APC_LEVEL)
		{
#pragma warning(push)
#pragma warning(disable: 28107)
			ExReleaseResourceLite(&ms_Lock);
			KeLeaveCriticalRegion();
#pragma warning(pop)
		}
		else
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"IRQL > DISPATCH_LEVEL");
	}
	__finally
	{
		;
	}

	return;
}
#pragma warning(pop)

BOOLEAN
	CProcWhiteList::Insert(
	__in ULONG ulPid
	)
{
	BOOLEAN				bRet				= FALSE;

	LPPROC_WHITE_LIST	lpProcProtectInfo	= NULL;


	__try
	{
		GetLock();

		if (Get(ulPid))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Already exist. Need not insert");
			__leave;
		}

		lpProcProtectInfo = new(MEMORY_TAG_PROC_WHILTE_LIST) PROC_WHITE_LIST;
		lpProcProtectInfo->ulPid = ulPid;
		InsertTailList(&ms_ListHead, &lpProcProtectInfo->List);

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Pid(%d)",
			ulPid);

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CProcWhiteList::Delete(
	__in ULONG ulPid
	)
{
	BOOLEAN				bRet				= FALSE;

	LPPROC_WHITE_LIST	lpProcProtectInfo	= NULL;


	__try
	{
		GetLock();

		lpProcProtectInfo = Get(ulPid);
		if (!lpProcProtectInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"not exist. Need not del. ulPid(%d)",
				ulPid);

			__leave;
		}

		RemoveEntryList(&lpProcProtectInfo->List);
		delete lpProcProtectInfo;
		lpProcProtectInfo = NULL;

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Pid(%d)",
			ulPid);

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

LPPROC_WHITE_LIST
	CProcWhiteList::Get(
	__in ULONG ulPid
	)
{
	LPPROC_WHITE_LIST	lpProcProtectInfo	= NULL;

	PLIST_ENTRY			pNode				= NULL;


	__try
	{
		GetLock();

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. Pid(%d)",
				ulPid);

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpProcProtectInfo = NULL)
		{
			lpProcProtectInfo = CONTAINING_RECORD(pNode, PROC_WHITE_LIST, List);
			if (!lpProcProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			if (lpProcProtectInfo->ulPid == ulPid)
				__leave;
		}
	}
	__finally
	{
		FreeLock();
	}

	return lpProcProtectInfo;
}

BOOLEAN
	CProcWhiteList::IsIn(
	__in ULONG ulPid
	)
{
	if (Get(ulPid))
		return TRUE;
	else
		return FALSE;
}
