/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		FileName.cpp
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
#include "FileName.h"

LIST_ENTRY	CFileName::ms_ListHead	= {0};
ERESOURCE	CFileName::ms_Lock		= {0};
KSPIN_LOCK	CFileName::ms_SpLock	= 0;

CFileName::CFileName()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

CFileName::~CFileName()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

/*
* 函数说明：
*		初始化模块
*
* 参数：
*		无
*
* 返回值：
*		无
*
* 备注：
*		无
*/
VOID
	CFileName::Init()
{
	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	InitializeListHead(&ms_ListHead);
	ExInitializeResourceLite(&ms_Lock);
	KeInitializeSpinLock(&ms_SpLock);

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"end");

	return;
}

/*
* 函数说明：
*		卸载模块,清理卷实例时，会调用该模块，释放卷信息链表
*
* 参数：
*		无			
*
* 返回值：
*		无
*
* 备注：
*		加锁
*/
BOOLEAN 
	CFileName::Unload()
{
	BOOLEAN				bRet			= FALSE;

	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEED, L"begin");

	__try
	{
		GetLock();

		while (!IsListEmpty(&ms_ListHead))
		{
			lpVolNameInfo = CONTAINING_RECORD(ms_ListHead.Flink, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed");
				__leave;
			}

			RemoveEntryList(&lpVolNameInfo->List);
			delete lpVolNameInfo;
			lpVolNameInfo = NULL;
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

/*
* 函数说明：
*		获得锁
*
* 参数：
*		无
*
* 返回值：
*		无
*
* 备注：
*		无
*/
#pragma warning(push)
#pragma warning(disable: 28167)
VOID
	CFileName::GetLock()
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

/*  
* 函数说明：
*		释放锁
*
* 参数：
*		无
*
* 返回值：
*		无
*
* 备注：
*		无
* 
*/
#pragma warning(push)
#pragma warning(disable: 28167)
VOID
	CFileName::FreeLock()
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
	CFileName::GetParentPath(
	__in	CKrnlStr * pPath,
	__out	CKrnlStr * pParentPath
	)
{
	BOOLEAN		bRet				= FALSE;

	PWCHAR		pwchPostion			= NULL;
	PWCHAR		pwchPostionStart	= NULL;
	PWCHAR		pwchPostionEnd		= NULL;
	ULONG		ulCount				= 0;


	__try
	{
		if (!pPath || !pParentPath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pPath(%p) pParentPath(%p)",
				pPath, pParentPath);

			__leave;
		}

		if (!pParentPath->Set(pPath))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pParentPath->Set failed. Path(%wZ)",
				pPath->Get());

			__leave;
		}

		pwchPostionStart = pParentPath->GetString();
		pwchPostionEnd = pwchPostion = pParentPath->GetString() + pParentPath->GetLenCh() - 1;

		for (; pwchPostion >= pwchPostionStart; pwchPostion--)
		{
			if (L'\\' == *pwchPostion && pwchPostion != pwchPostionEnd)
				ulCount++;

			if (1 == ulCount)
			{
				if (!pParentPath->Shorten((USHORT)(pwchPostion - pwchPostionStart)))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pParentPath->Shorten failed. Path(%wZ)",
						pParentPath->Get());

					__leave;
				}

				bRet = TRUE;
				__leave;
			}
			else
			{
				if (2 <= ulCount)
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ulCount error. ulCount(%d) Path(%wZ)",
						ulCount, pParentPath->Get());

					__leave;
				}
			}
		}
	}
	__finally
	{
		;
	}

	return bRet;
}

/*
* 函数说明：
*		将文件的符号链接名或R3名转成设备名
*
* 参数：
*		pName		符号链接名或R3名	\\??\\C: 或者 C:
*		pDevName	设备名				\\Device\\HarddiskVolume1
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		无
*/
BOOLEAN 
	CFileName::ToDev(
	__in	CKrnlStr * pName,
	__inout	CKrnlStr * pDevName
	)
{
	BOOLEAN				bRet			= FALSE;

	CKrnlStr			PrePartName;
	CKrnlStr			PostPartName;

	CFileName			FileName;

	BOOLEAN				bDisk			= FALSE;
	PVOLUME_NAME_INFO	pVolNameInfo	= NULL;
	NAME_TYPE			NameType		= TYPE_UNKNOW;


	__try
	{
		FileName.GetLock();

		if (!pName || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pName(%p) pDevName(%p)",
				pName, pDevName);

			__leave;
		}

		if (FileName.IsSystemRootPath(pName))
		{
			if (!FileName.SystemRootToDev(pName, pDevName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SystemRootToDev failed. File(%wZ)",
					pName->Get());

				__leave;
			}

			bRet = TRUE;
			__leave;
		}

		if (!ParseAppOrSymName(pName, &PrePartName, &PostPartName, &bDisk, &NameType))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ParseAppOrSymName failed. Name(%wZ)",
				pName->Get());

			__leave;
		}

		if (TYPE_APP == NameType)
		{
			pVolNameInfo = FileName.GetVolNameInfoByVolAppName(&PrePartName);
			if (!pVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.GetVolNameInfoByVolAppName failed. Name(%wZ) Type(%x)",
					PrePartName.Get(), NameType);

				__leave;
			}
		}
		else if (TYPE_SYM == NameType)
		{
			pVolNameInfo = FileName.GetVolNameInfoByVolSymName(&PrePartName);
			if (!pVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.GetVolNameInfoByVolSymName failed. Name(%wZ) Type(%x)",
					PrePartName.Get(), NameType);

				__leave;
			}
		}
		else
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NameType error. Name(%wZ) Type(%x)",
				pName->Get(), NameType);

			__leave;
		}

		if (!pDevName->Set(&pVolNameInfo->DevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pDevName->Set failed. Vol(%wZ) Type(%x)",
				pVolNameInfo->DevName.Get(), NameType);

			__leave;
		}

		if (!bDisk)
		{
			if (!pDevName->Append(&PostPartName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pDevName->Append failed. Name(%wZ) Type(%x)",
					PostPartName.Get(), NameType);

				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		FileName.FreeLock();
	}

	return bRet;
}

