/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		Log.cpp
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
#include "Log.h"

LIST_ENTRY		CLog::ms_ListHead		= {0};
ERESOURCE		CLog::ms_Lock			= {0};
KSPIN_LOCK		CLog::ms_SpLock			= 0;
KEVENT			CLog::ms_UnloadEvent	= {0};
CKrnlStr*		CLog::ms_pLogFile		= NULL;
CKrnlStr*		CLog::ms_pLogDir		= NULL;
HANDLE			CLog::ms_hLogFile		= NULL;
PFILE_OBJECT	CLog::ms_pLogFileObj	= NULL;
LARGE_INTEGER	CLog::ms_liByteOffset	= {0};
PETHREAD		CLog::ms_pEThread		= NULL;
BOOLEAN			CLog::ms_bCanInsertLog	= FALSE;
ULONG			CLog::ms_ulSectorSize	= 0;
PFLT_INSTANCE	CLog::ms_pFltInstance	= NULL;

CLog::CLog()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

CLog::~CLog()
{
	RtlZeroMemory(&m_Irql, sizeof(m_Irql));
	m_LockRef = 0;
}

VOID 
	CLog::ThreadStart( 
	__in PVOID StartContext
	)
{
	LARGE_INTEGER	WaitTime		= {0};
	ULONG			ulCount			= 0;
	NTSTATUS		ntStatus		= STATUS_UNSUCCESSFUL;

	CKrnlStr		LogInfo;

	CLog			Log;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"begin");

	__try
	{
		Log.GetLock();

		WaitTime.QuadPart = - 100 * 1000;

		do 
		{
			Log.FreeLock();
			ntStatus = KeWaitForSingleObject(
				&ms_UnloadEvent,
				Executive,
				KernelMode,
				FALSE,
				&WaitTime
				);
			Log.GetLock();
			switch (ntStatus)
			{
			case STATUS_SUCCESS:
				{
					Log.FreeLock();
					if (Log.LogFileReady())
					{
						Log.GetLock();

						do 
						{
							if (!Log.Pop(&LogInfo))
							{
								// KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"[STATUS_SUCCESS] Log.Pop failed");
								break;
							}

							Log.FreeLock();
							if (!Log.Write(&LogInfo))
							{
								Log.GetLock();
								KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"[STATUS_SUCCESS] Log.Write failed. Log(%wZ)",
									LogInfo.Get());

								__leave;
							}
							Log.GetLock();
						} while (TRUE);
					}
					else
						Log.GetLock();

					__leave;
				}
			case STATUS_TIMEOUT:
				{
					Log.FreeLock();
					if (Log.LogFileReady())
					{
						Log.GetLock();

						do 
						{
							if (!Log.Pop(&LogInfo))
							{
								// KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"[STATUS_TIMEOUT] Log.Pop failed");
								break;
							}

							Log.FreeLock();
							if (!Log.Write(&LogInfo))
							{
								Log.GetLock();
								KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"[STATUS_TIMEOUT] Log.Write failed. Log(%wZ)",
									LogInfo.Get());

								__leave;
							}
							Log.GetLock();

							ulCount++;
						} while (MAX_EVERY_TIME_LOG_COUNT > ulCount);
					}
					else
					{
						Log.InitLogFile(TRUE);
						Log.GetLock();
					}

					break;
				}
			default:
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"KeWaitForSingleObject failed. (%x)",
						ntStatus);

					__leave;
				}
			}

			ulCount = 0;
		} while (TRUE);
	}
	__finally
	{
		Log.FreeLock();

		if (!Log.ReleaseLogFile())
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RleaseLogFile failed");
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"end");

	return;
}

