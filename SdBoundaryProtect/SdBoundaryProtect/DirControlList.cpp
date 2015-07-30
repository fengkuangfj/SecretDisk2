/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		DirControlList.cpp
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
#include "DirControlList.h"

LIST_ENTRY	CDirControlList::ms_ListHead	= {0};
ERESOURCE	CDirControlList::ms_Lock		= {0};
KSPIN_LOCK	CDirControlList::ms_SpLock		= 0;
ULONG		CDirControlList::ms_ulCount		= 0;

CDirControlList::CDirControlList()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

CDirControlList::~CDirControlList()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

VOID
	CDirControlList::Init()
{
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	InitializeListHead(&ms_ListHead);
	KeInitializeSpinLock(&ms_SpLock);	
	ExInitializeResourceLite(&ms_Lock);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return;
}

BOOLEAN
	CDirControlList::Unload()
{
	BOOLEAN				bRet				= FALSE;

	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;
	PLIST_ENTRY			pNode				= NULL;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		while (!IsListEmpty(&ms_ListHead))
		{
			lpDirProtectInfo = CONTAINING_RECORD(ms_ListHead.Flink, DIR_CONTROL_LIST, List);
			if (!lpDirProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Rule(%wZ) Type(0x%08x)",
				lpDirProtectInfo->RuleEx.Get(), lpDirProtectInfo->Type);

			RemoveEntryList(&lpDirProtectInfo->List);
			delete lpDirProtectInfo;
			lpDirProtectInfo = NULL;

			ms_ulCount--;
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
	CDirControlList::Clear()
{
	BOOLEAN				bRet				= FALSE;

	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;
	PLIST_ENTRY			pNode				= NULL;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		while (!IsListEmpty(&ms_ListHead))
		{
			lpDirProtectInfo = CONTAINING_RECORD(ms_ListHead.Flink, DIR_CONTROL_LIST, List);
			if (!lpDirProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Rule(%wZ) Type(0x%08x)",
				lpDirProtectInfo->RuleEx.Get(), lpDirProtectInfo->Type);

			RemoveEntryList(&lpDirProtectInfo->List);
			delete lpDirProtectInfo;
			lpDirProtectInfo = NULL;

			ms_ulCount--;
		}

		bRet = TRUE;
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
	CDirControlList::GetLock()
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
	CDirControlList::FreeLock()
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
	CDirControlList::Insert(
	__in LPREGISTER_DIR_INFO lpRegisterDirInfo
	)
{
	BOOLEAN				bRet				= FALSE;

	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;

	CKrnlStr			PathDev;
	CKrnlStr			RuleEx;
	CKrnlStr			ParentDirRuleEx;

	CFileName			FileName;


	__try
	{
		GetLock();

		if (!lpRegisterDirInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (!RuleEx.Set(&lpRegisterDirInfo->FileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx.Set failed. Dir(%wZ)",
				lpRegisterDirInfo->FileName.Get());

			__leave;
		}

		if (L'\\' == *(RuleEx.GetString() + RuleEx.GetLenCh() - 1))
		{
			if (!RuleEx.Append(L"*", wcslen(L"*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx.Append failed. Rule(%wZ)",
					RuleEx.Get());

				__leave;
			}
		}
		else if (L'*' != *(RuleEx.GetString() + RuleEx.GetLenCh() - 1))
		{
			if (!RuleEx.Append(L"\\*", wcslen(L"\\*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx.Append failed. Dir(%wZ)",
					RuleEx.Get());

				__leave;
			}
		}
		else
		{
			if (L'\\' != *(RuleEx.GetString() + RuleEx.GetLenCh() - 2))
			{
				*(RuleEx.GetString() + RuleEx.GetLenCh() - 1) = L'\\';

				if (!RuleEx.Append(L"*", wcslen(L"*")))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx.Append failed. Rule(%wZ)",
						RuleEx.Get());

					__leave;
				}
			}
		}

		if (!RuleEx.ToUpper())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx.ToUpper failed. Rule(%wZ)",
				RuleEx.Get());

			__leave;
		}

		lpDirProtectInfo = Get(&RuleEx);
		if (lpDirProtectInfo)
		{
			lpDirProtectInfo->Type = lpRegisterDirInfo->Type;

			KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"[OLD] Rule(%wZ) Type(0x%08x)",
				lpDirProtectInfo->RuleEx.Get(), lpDirProtectInfo->Type);

			bRet = TRUE;
			__leave;
		}

		lpDirProtectInfo = new(MEMORY_TAG_DIR_CONTROL_LIST) DIR_CONTROL_LIST;

		lpDirProtectInfo->Type = lpRegisterDirInfo->Type;

		if (!lpDirProtectInfo->RuleEx.Set(&RuleEx))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"lpDirProtectInfo->Rule.Set failed. Rule(%wZ)",
				RuleEx.Get());

			__leave;
		}

		if (!RuleEx.Shorten(RuleEx.GetLenCh() - 2))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx.Shorten failed. Rule(%wZ)",
				RuleEx.Get());

			__leave;
		}

		if (!CFileName::GetParentPath(&RuleEx, &ParentDirRuleEx))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CFileName::GetParentPath failed. Dir(%wZ)",
				RuleEx.Get());

			__leave;
		}

		if (!ParentDirRuleEx.Append(L"\\*", wcslen(L"\\*")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ParentDirRuleEx.Append failed. ParentDir(%wZ)",
				ParentDirRuleEx.Get());

			__leave;
		}

		if (!lpDirProtectInfo->ParentDirRuleEx.Set(&ParentDirRuleEx))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"lpDirProtectInfo->ParentDirRule.Set failed. ParentDirRule(%wZ)",
				ParentDirRuleEx.Get());

			__leave;
		}

		InsertTailList(&ms_ListHead, &lpDirProtectInfo->List);

		ms_ulCount++;

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"[NEW] Rule(%wZ) Type(0x%08x)",
			lpDirProtectInfo->RuleEx.Get(), lpDirProtectInfo->Type);

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
		{
			delete lpDirProtectInfo;
			lpDirProtectInfo = NULL;
		}

		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CDirControlList::Delete(
	__in CKrnlStr* pRule
	)
{
	BOOLEAN				bRet				= FALSE;

	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;


	__try
	{
		GetLock();

		if (!pRule)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (L'\\' == *(pRule->GetString() + pRule->GetLenCh() - 1))
		{
			if (!pRule->Append(L"*", wcslen(L"*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pRuleEx->Append failed. Rule(%wZ)",
					pRule->Get());

				__leave;
			}
		}
		else if (L'*' != *(pRule->GetString() + pRule->GetLenCh() - 1))
		{
			if (!pRule->Append(L"\\*", wcslen(L"\\*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pRuleEx->Append failed. Dir(%wZ)",
					pRule->Get());

				__leave;
			}
		}
		else
		{
			if (L'\\' != *(pRule->GetString() + pRule->GetLenCh() - 2))
			{
				*(pRule->GetString() + pRule->GetLenCh() - 1) = L'\\';

				if (!pRule->Append(L"*", wcslen(L"*")))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pRuleEx->Append failed. Rule(%wZ)",
						pRule->Get());

					__leave;
				}
			}
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. Rule(%wZ)",
				pRule->Get());

			__leave;
		}

		lpDirProtectInfo = Get(pRule);
		if (!lpDirProtectInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Get failed. Rule(%wZ)",
				pRule->Get());

			__leave;
		}

		KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"Rule(%wZ) Type(0x%08x)",
			lpDirProtectInfo->RuleEx.Get(), lpDirProtectInfo->Type);

		RemoveEntryList(&lpDirProtectInfo->List);
		delete lpDirProtectInfo;
		lpDirProtectInfo = NULL;

		ms_ulCount--;

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