BOOLEAN
	CFileName::GetFileFullPathFromDataAndFltVol(
	__in	PFLT_CALLBACK_DATA		pData,
	__in	PFLT_VOLUME				pFltVol,
	__out	CKrnlStr			*	pName
	)
{
	BOOLEAN		bRet			= FALSE;

	NTSTATUS	ntStatus		= STATUS_UNSUCCESSFUL;
	ULONG		ulVolNameLen	= 0;


	__try
	{
		if (!pData || !pFltVol || !pName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pData(%p) pFltVol(%p) pName(%p)",
				pData, pFltVol, pName);

			__leave;
		}

		if (!GetVolDevNameFromFltVol(pFltVol, pName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetDevNameFromFltVol failed. Name(%wZ)",
				&pData->Iopb->TargetFileObject->FileName);

			__leave;
		}

		if (pData->Iopb->TargetFileObject->FileName.Length)
		{
			if (L'\\' != *(pData->Iopb->TargetFileObject->FileName.Buffer))
			{
				if (!pName->Append(L"\\", wcslen(L"\\")))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pName->Append failed. Name(%wZ)",
						pName->Get());

					__leave;
				}
			}

			if (!pName->Append(&pData->Iopb->TargetFileObject->FileName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pName->Append failed. Name(%wZ)",
					&pData->Iopb->TargetFileObject->FileName);

				__leave;
			}
		}

		if (IRP_MJ_CREATE == pData->Iopb->MajorFunction && FlagOn(pData->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
		{
			if (!CFileName::GetParentPath(pName, pName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CFileName::GetParentPath failed. Name(%wZ)",
					&pData->Iopb->TargetFileObject->FileName);

				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CFileName::GetFileFullPath(
	__in	PFLT_CALLBACK_DATA		pData,
	__in	PFLT_VOLUME				pFltVol,
	__out	CKrnlStr			*	pDevName
	)
{
	BOOLEAN						bRet		= FALSE;

	NTSTATUS					ntStatus	= STATUS_UNSUCCESSFUL;
	PFLT_FILE_NAME_INFORMATION	pNameInfo	= NULL;


	__try
	{
		if (!pData || !pFltVol || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pData(%p) pFltVol(%p) pName(%p)",
				pData, pFltVol, pDevName);

			__leave;
		}

		ntStatus = FltGetFileNameInformation( 
			pData,
			FLT_FILE_NAME_NORMALIZED |
			FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
			&pNameInfo
			);
		if (!NT_SUCCESS(ntStatus))
		{
			if (!CFileName::GetFileFullPathFromDataAndFltVol(pData, pFltVol, pDevName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.GetNameFromDataAndFltVol failed. Name(%wZ)",
					&pData->Iopb->TargetFileObject->FileName);

				__leave;
			}
		}
		else
		{
			if (!pDevName->Set(&pNameInfo->Name))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pName->Set failed. Name(%wZ)",
					&pNameInfo->Name);

				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pNameInfo) 
		{
			FltReleaseFileNameInformation(pNameInfo);
			pNameInfo = NULL;
		}
	}

	return bRet;
}

/*
* 函数说明：
*		拼接文件名
*
* 参数：
*		pDirPath		文件所在文件夹
*		pFileName		文件名
*		pFilePath		文件全路径
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		无
*/
BOOLEAN
	CFileName::SpliceFilePath(
	__in	CKrnlStr * pDirPath,
	__in	CKrnlStr * pFileName,
	__inout	CKrnlStr * pFilePath
	)
{
	BOOLEAN		bRet			= FALSE;

	CKrnlStr	TmpFileName;


	__try
	{
		if (!pDirPath || !pFileName || !pFilePath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pDirPath(%p) pFileName(%p) pFilePath(%p)",
				pDirPath, pFileName, pFilePath);

			__leave;
		}

		if (!pFilePath->Set(pDirPath))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Set failed. DirPath(%wZ)",
				pDirPath->Get());

			__leave;
		}

		if (L'\\' == *(pFilePath->GetString() + pFilePath->GetLenCh() - 1))
		{
			if (!pFilePath->Shorten(pFilePath->GetLenCh() - 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Shorten failed. DirPath(%wZ)",
					pFilePath->Get());

				__leave;
			}
		}

		if (!TmpFileName.Set(pFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"TmpFileName.Set failed. FileName(%wZ)",
				pFileName->Get());

			__leave;
		}

		if (L'\\' != *TmpFileName.GetString())
		{
			if (!pFilePath->Append(L"\\", wcslen(L"\\")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Append failed. DirPath(%wZ)",
					pFilePath->Get());

				__leave;
			}
		}

		if (!pFilePath->Append(&TmpFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFilePath->Append failed. DirPath(%wZ) FileName(%wZ)",
				pFileName->Get(), TmpFileName.Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CFileName::GetVolDevNameFromFltVol(
	__in	PFLT_VOLUME		pFltVol,
	__out	CKrnlStr	*	pDevName
	)
{
	BOOLEAN		bRet			= FALSE;

	NTSTATUS	ntStatus		= STATUS_UNSUCCESSFUL;
	ULONG		ulDevNameLen	= 0;


	__try
	{
		if (!pFltVol || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pFltVol(%p) pName(%p)",
				pFltVol, pDevName);

			__leave;
		}

		ntStatus = FltGetVolumeName(
			pFltVol,
			NULL,
			&ulDevNameLen
			);
		if (NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltGetVolumeName succeed");
			__leave;
		}

		if (STATUS_BUFFER_TOO_SMALL != ntStatus)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ntStatus error. (%x)",
				ntStatus);

			__leave;
		}

		if (!pDevName->Lengthen((USHORT)ulDevNameLen / sizeof(WCHAR)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pName->Lengthen failed.");
			__leave;
		}

		ntStatus = FltGetVolumeName(
			pFltVol,
			pDevName->Get(),
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltGetVolumeName failed. (%x)",
				ntStatus);

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;	
}

BOOLEAN
	CFileName::InsertVolNameInfo(
	__in_opt	CKrnlStr		*	pAppName,
	__in_opt	CKrnlStr		*	pSymName,
	__in		CKrnlStr		*	pDevName,
	__in		BOOLEAN				bOnlyDevName,
	__in		BOOLEAN				bRemoveable,
	__in		PFLT_INSTANCE		pFltInstance,
	__in		ULONG				ulSectorSize
	)
{
	BOOLEAN				bRet			= FALSE;

	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;
	BOOLEAN				GetOld			= FALSE;


	__try
	{
		GetLock();

		if (!pDevName || !pFltInstance || !ulSectorSize)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pDevName(%p) pFltInstance(%p) ulSectorSize(%d)",
				pDevName, pFltInstance, ulSectorSize);

			__leave;
		}

		lpVolNameInfo = GetVolNameInfo(pDevName, TYPE_DEV);
		if (lpVolNameInfo)
		{
			GetOld = TRUE;

			lpVolNameInfo->pFltInstance = pFltInstance;
			lpVolNameInfo->ulSectorSize	= ulSectorSize;
			lpVolNameInfo->bRemoveable = bRemoveable;
			lpVolNameInfo->bOnlyDevName = bOnlyDevName;

			if (!lpVolNameInfo->bOnlyDevName)
			{
				if (!pAppName || !pSymName)
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[OLD] input arguments error. pAppName(%p) pSymName(%p)",
						pAppName, pSymName);

					__leave;
				}

				if (!lpVolNameInfo->AppName.Set(pAppName))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[OLD] lpVolNameInfo->AppName.Set failed. Name(%wZ)",
						pAppName->Get());

					__leave;
				}

				if (!lpVolNameInfo->AppName.ToUpper())
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[OLD] lpVolNameInfo->AppName.ToUpper failed. Name(%wZ)",
						lpVolNameInfo->AppName.Get());

					__leave;
				}

				if (!lpVolNameInfo->SymName.Set(pSymName))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[OLD] lpVolNameInfo->SymName.Set failed. Name(%wZ)",
						pSymName->Get());

					__leave;
				}

				if (!lpVolNameInfo->SymName.ToUpper())
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[OLD] lpVolNameInfo->SymName.ToUpper failed. Name(%wZ)",
						lpVolNameInfo->SymName.Get());

					__leave;
				}
			}

			bRet = TRUE;
			__leave;
		}

		lpVolNameInfo = new(MEMORY_TAG_FILE_NAME) VOLUME_NAME_INFO;

		lpVolNameInfo->pFltInstance = pFltInstance;
		lpVolNameInfo->ulSectorSize	= ulSectorSize;
		lpVolNameInfo->bRemoveable = bRemoveable;
		lpVolNameInfo->bOnlyDevName = bOnlyDevName;

		if (!lpVolNameInfo->bOnlyDevName)
		{
			if (!pAppName || !pSymName)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[NEW] input arguments error. pAppName(%p) pSymName(%p)",
					pAppName, pSymName);

				__leave;
			}

			if (!lpVolNameInfo->AppName.Set(pAppName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[NEW] lpVolNameInfo->AppName.Set failed. Name(%wZ)",
					pAppName->Get());

				__leave;
			}

			if (!lpVolNameInfo->AppName.ToUpper())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[NEW] lpVolNameInfo->AppName.ToUpper failed. Name(%wZ)",
					lpVolNameInfo->AppName.Get());

				__leave;
			}

			if (!lpVolNameInfo->SymName.Set(pSymName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[NEW] lpVolNameInfo->SymName.Set failed. Name(%wZ)",
					pSymName->Get());

				__leave;
			}

			if (!lpVolNameInfo->SymName.ToUpper())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[NEW] lpVolNameInfo->SymName.ToUpper failed. Name(%wZ)",
					lpVolNameInfo->SymName.Get());

				__leave;
			}
		}

		if (!lpVolNameInfo->DevName.Set(pDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"lpVolNameInfo->DevName.Set failed. Name(%wZ)",
				pDevName->Get());

			__leave;
		}

		if (!lpVolNameInfo->DevName.ToUpper())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"lpVolNameInfo->DevName.ToUpper failed. Name(%wZ)",
				lpVolNameInfo->DevName.Get());

			__leave;
		}

		InsertTailList(&ms_ListHead, &lpVolNameInfo->List);

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet && !GetOld)
		{
			delete lpVolNameInfo;
			lpVolNameInfo = NULL;
		}

		FreeLock();
	}

	return bRet;
}