BOOLEAN
	CLog::Init()
{
	BOOLEAN		bRet		= FALSE;

	NTSTATUS	ntStatus	= STATUS_UNSUCCESSFUL;
	HANDLE		hThead		= NULL;  


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"begin");

	__try
	{
		InitializeListHead(&ms_ListHead);
		KeInitializeSpinLock(&ms_SpLock);	
		ExInitializeResourceLite(&ms_Lock);
		KeInitializeEvent(&ms_UnloadEvent, NotificationEvent, FALSE);

		ms_pLogFile = new(MEMORY_TAG_LOG) CKrnlStr;
		ms_pLogDir = new(MEMORY_TAG_LOG) CKrnlStr;

		ntStatus = PsCreateSystemThread(
			&hThead,
			GENERIC_ALL,
			NULL,
			NULL,
			NULL,
			ThreadStart,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"PsCreateSystemThread failed. (%x)",
				ntStatus);

			__leave;
		}

		ntStatus = ObReferenceObjectByHandle(
			hThead,
			GENERIC_ALL,
			*PsThreadType,
			KernelMode,
			(PVOID *)&ms_pEThread,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ObReferenceObjectByHandle failed. (%x)",
				ntStatus);

			__leave;
		}

		KeSetBasePriorityThread(ms_pEThread, -4);

		ms_bCanInsertLog = TRUE;

		bRet = TRUE;
	}
	__finally
	{
		// 不要置为NULL
		if (ms_pEThread)
			ObDereferenceObject(ms_pEThread);

		if (hThead)
		{
			ZwClose(hThead);
			hThead = NULL;
		}

		if (!bRet)
		{
			delete ms_pLogFile;
			ms_pLogFile = NULL;

			delete ms_pLogDir;
			ms_pLogDir = NULL;

			ExDeleteResourceLite(&ms_Lock);
			RtlZeroMemory(&ms_Lock, sizeof(ms_Lock));
		}
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"end");

	return bRet;
}

BOOLEAN
	CLog::Unload()
{
	BOOLEAN			bRet		= FALSE;

	NTSTATUS		ntStatus	= STATUS_UNSUCCESSFUL;
	KPRIORITY		kPriority	= 0;
	PLIST_ENTRY		pNode		= NULL;
	LPLOG_INFO		lpLogInfo	= NULL;

	CKrnlStr		LogInfo;


	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"begin");

	__try
	{
		GetLock();

		ms_bCanInsertLog = FALSE;

		FreeLock();
		if (KeSetEvent(&ms_UnloadEvent, kPriority, FALSE))
		{
			GetLock();
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"already signaled");
			__leave;
		}

		ntStatus = KeWaitForSingleObject(
			ms_pEThread,
			Executive,
			KernelMode,
			FALSE,
			NULL
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"KeWaitForSingleObject failed. (%x)",
				ntStatus);

			__leave;
		}

		while (!IsListEmpty(&ms_ListHead))
		{
			pNode = RemoveHeadList(&ms_ListHead);

			lpLogInfo = CONTAINING_RECORD(pNode, LOG_INFO, List);
			if (!lpLogInfo)
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CONTAINING_RECORD failed");
				__leave;
			}

			delete lpLogInfo;
			lpLogInfo = NULL;
		}

		bRet = TRUE;
	}
	__finally
	{
		delete ms_pLogFile;
		ms_pLogFile = NULL;

		delete ms_pLogDir;
		ms_pLogDir = NULL;

		FreeLock();

		ExDeleteResourceLite(&ms_Lock);
		RtlZeroMemory(&ms_Lock, sizeof(ms_Lock));
	}

	KdPrintKrnl(LOG_PRINTF_LEVEL_INFO, LOG_RECORED_LEVEL_NEEDNOT, L"end");

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
	CLog::GetLock()
{
	__try
	{
		// 判断当前锁引用计数是否合法
		if (m_LockRef < 0)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"m_LockRef < 0 (%d)", m_LockRef);
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
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"IRQL > DISPATCH_LEVEL");
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
	CLog::FreeLock()
{
	__try
	{
		// 判断当前锁引用计数是否合法
		if (m_LockRef <= 0)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"m_LockRef <= 0 (%d)", m_LockRef);
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
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"IRQL > DISPATCH_LEVEL");
	}
	__finally
	{
		;
	}

	return;
}
#pragma warning(pop)