LPDIR_CONTROL_LIST
	CDirControlList::Get(
	__in CKrnlStr* pRule
	)
{
	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;

	PLIST_ENTRY			pNode				= NULL;


	__try
	{
		GetLock();

		if(!pRule)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (L'\\' == *(pRule->GetString() + pRule->GetLenCh() - 1))
		{
			if (!pRule->Append(L"*", wcslen(L"*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx->Append failed. Rule(%wZ)",
					pRule->Get());

				__leave;
			}
		}
		else if (L'*' != *(pRule->GetString() + pRule->GetLenCh() - 1))
		{
			if (!pRule->Append(L"\\*", wcslen(L"\\*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx->Append failed. Dir(%wZ)",
					pRule->Get());

				__leave;
			}
		}
		else
		{
			if (L'\\' != *(pRule->GetString() + pRule->GetLenCh() - 2))
			{
				*(pRule->GetString() + pRule->GetLenCh() - 1) = L'\\';

				if (!pRule->Append(L"*", wcslen(L"*")))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RuleEx->Append failed. Rule(%wZ)",
						pRule->Get());

					__leave;
				}
			}
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"list empty. Rule(%wZ)",
				pRule->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpDirProtectInfo = NULL)
		{
			lpDirProtectInfo = CONTAINING_RECORD(pNode, DIR_CONTROL_LIST, List);
			if (!lpDirProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. Rule(%wZ)",
					pRule->Get());

				__leave;
			}

			if (lpDirProtectInfo->RuleEx.Equal(pRule, TRUE))
				break;
		}
	}
	__finally
	{
		FreeLock();
	}

	return lpDirProtectInfo;
}