/*
* 函数说明：
*		删除一条VOLUME_NAME_INFO
*
* 参数：
*		pName		卷的设备名		\\Device\\HarddiskVolume1
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		无
*/
BOOLEAN
	CFileName::DelVolNameInfo(
	__in CKrnlStr * pDevName
	)
{
	BOOLEAN				bRet			= FALSE;

	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;


	__try
	{
		GetLock();

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. Name(%wZ)",
				pDevName->Get());

			__leave;
		}

		lpVolNameInfo = GetVolNameInfoByVolDevName(pDevName);
		if (!lpVolNameInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetVolNameInfoByVolDevName failed. Name(%wZ)",
				pDevName->Get());

			__leave;
		}

		RemoveEntryList(&lpVolNameInfo->List);
		delete lpVolNameInfo;
		lpVolNameInfo = NULL;

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

/*
* 函数说明：
*		获得指定卷的VOLUME_NAME_INFO
*
* 参数：
*		pName	卷的设备名
*
* 返回值：
*		LPVOLUME_NAME_INFO
*		NULL	失败
*		!NULL	成功
*
* 备注：
*		无
*/
LPVOLUME_NAME_INFO 
	CFileName::GetVolNameInfoByVolDevName(
	__in CKrnlStr * pName
	)
{
	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;

	PLIST_ENTRY			pNode			= NULL;


	__try
	{
		GetLock();

		if (!pName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"List empty. Can not get. Vol(%wZ)",
				pName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. Vol(%wZ)",
					pName->Get());

				__leave;
			}

			if (lpVolNameInfo->DevName.Equal(pName, TRUE))
				__leave;
		}
	}
	__finally
	{
		FreeLock();
	}

	return lpVolNameInfo;
}