BOOLEAN
	CLog::Pop(
	__out CKrnlStr * pLog
	)
{
	BOOLEAN		bRet		= FALSE;

	LPLOG_INFO	lpLogInfo	= NULL;
	LIST_ENTRY*	pNode		= NULL;


	__try
	{
		GetLock();

		if (!pLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		pNode = RemoveHeadList(&ms_ListHead);
		if (pNode == &ms_ListHead)
		{
			// KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"list empty");
			__leave;
		}

		lpLogInfo = CONTAINING_RECORD(pNode, LOG_INFO, List);
		if (!lpLogInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CONTAINING_RECORD failed");
			__leave;
		}

		if (!pLog->Set(&lpLogInfo->Log))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"pLog->Set failed. Log(%wZ)",
				lpLogInfo->Log.Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		delete lpLogInfo;
		lpLogInfo = NULL;

		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CLog::Write(
	__in CKrnlStr * pLog
	)
{
	BOOLEAN			bRet		= FALSE;

	NTSTATUS		ntStatus	= STATUS_UNSUCCESSFUL;
	ULONG			ulWriteLen	= 0;
	PCHAR  			pWriteBuf	= NULL;

	CKrnlStr		Log;


	__try
	{
		GetLock();

		if (!pLog)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!Log.Set(L"\r\n", wcslen(L"\r\n")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Log.Set failed");
			__leave;
		}

		if (!Log.Append(pLog))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"Log.Append failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		ntStatus = RtlUnicodeToMultiByteSize(&ulWriteLen, Log.GetString(), Log.GetLenB());
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeToMultiByteSize failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		ulWriteLen = (ULONG)ROUND_TO_SIZE(ulWriteLen, ms_ulSectorSize);

		pWriteBuf = (PCHAR)FltAllocatePoolAlignedWithTag(
			ms_pFltInstance,
			PagedPool,
			ulWriteLen,
			MEMORY_TAG_LOG
			);
		if (!pWriteBuf)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltAllocatePoolAlignedWithTag failed");
			__leave;
		}

		RtlZeroMemory(pWriteBuf, ulWriteLen);

		ntStatus = RtlUnicodeToMultiByteN(
			pWriteBuf,
			ulWriteLen,
			NULL,
			Log.GetString(),
			Log.GetLenB()
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlUnicodeToMultiByteN failed. Log(%wZ)",
				pLog->Get());

			__leave;
		}

		FreeLock();
		ntStatus = FltWriteFile(
			ms_pFltInstance,
			ms_pLogFileObj,
			&ms_liByteOffset,
			ulWriteLen,
			pWriteBuf,
			FLTFL_IO_OPERATION_NON_CACHED,
			NULL,
			NULL,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			GetLock();
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltWriteFile failed. (%x) Log(%wZ)",
				ntStatus, pLog->Get());

			__leave;
		}
		GetLock();

		ms_liByteOffset.QuadPart = (ULONG)ROUND_TO_SIZE(ms_liByteOffset.QuadPart + ulWriteLen, ms_ulSectorSize);

		// 刷缓存，会导致prewrite捕获
		// FltFlushBuffers(ms_pFltInstance, ms_pLogFileObj);

		if (MAX_LOG_FILE_SIZE < ms_liByteOffset.QuadPart)
		{
			FreeLock();
			if (!InitLogFile(TRUE))
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogFile failed");
				__leave;
			}
			GetLock();
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pWriteBuf)
		{
			FltFreePoolAlignedWithTag(
				ms_pFltInstance,
				pWriteBuf,
				MEMORY_TAG_LOG
				);
			pWriteBuf = NULL;
		}

		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CLog::LogFileReady()
{
	BOOLEAN				bRet		= FALSE;

	OBJECT_ATTRIBUTES	Oa			= {0};
	NTSTATUS			ntStatus	= STATUS_UNSUCCESSFUL;
	HANDLE				Handle		= NULL;
	IO_STATUS_BLOCK		Iosb		= {0};


	__try
	{
		GetLock();

		if (!CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CMinifilter::ms_pMfIns->CheckEnv failed");
			__leave;
		}

		if (!ms_pFltInstance)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pFltInstance error");
			__leave;
		}

		if (!ms_pLogFile || !ms_pLogFile->GetLenCh())
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile error");
			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			ms_pLogFile->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		FreeLock();
		ntStatus = FltCreateFile(
			CMinifilter::ms_pMfIns->m_pFltFilter,
			ms_pFltInstance,
			&Handle,
			FILE_READ_ATTRIBUTES,
			&Oa,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			NULL,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			if (STATUS_DELETE_PENDING == ntStatus)
				KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed. (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());
			else
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed. (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		if (Handle)
		{
			FltClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}

BOOLEAN
	CLog::InitLogFile(
	__in BOOLEAN bReset
	)
{
	BOOLEAN						bRet				= FALSE;

	OBJECT_ATTRIBUTES			Oa					= {0};
	NTSTATUS					ntStatus			= STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK				Iosb				= {0};
	FILE_STANDARD_INFORMATION	FileStandardInfo	= {0};
	ULONG						ulRet				= 0;


	__try
	{
		GetLock();

		if (bReset)
		{
			FreeLock();
			if (!ReleaseLogFile())
			{
				GetLock();
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RleaseLogFile failed");
				__leave;
			}
			GetLock();
		}

		if (!CMinifilter::ms_pMfIns->CheckEnv(MINIFILTER_ENV_TYPE_FLT_FILTER))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"CMinifilter::ms_pMfIns->CheckEnv failed");
			__leave;
		}

		if (!ms_pFltInstance)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pFltInstance error");
			__leave;
		}

		if (!InitLogFileName())
		{
			// KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"InitLogFileName failed");
			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			ms_pLogFile->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		FreeLock();
		ntStatus = FltCreateFile(
			CMinifilter::ms_pMfIns->m_pFltFilter,
			ms_pFltInstance,
			&ms_hLogFile,
			GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
			&Oa,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			NULL,
			FILE_OPEN_IF,
			FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED | FILE_NO_INTERMEDIATE_BUFFERING,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed. (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		ntStatus = ObReferenceObjectByHandle(
			ms_hLogFile,
			0,
			NULL,
			KernelMode,
			(PVOID *)&ms_pLogFileObj,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ObReferenceObjectByHandle failed. (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		ntStatus = FltQueryInformationFile(
			ms_pFltInstance,
			ms_pLogFileObj,
			&FileStandardInfo,
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation,
			&ulRet
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltQueryInformationFile failed. (%x) File(%wZ)",
				ntStatus, ms_pLogFile->Get());

			__leave;
		}

		ms_liByteOffset.QuadPart = (ULONG)ROUND_TO_SIZE(FileStandardInfo.EndOfFile.QuadPart, ms_ulSectorSize);

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		if (!bRet)
		{
			if (!ReleaseLogFile())
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RleaseLogFile failed");
		}
	}

	return bRet;
}

BOOLEAN
	CLog::InitLogFileName()
{
	BOOLEAN			bRet		= FALSE;

	NTSTATUS		ntStatus	= STATUS_UNSUCCESSFUL;
	LARGE_INTEGER	snow		= {0};
	LARGE_INTEGER	now			= {0};
	TIME_FIELDS		nowFields	= {0};
	WCHAR			Time[64]	= {0};


	__try
	{
		GetLock();

		if (!ms_pLogFile)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile error");
			__leave;
		}

		if (!ms_pLogDir->GetLenCh())
		{
			// KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogDir->GetLenCh failed");
			__leave;
		}

		if (!ms_pLogFile->Set(ms_pLogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Set failed. Dir(%wZ)",
				ms_pLogDir->Get());

			__leave;
		}

		if (!ms_pLogFile->Append(L"\\SdBoundatyProtect", wcslen(L"\\SdBoundatyProtect")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed 1. File(%wZ)",
				ms_pLogFile->Get());

			__leave;
		}

		KeQuerySystemTime(&snow);
		ExSystemTimeToLocalTime(&snow, &now);
		RtlTimeToTimeFields(&now, &nowFields);

		ntStatus = RtlStringCchPrintfW(
			Time,
			64,
			L"[%04d-%02d-%02d][%02d-%02d-%02d.%03d]",
			nowFields.Year,
			nowFields.Month,
			nowFields.Day,
			nowFields.Hour,
			nowFields.Minute,
			nowFields.Second,
			nowFields.Milliseconds
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"RtlStringCchPrintfW failed. (%x)",
				ntStatus);

			__leave;
		}

		if (!ms_pLogFile->Append(Time, wcslen(Time)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed 2. File(%wZ)",
				ms_pLogFile->Get());

			__leave;
		}

		if (!ms_pLogFile->Append(L".log", wcslen(L".log")))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogFile->Append failed 3. File(%wZ)",
				ms_pLogFile->Get());

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
	CLog::Insert(
	__in WCHAR	*	pLog,
	__in USHORT		usLenCh
	)
{
	BOOLEAN		bRet		= FALSE;

	LPLOG_INFO lpLogInfo	= NULL;


	__try
	{
		GetLock();

		if (!ms_bCanInsertLog)
		{
			// KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"not ready for insert");
			__leave;
		}

		if (!pLog || !usLenCh)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input arguments error. pLog(%p) usLenCh(%d)",
				pLog, usLenCh);

			__leave;
		}

		lpLogInfo = new(MEMORY_TAG_LOG) LOG_INFO;

		if (!lpLogInfo->Log.Set(pLog, usLenCh))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"lpLogInfo->Log.Set failed. Log(%lS)",
				pLog);

			__leave;
		}

		InsertTailList(&ms_ListHead, &lpLogInfo->List);

		bRet = TRUE;
	}
	__finally
	{
		if (!bRet)
		{
			delete lpLogInfo;
			lpLogInfo = NULL;
		}

		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CLog::ReleaseLogFile()
{
	BOOLEAN bRet = FALSE;


	__try
	{
		GetLock();

		if (ms_pLogFileObj)
		{
			ObDereferenceObject(ms_pLogFileObj);
			ms_pLogFileObj = NULL;
		}

		if (ms_hLogFile)
		{
			FreeLock();
			FltClose(ms_hLogFile);
			GetLock();
			ms_hLogFile = NULL;
		}

		ms_liByteOffset.QuadPart = 0;

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();
	}

	return bRet;
}

BOOLEAN
	CLog::SetLogDir(
	__in CKrnlStr * pLogDir
	)
{
	BOOLEAN					bRet		= FALSE;

	OBJECT_ATTRIBUTES		Oa			= {0};
	NTSTATUS				ntStatus	= STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK			Iosb		= {0};
	HANDLE					hDir		= NULL;

	CFileName	FileName;


	__try
	{
		GetLock();

		if (!pLogDir)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"input argument error");
			__leave;
		}

		if (!FileName.GetSectorSize(pLogDir, &ms_ulSectorSize))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FileName.GetSectorSize failed. Dir(%wZ)",
				pLogDir->Get());

			__leave;
		}

		if (!FileName.GetFltInstance(pLogDir, &ms_pFltInstance, TYPE_DEV))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FileName.GetFltInstance failed. Dir(%wZ)",
				pLogDir->Get());

			__leave;
		}

		if (!ms_pLogDir->Set(pLogDir))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"ms_pLogDir->Set failed. Dir(%wZ)",
				pLogDir->Get());

			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			ms_pLogDir->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		FreeLock();
		ntStatus = FltCreateFile(
			CMinifilter::ms_pMfIns->m_pFltFilter,
			ms_pFltInstance,
			&hDir,
			GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
			&Oa,
			&Iosb,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			NULL,
			FILE_OPEN_IF,
			FILE_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED | FILE_NO_INTERMEDIATE_BUFFERING,
			NULL,
			0,
			IO_IGNORE_SHARE_ACCESS_CHECK
			);
		GetLock();
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEEDNOT, L"FltCreateFile failed. (%x) Dir(%wZ)",
				ntStatus, ms_pLogDir->Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		FreeLock();

		if (hDir)
		{
			FltClose(hDir);
			hDir = NULL;
		}
	}

	return bRet;
}