BOOLEAN
	CDirControlList::Filter(
	__in	CKrnlStr			*	pFileName,
	__inout PFLT_CALLBACK_DATA		pData
	)
{
	BOOLEAN				bRet				= FALSE; 

	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;
	PLIST_ENTRY			pNode				= NULL;

	CKrnlStr			FileName1;
	CKrnlStr			FileName2;


	__try
	{
		GetLock();

		if (!pFileName || !pData)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pFileName(%p) pData(%p)",
				pFileName, pData);

			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpDirProtectInfo = NULL)
		{
			lpDirProtectInfo = CONTAINING_RECORD(pNode, DIR_CONTROL_LIST, List);
			if (!lpDirProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. File(%wZ)",
					pFileName->Get());

				__leave;
			}

			if (!FileName1.Set(&lpDirProtectInfo->ParentDirRuleEx))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName1.Set failed. File(%wZ)",
					lpDirProtectInfo->ParentDirRuleEx.Get());

				__leave;
			}

			if (!FileName1.Shorten(lpDirProtectInfo->ParentDirRuleEx.GetLenCh() - 2))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName1.Shorten failed. File(%wZ)",
					FileName1.Get());

				__leave;
			}

			if (!FileName2.Set(pFileName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName2.Set failed. File(%wZ)",
					pFileName->Get());

				__leave;
			}

			if (L'*' == *(FileName2.GetString() + FileName2.GetLenCh() - 1))
			{
				if (!FileName2.Shorten(FileName2.GetLenCh() - 1))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName2.Shorten failed. File(%wZ)",
						FileName2.Get());

					__leave;
				}
			}

			if (L'\\' == *(FileName2.GetString() + FileName2.GetLenCh() - 1))
			{
				if (!FileName2.Shorten(FileName2.GetLenCh() - 1))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName2.Shorten failed. File(%wZ)",
						FileName2.Get());

					__leave;
				}
			}

			if (FileName2.GetLenCh() < FileName1.GetLenCh())
				continue;

			if (FileName2.GetLenCh() == FileName1.GetLenCh())
			{
				if (!FileName1.Equal(&FileName2, TRUE))
					continue;
			}
			else if (FileName2.GetLenCh() > FileName1.GetLenCh())
			{
				if (!FileName2.Append(L"\\", 1))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName2.Append failed. File(%wZ)",
						FileName2.Get());
					__leave;
				}

				if (!FsRtlIsNameInExpression(
					lpDirProtectInfo->RuleEx.Get(),
					FileName2.Get(),
					TRUE,
					NULL
					))
					continue;
			}

			FreeLock();
			if (!CDirHide::BreakLink(pData, &lpDirProtectInfo->RuleEx, pFileName))
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"BreakLink failed. File(%wZ)",
					pFileName->Get());

				__leave;
			}
			GetLock();
			break;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CDirControlList::IsIn(
	__in CKrnlStr			*	pFileName,
	__in DIR_CONTROL_TYPE		DirControlType
	)
{
	BOOLEAN				bRet				= FALSE;

	LPDIR_CONTROL_LIST	lpDirProtectInfo	= NULL;
	PLIST_ENTRY			pNode				= NULL;

	CKrnlStr			Name;


	__try
	{
		GetLock();

		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpDirProtectInfo = NULL)
		{
			lpDirProtectInfo = CONTAINING_RECORD(pNode, DIR_CONTROL_LIST, List);
			if (!lpDirProtectInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. File(%wZ)",
					pFileName->Get());

				__leave;
			}

			if (FsRtlIsNameInExpression(
				lpDirProtectInfo->RuleEx.Get(),
				pFileName->Get(),
				TRUE,
				NULL
				))
			{
				if (FlagOn(lpDirProtectInfo->Type, DirControlType))
					bRet = TRUE;
				
				break;
			}
			else
			{
				if (pFileName->GetLenCh() + 2 == lpDirProtectInfo->RuleEx.GetLenCh())
				{
					if (!Name.Set(pFileName))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Name.Set failed. File(%wZ)",
							pFileName->Get());

						__leave;
					}

					if (!Name.Append(L"\\*", wcslen(L"\\*")))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Name.Append failed. File(%wZ)",
							Name.Get());

						__leave;
					}

					if (Name.Equal(&lpDirProtectInfo->RuleEx, TRUE))
					{
						if (FlagOn(lpDirProtectInfo->Type, DirControlType))
							bRet = TRUE;

						break;
					}
				}
			}
		}
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