/*
* 函数说明：
*		获得卷的R3名
*
* 参数：
*		pName			文件的设备名
*		pAppName		卷的R3名
*		pulCutOffset	卷的设备名长度
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		无
*/
BOOLEAN
	CFileName::GetVolAppNameByQueryObj(
	__in	CKrnlStr *	pName,
	__inout CKrnlStr *	pAppName,
	__inout PUSHORT		pusCutOffset
	)
{
	BOOLEAN			bRet			=FALSE;

	PWCHAR			pVolume			= NULL;

	CKrnlStr		VolumeSym;
	CKrnlStr		VolumeDevEx;	


	__try
	{
		if (!pName || !pAppName || !pusCutOffset)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pName(%p) pAppName(%p) pusCutOffset(%p)",
				pName, pAppName, pusCutOffset);

			__leave;
		}

		// \\??\\A:
		if (!VolumeSym.Set(L"\\??\\A:", wcslen(L"\\??\\A:")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolumeSym.Set failed");
			__leave;
		}

		pVolume = VolumeSym.GetString() + 4;
		for (; *pVolume <= L'Z'; (*pVolume)++)
		{
			if (!CFileName::GetVolDevNameByQueryObj(&VolumeSym, &VolumeDevEx))
				continue;

			// \\Device\\HarddiskVolume1\\*
			if (!VolumeDevEx.Append(L"\\*", wcslen(L"\\*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolumeDevEx.Append failed. Vol(%wZ)",
					VolumeDevEx.Get());

				__leave;
			}

			// 转大写
			if (!VolumeDevEx.ToUpper())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolumeDevEx.ToUpper failed. Name(%wZ)",
					VolumeDevEx.Get());

				__leave;
			}

			if (FsRtlIsNameInExpression(
				VolumeDevEx.Get(),
				pName->Get(),
				TRUE,
				NULL
				))
			{
				// 获得卷的R3名
				if (!pAppName->Set(pVolume, wcslen(pVolume)))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"AppName->Set failed. Vol(%lS)",
						pVolume);

					__leave;
				}

				*pusCutOffset = VolumeDevEx.GetLenCh() - 2;
				bRet = TRUE;
				break;
			}

			if (!VolumeDevEx.Clean())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolumeDevEx.Clean failed. Vol(%wZ)",
					VolumeDevEx.Get());

				__leave;
			}
		}
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CFileName::IsVolume(
	__in PFLT_CALLBACK_DATA		pData,
	__in CKrnlStr			*	pDevName
	)
{
	BOOLEAN				bRet			= FALSE;

	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;
	PLIST_ENTRY			pNode			= NULL;

	CKrnlStr			FileName;


	__try
	{
		GetLock();

		if (!pData || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pData(%p) pFileName(%p)",
				pData, pDevName);

			__leave;
		}

		if (IRP_MJ_CREATE == pData->Iopb->MajorFunction && FlagOn(pData->Iopb->TargetFileObject->Flags, FO_VOLUME_OPEN))
		{
			bRet = TRUE;
			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. File(%wZ)",
				pDevName->Get());

			__leave;
		}

		if (!FileName.Set(pDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Set. File(%wZ)",
				pDevName->Get());

			__leave;
		}

		if (L'\\' == *(FileName.GetString() + FileName.GetLenCh() - 1))
		{
			if (!FileName.Shorten(FileName.GetLenCh() - 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Shorten. File(%wZ)",
					FileName.Get());

				__leave;
			}
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. File(%wZ)",
					pDevName->Get());

				__leave;
			}

			if (lpVolNameInfo->DevName.GetLenCh() == FileName.GetLenCh() &&
				lpVolNameInfo->DevName.Equal(&FileName, TRUE))
			{
				bRet = TRUE;
				__leave;
			}
		}
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CFileName::GetPathByHandle(
	__in	PFLT_CALLBACK_DATA		pData,
	__in	PCFLT_RELATED_OBJECTS	pFltObjects,
	__in	HANDLE					hFile,
	__out	CKrnlStr			*	pDevName
	)
{
	BOOLEAN					bRet			= FALSE;

	NTSTATUS				ntStatus		= STATUS_UNSUCCESSFUL;
	PFILE_OBJECT			pFileObj		= NULL;
	PFILE_NAME_INFORMATION	pFileNameInfo	= NULL;
	PWCHAR					pName			= NULL;

	CKrnlStr				RelativeFileName;


	__try
	{
		if (!pData || !hFile || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pData(%p) hFile(%p) pFileName(%p)",
				pData, hFile, pDevName);

			__leave;
		}

		ntStatus = ObReferenceObjectByHandle(
			hFile,
			0,
			NULL,
			KernelMode,
			(PVOID *)&pFileObj,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ObReferenceObjectByHandle failed. (%x) File(%wZ)",
				ntStatus, &pData->Iopb->TargetFileObject->FileName);

			__leave;
		}

		pFileNameInfo = (PFILE_NAME_INFORMATION)new(MEMORY_TAG_FILE_NAME) CHAR[sizeof(FILE_NAME_INFORMATION) + (MAX_PATH - 1) * sizeof(WCHAR)];

		ntStatus = FltQueryInformationFile(
			pData->Iopb->TargetInstance,
			pFileObj,
			pFileNameInfo,
			sizeof(FILE_NAME_INFORMATION) + (MAX_PATH - 1) * sizeof(WCHAR),
			FileNameInformation,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			// 碰到过返回STATUS_BUFFER_OVERFLOW，但ulRet为0的情况，因此提前就分配一个足够大的buffer
			// Comments
			// FltQueryInformationFile returns zero in any member of a FILE_XXX_INFORMATION structure that is not supported by a particular file system. 
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltQueryInformationFile failed. (%x) File(%wZ)",
				ntStatus, &pData->Iopb->TargetFileObject->FileName);

			__leave;
		}

		pName = (PWCHAR)new(MEMORY_TAG_FILE_NAME) CHAR[pFileNameInfo->FileNameLength + sizeof(WCHAR)];
		RtlCopyMemory(pName, pFileNameInfo->FileName, pFileNameInfo->FileNameLength);

		if (!pDevName->Set(pName, wcslen(pName)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileName->Set failed. File(%lS)",
				pName);

			__leave;
		}

		if (L'\\' == *(pDevName->GetString() + pDevName->GetLenCh() - 1))
		{
			if (!pDevName->Shorten(pDevName->GetLenCh() - 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileName->Shorten failed. File(%wZ)",
					pDevName->Get());

				__leave;
			}
		}

		if (!RelativeFileName.Set(pDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"RelativeFileName.Set failed. File(%wZ)",
				pDevName->Get());

			__leave;
		}

		if (!GetVolDevNameFromFltVol(pFltObjects->Volume, pDevName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetVolDevNameFromFltVol failed. File(%wZ)",
				&pData->Iopb->TargetFileObject->FileName);

			__leave;
		}

		if (!pDevName->Append(&RelativeFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileName->Append failed. Volume(%wZ) File(%wZ)",
				pDevName->Get(), RelativeFileName.Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		delete[] pFileNameInfo;
		pFileNameInfo = NULL;

		delete[] pName;
		pName = NULL;

		if (pFileObj)
		{
			ObDereferenceObject(pFileObj);
			pFileObj = NULL;
		}
	}

	return bRet;
}

BOOLEAN
	CFileName::IsExpression(
	__in CKrnlStr * pFileName
	)
{
	BOOLEAN bRet		= FALSE;

	PWCHAR	pPosition	= NULL;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		for (pPosition = pFileName->GetString(); pPosition < pFileName->GetString() + pFileName->GetLenCh(); pPosition++)
		{
			if (L'*' == *pPosition)
			{
				bRet = TRUE;
				__leave;
			}
		}
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CFileName::IsDisMountStandard(
	__in CKrnlStr		*	pVolDevName,
	__in PFLT_INSTANCE		pFltInstance
	)
{
	BOOLEAN				bRet			= FALSE;

	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;


	__try
	{
		GetLock();

		if (!pVolDevName || !pFltInstance)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pVolDevName(%p) pFltInstance(%p)",
				pVolDevName, pFltInstance);

			__leave;
		}

		lpVolNameInfo = GetVolNameInfo(pVolDevName, TYPE_DEV);
		if (!lpVolNameInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetVolNameInfo failed. Volume(%wZ)",
				pVolDevName->Get());

			__leave;
		}

		if (lpVolNameInfo->pFltInstance == pFltInstance)
			bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CFileName::GetFltInstance(
	__in	CKrnlStr		*	pFileName,
	__out	PFLT_INSTANCE	*	pPFltInstance,
	__in	NAME_TYPE			NameType
	)
{
	BOOLEAN				bRet			= FALSE;

	LIST_ENTRY*			pNode			= NULL;
	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;

	CKrnlStr			VolNameEx;


	__try
	{
		GetLock();

		if (!pFileName || !pPFltInstance)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pFileName(%p) pPFltInstance(%p)",
				pFileName, pPFltInstance);

			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. File(%wZ)",
					pFileName->Get());

				__leave;
			}

			switch (NameType)
			{
			case TYPE_DEV:
				{
					if (!VolNameEx.Set(&lpVolNameInfo->DevName))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolNameEx.Set failed. File(%wZ) Vol(%wZ)",
							pFileName->Get(), lpVolNameInfo->DevName.Get());

						__leave;
					}

					break;
				}
			case TYPE_APP:
				{
					if (!lpVolNameInfo->bOnlyDevName)
					{
						if (!VolNameEx.Set(&lpVolNameInfo->AppName))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolNameEx.Set failed. File(%wZ) Vol(%wZ)",
								pFileName->Get(), lpVolNameInfo->AppName.Get());

							__leave;
						}
					}

					break;
				}
			default:
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"NameType error. NameType(%d) File(%wZ)",
						NameType, pFileName->Get());

					__leave;
				}
			}

			if (!VolNameEx.Append(L"\\*", wcslen(L"\\*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolNameEx.Append failed. File(%wZ) Vol(%wZ)",
					pFileName->Get(), VolNameEx.Get());

				__leave;
			}

			if (FsRtlIsNameInExpression(
				VolNameEx.Get(),
				pFileName->Get(),
				TRUE,
				NULL
				))
			{
				*pPFltInstance = lpVolNameInfo->pFltInstance;

				bRet = TRUE;
				__leave;
			}
		}
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CFileName::GetSectorSize(
	__in	CKrnlStr	*	pDevName,
	__inout ULONG		*	pUlSectorSize
	)
{
	BOOLEAN				bRet			= FALSE;

	LIST_ENTRY*			pNode			= NULL;
	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;

	CKrnlStr			VolNameEx;


	__try
	{
		GetLock();

		if (!pDevName || !pUlSectorSize)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pFileName(%p) pUlSectorSize(%p)",
				pDevName, pUlSectorSize);

			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"list empty. File(%wZ)",
				pDevName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. File(%wZ)",
					pDevName->Get());

				__leave;
			}

			if (!VolNameEx.Set(&lpVolNameInfo->DevName))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolNameEx.Set failed. File(%wZ) Vol(%wZ)",
					pDevName->Get(), lpVolNameInfo->DevName.Get());

				__leave;
			}

			if (!VolNameEx.Append(L"\\*", wcslen(L"\\*")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"VolNameEx.Append failed. File(%wZ) Vol(%wZ)",
					pDevName->Get(), VolNameEx.Get());

				__leave;
			}

			if (FsRtlIsNameInExpression(
				VolNameEx.Get(),
				pDevName->Get(),
				TRUE,
				NULL
				))
			{
				*pUlSectorSize = lpVolNameInfo->ulSectorSize;

				bRet = TRUE;
				__leave;
			}
		}
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