ULONG
	CDirControlList::GetCount()
{
	ULONG ulCount = 0;


	__try
	{
		GetLock();

		ulCount = ms_ulCount;
	}
	__finally
	{
		FreeLock();
	}

	return ulCount;
}

BOOLEAN
	CDirControlList::Fill(
	__in LPCOMM_INFO	lpCommInfo,
	__in ULONG			ulCount
	)
{
	BOOLEAN				bRet				= FALSE;

	LPDIR_CONTROL_LIST	lpDirControlList	= NULL;
	PLIST_ENTRY			pNode				= NULL;

	CKrnlStr			AppName;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		if (!lpCommInfo || !ulCount)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. lpCommInfo(%p) ulCount(%d)",
				lpCommInfo, ulCount);

			__leave;
		}

		if (ulCount < ms_ulCount)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ulCount error. ulCount(%d) ms_ulCount(%d)",
				ulCount, ms_ulCount);

			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			bRet = TRUE;
			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpDirControlList = NULL)
		{
			lpDirControlList = CONTAINING_RECORD(pNode, DIR_CONTROL_LIST, List);
			if (!lpDirControlList)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			lpCommInfo->Dir.DirControlType = lpDirControlList->Type;

			if (!CFileName::ToApp(&lpDirControlList->RuleEx, &AppName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CFileName::ToApp failed. Rule(%wZ)",
					lpDirControlList->RuleEx.Get());

				__leave;
			}

			if (!AppName.Shorten(AppName.GetLenCh() - 2))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"AppName.Shorten failed. Rule(%wZ)",
					AppName.Get());

				__leave;
			}

			RtlCopyMemory(lpCommInfo->Dir.wchFileName, AppName.GetString(), AppName.GetLenB());

			lpCommInfo++;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return bRet;
}