/*
* 函数说明：
*		解析一个R3或者符号链接名
*
* 参数：
*		pName		文件R3或者符号链接名
*		pVolName	文件所在卷
*		pPartName	文件去除卷名后剩余部分
*		pbDisk		解析完后是否是盘符
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		没有将解析内核设备名写在这里
*		因为，无法直接区分本地硬盘和本地U盘的设备名
*		可判断是否是可移动介质，再进一步区分，太过繁琐
*/
BOOLEAN
	CFileName::ParseAppOrSymName(
	__in	CKrnlStr	*	pName,
	__inout CKrnlStr	*	pVolName,
	__inout CKrnlStr	*	pPartName,
	__inout PBOOLEAN		pbDisk,
	__inout PNAME_TYPE		pNameType
	)
{
	BOOLEAN	bRet		= FALSE;

	PWCHAR	pPosition	= NULL;


	__try
	{
		if (!pName || !pVolName || !pPartName || !pbDisk || !pNameType)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pName(%p) pVolName(%p) pPartName(%p) pbDisk(%p) pNameType(%p)",
				pName, pVolName, pPartName, pbDisk, pNameType);

			__leave;
		}

		if (!pVolName->Set(pName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName->Set failed. Name(%wZ)",
				pName->Get());

			__leave;
		}

		if ((L'a' <= *pVolName->GetString() && L'z' >= *pVolName->GetString())
			||
			(L'A' <= *pVolName->GetString() && L'Z' >= *pVolName->GetString()))
		{
			if (1 == pVolName->GetLenCh())
			{
				// c
				if (!pVolName->Append(L":", wcslen(L":")))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName->Append failed. Name(%wZ)",
						pVolName->Get());

					__leave;
				}
			
				*pbDisk = TRUE;
			}
			else if (2 == pName->GetLenCh())
			{
				// c:
				*pbDisk = TRUE;
			}
			else
			{
				if (!pVolName->Shorten(2))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName->Shorten failed. Name(%wZ)",
						pVolName->Get());

					__leave;
				}

				if (!pPartName->Set(pName->GetString() + 2, pName->GetLenCh() - 2))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pPartName->Set failed. Name(%wZ)",
						pName->Get());

					__leave;
				}
			}	

			*pNameType = TYPE_APP;
			bRet = TRUE;
			__leave;
		}

		if (L'?' == *(pVolName->GetString() + 1))
		{
			if (5 == pVolName->GetLenCh())
			{
				// \??\c
				if (!pVolName->Append(L":", wcslen(L":")))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName->Append failed. Name(%wZ)",
						pVolName->Get());

					__leave;
				}

				*pbDisk = TRUE;
			}
			else if (6 == pVolName->GetLenCh())
			{
				// \??\c:
				*pbDisk = TRUE;
			}
			else
			{
				if (L'\\' == *(pVolName->GetString() + 6))
				{
					// \??\c:\	
					if (!pVolName->Shorten(6))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName->Shorten failed. Name(%wZ)",
							pVolName->Get());

						__leave;
					}

					if (!pPartName->Set(pName->GetString() + 6, pName->GetLenCh() - 6))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pPartName->Set failed. Name(%wZ)",
							pName->Get());

						__leave;
					}
				}
				else
				{
					if (7 >= pVolName->GetLenCh())
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName len error. Name(%wZ)",
							pVolName->Get());

						__leave;
					}

					pPosition = pVolName->SearchCharacter(
						L'\\',
						pVolName->GetString() + 7,
						pVolName->GetString() + pVolName->GetLenCh() - 1
						);
					if (!pPosition)
					{
						// \??\volume{}
						*pbDisk = TRUE;
					}
					else
					{
						// \??\volume{}\	
						if (!pPartName->Set(pPosition, wcslen(pPosition)))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pPartName->Set failed. Name(%lS)",
								pPosition);

							__leave;
						}

						if (!pVolName->Shorten((USHORT)(pPosition - pVolName->GetString())))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pVolName->Shorten failed. Name(%wZ)",
								pVolName->Get());

							__leave;
						}
					}
				}
			}

			*pNameType = TYPE_SYM;
			bRet = TRUE;
		}
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CFileName::GetVolDevNameByQueryObj(
	__in	CKrnlStr * pName,
	__out	CKrnlStr * pDevName
	)
{
	BOOLEAN				bRet		= FALSE;

	OBJECT_ATTRIBUTES	Oa			= {0};
	NTSTATUS			ntStatus	= STATUS_UNSUCCESSFUL;
	HANDLE				Handle		= NULL;


	__try
	{
		if (!pName || !pDevName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pName(%p) pDevName(%p)",
				pName, pDevName);

			__leave;
		}

		InitializeObjectAttributes(
			&Oa, 
			pName->Get(),
			OBJ_CASE_INSENSITIVE ,
			NULL, 
			NULL
			);

		ntStatus = ZwOpenSymbolicLinkObject(
			&Handle, 
			GENERIC_READ, 
			&Oa
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"ZwOpenSymbolicLinkObject failed. (%x) Name(%wZ)",
				ntStatus, pName->Get());

			__leave;
		}

		if (!pDevName->Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pDevName.Init failed");
			__leave;
		}

		ntStatus = ZwQuerySymbolicLinkObject(
			Handle, 
			pDevName->Get(), 
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwQuerySymbolicLinkObject failed. (%x) Name(%wZ)",
				ntStatus, pName->Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (Handle)
		{
			ZwClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}
/*
* 函数说明：
*		获得指定卷的VOLUME_NAME_INFO
*
* 参数：
*		pName	卷的R3名
*
* 返回值：
*		LPVOLUME_NAME_INFO
*		NULL	失败
*		!NULL	成功
*
* 备注：
*		无
*/
LPVOLUME_NAME_INFO 
	CFileName::GetVolNameInfoByVolAppName(
	__in CKrnlStr * pAppName
	)
{
	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;

	PLIST_ENTRY			pNode			= NULL;


	__try
	{
		GetLock();

		if (!pAppName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"List empty. Can not get. Vol(%wZ)",
				pAppName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. Vol(%wZ)",
					pAppName->Get());

				__leave;
			}

			if (!lpVolNameInfo->bOnlyDevName)
			{
				if (lpVolNameInfo->AppName.Equal(pAppName, TRUE))
					__leave;
			}
		}
	}
	__finally
	{
		FreeLock();
	}

	return lpVolNameInfo;
}

/*
* 函数说明：
*		获得指定卷的VOLUME_NAME_INFO
*
* 参数：
*		pName	卷的符号链接名
*
* 返回值：
*		LPVOLUME_NAME_INFO
*		NULL	失败
*		!NULL	成功
*
* 备注：
*		无
*/
LPVOLUME_NAME_INFO 
	CFileName::GetVolNameInfoByVolSymName(
	__in CKrnlStr * pName
	)
{
	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;

	PLIST_ENTRY			pNode			= NULL;


	__try
	{
		GetLock();

		if (!pName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (IsListEmpty(&ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"List empty. Can not get. Vol(%wZ)",
				pName->Get());

			__leave;
		}

		for (pNode = ms_ListHead.Flink; pNode != &ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. Vol(%wZ)",
					pName->Get());

				__leave;
			}

			if (!lpVolNameInfo->bOnlyDevName)
			{
				if (lpVolNameInfo->SymName.Equal(pName, TRUE))
					__leave;
			}
		}
	}
	__finally
	{
		FreeLock();
	}

	return lpVolNameInfo;
}


/*
* 函数说明：
*		获得指定卷的VOLUME_NAME_INFO
*
* 参数：
*		pName		卷名或者文件全路径
*		VolNameType	传入的卷名类型
*
* 返回值：
*		LPVOLUME_NAME_INFO
*		NULL	失败
*		!NULL	成功
*
* 备注：
*		无
*/
LPVOLUME_NAME_INFO 
	CFileName::GetVolNameInfo(
	__in CKrnlStr	*	pName,
	__in NAME_TYPE		NameType
	)
{
	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;

	CFileName			FileName;

	CKrnlStr			VolNameEx;

	PLIST_ENTRY			pNode			= NULL;


	__try
	{
		FileName.GetLock();

		if (!pName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (IsListEmpty(&FileName.ms_ListHead))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"list empty. Name(%wZ)",
				pName->Get());

			__leave;
		}

		if (!FlagOn(NameType, TYPE_UNKNOW))
			NameType = (NAME_TYPE)(NameType | TYPE_UNKNOW);

		for (pNode = FileName.ms_ListHead.Flink; pNode != &FileName.ms_ListHead; pNode = pNode->Flink, lpVolNameInfo = NULL)
		{
			lpVolNameInfo = CONTAINING_RECORD(pNode, VOLUME_NAME_INFO, List);
			if (!lpVolNameInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"CONTAINING_RECORD failed. Name(%wZ)",
					pName->Get());

				__leave;
			}

			if (!FlagOn(NameType, TYPE_FULL_PATH))
			{
				if (!lpVolNameInfo->bOnlyDevName)
				{
					if (FlagOn(NameType, TYPE_APP))
					{
						if (lpVolNameInfo->AppName.Equal(pName, TRUE))
							__leave;
					}

					if (FlagOn(NameType, TYPE_SYM))
					{
						if (lpVolNameInfo->SymName.Equal(pName, TRUE))
							__leave;
					}
				}

				if (FlagOn(NameType, TYPE_DEV))
				{
					if (lpVolNameInfo->DevName.Equal(pName, TRUE))
						__leave;
				}
			}
			else
			{
				if (!lpVolNameInfo->bOnlyDevName)
				{
					if (FlagOn(NameType, TYPE_APP))
					{
						if (!VolNameEx.Set(&lpVolNameInfo->AppName))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[TYPE_APP] VolNameEx.Set failed. Name(%wZ)",
								lpVolNameInfo->AppName.Get());

							__leave;
						}

						if (!VolNameEx.Append(L"*", wcslen(L"*")))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[TYPE_APP] VolNameEx.Append failed. Name(%wZ)",
								VolNameEx.Get());

							__leave;
						}

						if (FsRtlIsNameInExpression(
							VolNameEx.Get(),
							pName->Get(),
							TRUE,
							NULL
							))
							__leave;
					}

					if (FlagOn(NameType, TYPE_SYM))
					{
						if (!VolNameEx.Set(&lpVolNameInfo->SymName))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[TYPE_SYM] VolNameEx.Set failed. Name(%wZ)",
								lpVolNameInfo->SymName.Get());

							__leave;
						}

						if (!VolNameEx.Append(L"*", wcslen(L"*")))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[TYPE_SYM] VolNameEx.Append failed. Name(%wZ)",
								VolNameEx.Get());

							__leave;
						}

						if (FsRtlIsNameInExpression(
							VolNameEx.Get(),
							pName->Get(),
							TRUE,
							NULL
							))
							__leave;
					}
				}

				if (FlagOn(NameType, TYPE_DEV))
				{
					if (!VolNameEx.Set(&lpVolNameInfo->DevName))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[TYPE_DEV] VolNameEx.Set failed. Name(%wZ)",
							lpVolNameInfo->DevName.Get());

						__leave;
					}

					if (VolNameEx.GetLenCh() == pName->GetLenCh())
					{
						if (VolNameEx.Equal(pName, TRUE))
							__leave;
					}
					else if(VolNameEx.GetLenCh() < pName->GetLenCh())
					{
						if (!VolNameEx.Append(L"\\*", wcslen(L"\\*")))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[TYPE_DEV] VolNameEx.Append failed. Name(%wZ)",
								VolNameEx.Get());

							__leave;
						}

						if (FsRtlIsNameInExpression(
							VolNameEx.Get(),
							pName->Get(),
							TRUE,
							NULL
							))
							__leave;
					}
				}
			}
		}
	}
	__finally
	{
		FileName.FreeLock();
	}

	return lpVolNameInfo;
}

/*
* 函数说明：
*		将文件的设备名或符号链接名转成R3名
*
* 参数：
*		Name		设备名或符号链接名
*		AppName		R3名
*
* 返回值：
*		TRUE	成功
*		FALSE	失败
*
* 备注：
*		无
*/
BOOLEAN 
	CFileName::ToApp(
	__in	CKrnlStr * pName,
	__inout CKrnlStr * pAppName
	)
{
	BOOLEAN				bRet			= FALSE;

	CKrnlStr			TmpName;

	CFileName			FileName;

	LPVOLUME_NAME_INFO	lpVolNameInfo	= NULL;
	USHORT				usCutOffset		= 0;


	__try
	{
		FileName.GetLock();

		if (!pName || !pAppName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pName(%p) pAppName(%p)",
				pName, pAppName);

			__leave;
		}

		if (!TmpName.Set(pName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"TmpName.Set failed. File(%wZ)",
				pName->Get());

			__leave;
		}

		if (!TmpName.Shorten(4))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"TmpName.Shorten failed. File(%wZ)",
				TmpName.Get());

			__leave;
		}

		if (TmpName.Equal(L"\\??\\", wcslen(L"\\??\\"), FALSE))
		{
			// 符号连接名
			if (!pAppName->Set(pName->GetString() + 4, pName->GetLenCh() - 4))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pAppName->Set failed. File(%wZ)",
					pName->Get());

				__leave;
			}

			bRet = TRUE;
			__leave;
		}

		// 设备名
		lpVolNameInfo = FileName.GetVolNameInfo(pName, (NAME_TYPE)(TYPE_DEV | TYPE_FULL_PATH));
		if (!lpVolNameInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.GetVolNameInfo failed. Name(%wZ)",
				pName->Get());

			__leave;
		}
		else
			usCutOffset = lpVolNameInfo->DevName.GetLenCh();

		if (!pAppName->Set(&lpVolNameInfo->AppName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pAppName->Set failed. Vol(%wZ)",
				lpVolNameInfo->AppName.Get());

			__leave;
		}

		if (pName->GetLenB() > usCutOffset)
		{
			if (!pAppName->Append(pName->GetString() + usCutOffset, pName->GetLenCh() - usCutOffset))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pAppName->Append failed. File(%wZ)",
					pName->Get());

				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		FileName.FreeLock();
	}

	return bRet;
}

BOOLEAN
	CFileName::IsSystemRootPath(
	__in CKrnlStr * pFileName
	)
{
	BOOLEAN		bRet		= FALSE;

	CKrnlStr	FileName;


	__try
	{
		if (!pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
			__leave;
		}

		if (11 > pFileName->GetLenCh())
			__leave;

		if (!FileName.Set(pFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Set failed. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		if (!FileName.Shorten(11))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileName.Shorten failed. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		bRet = FileName.Equal(L"\\systemroot", wcslen(L"\\systemroot"), TRUE);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CFileName::SystemRootToDev(
	__in	CKrnlStr * pFileName,
	__inout CKrnlStr * pFileNameDev
	)
{
	BOOLEAN bRet = FALSE;

	CKrnlStr FileNameSystemRoot;
	CKrnlStr FileNameTmpLong;
	CKrnlStr FileNameTmpShort;
	CKrnlStr FileNameWindows;

	PWCHAR	pPosition = NULL;


	__try
	{
		GetLock();

		if (!pFileName || !pFileNameDev)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pFileName(%p) pFileNameDev(%p)",
				pFileName, pFileNameDev);

			__leave;
		}

		if (!FileNameSystemRoot.Set(pFileName))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameSystemRoot.Set failed. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		if (!FileNameSystemRoot.Shorten(11))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameSystemRoot.Shorten failed. File(%wZ)",
				FileNameSystemRoot.Get());

			__leave;
		}

		if (!ConvertByZwQuerySymbolicLinkObject(&FileNameSystemRoot, &FileNameTmpLong))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ConvertByZwQuerySymbolicLinkObject failed. File(%wZ)",
				FileNameSystemRoot.Get());

			__leave;
		}

		if (!FileNameTmpShort.Set(&FileNameTmpLong))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameTmpShort.Set failed. File(%wZ)",
				FileNameTmpLong.Get());

			__leave;
		}

		for (pPosition = FileNameTmpShort.GetString() + FileNameTmpShort.GetLenCh() - 1; pPosition >= FileNameTmpShort.GetString(); pPosition--)
		{
			if (*pPosition == L'\\')
				break;
		}

		if (!FileNameWindows.Set(pPosition, FileNameTmpShort.GetLenCh() - (USHORT)(pPosition - FileNameTmpShort.GetString())))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameWindows.Set failed. Windows(%lS)",
				pPosition);

			__leave;
		}

		if (!FileNameTmpShort.Shorten((USHORT)(pPosition - FileNameTmpShort.GetString())))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileNameTmpShort.Shorten failed. File(%wZ)",
				FileNameTmpShort.Get());

			__leave;
		}

		if (!ConvertByZwQuerySymbolicLinkObject(&FileNameTmpShort, pFileNameDev))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ConvertByZwQuerySymbolicLinkObject failed. File(%wZ)",
				FileNameTmpShort.Get());

			__leave;
		}

		if (!pFileNameDev->Append(&FileNameWindows))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileNameDev->Append failed. File(%wZ) File(%wZ)",
				pFileNameDev->Get(), FileNameWindows.Get());

			__leave;
		}

		if (!pFileNameDev->Append(pFileName->GetString() + 11, pFileName->GetLenCh() - 11))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileNameDev->Append failed. File(%wZ) File(%wZ)",
				pFileNameDev->Get(), pFileName->Get());

			__leave;
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
	CFileName::ConvertByZwQuerySymbolicLinkObject(
	__in	CKrnlStr * pFileName,
	__inout CKrnlStr * pNewFileName
	)
{
	BOOLEAN				bRet		= FALSE;

	OBJECT_ATTRIBUTES	Oa			= {0};
	NTSTATUS			ntStatus	= STATUS_UNSUCCESSFUL;
	HANDLE				Handle		= NULL;


	__try
	{
		if (!pFileName || !pNewFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pFileName(%p) pNewFileName(%p)",
				pFileName, pNewFileName);

			__leave;
		}

		InitializeObjectAttributes(
			&Oa, 
			pFileName->Get(),
			OBJ_CASE_INSENSITIVE,
			NULL, 
			NULL
			);

		ntStatus = ZwOpenSymbolicLinkObject(
			&Handle, 
			GENERIC_READ, 
			&Oa
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwOpenSymbolicLinkObject failed. File(%wZ) (%x)",
				pFileName->Get(), ntStatus);

			__leave;
		}

		if (!pNewFileName->Init())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pNewFileName->Init failed");
			__leave;
		}

		ntStatus = ZwQuerySymbolicLinkObject(
			Handle, 
			pNewFileName->Get(), 
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ZwQuerySymbolicLinkObject failed. File(%wZ) (%x)",
				pFileName->Get(), ntStatus);

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (Handle)
		{
			ZwClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}
