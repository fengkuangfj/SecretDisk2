/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		File.cpp
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
#include "File.h"

CFile::CFile()
{
	;
}

CFile::~CFile()
{
	;
}

BOOLEAN
	CFile::DelFile(
	__in PFLT_CALLBACK_DATA		pData,
	__in CKrnlStr			*	pFileName,
	__in BOOLEAN				bDelTag,
	__in BOOLEAN				bRelation,
	__in FILE_OBJECT_TYPE			ObjType		
	)
{
	BOOLEAN							bRet				= FALSE;

	OBJECT_ATTRIBUTES				Oa					= {0};
	NTSTATUS						ntStatus			= STATUS_UNSUCCESSFUL;
	HANDLE							Handle				= NULL;
	IO_STATUS_BLOCK					Iosb				= {0};
	PFILE_OBJECT					pFileObj			= NULL;
	FILE_DISPOSITION_INFORMATION	DispositionInfo		= {0};
	ULONG							ulCreateOption		= 0;
	ULONG							ulFileAttributes	= 0;


	__try
	{
		if (!pData || !pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pData(%p) pFileName(%p)",
				pData, pFileName);

			__leave;
		}

		if (OBJECT_TYPE_UNKNOWN == ObjType)
			ObjType = GetObjType(pData, pFileName, bRelation);

		switch (ObjType)
		{
		case OBJECT_TYPE_FILE:
			{
				ulCreateOption = FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED;

				if (!GetFileAttributes(pData, pFileName, &ulFileAttributes))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetFileAttributes failed. File(%wZ)",
						pFileName->Get());

					__leave;
				}

				if (FlagOn(ulFileAttributes, FILE_ATTRIBUTE_READONLY))
				{
					ulFileAttributes &= ~FILE_ATTRIBUTE_READONLY;

					if (!SetFileAttributes(pData, pFileName, ulFileAttributes))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"SetFileAttributes failed. File(%wZ)",
							pFileName->Get());

						__leave;
					}
				}

				break;
			}
		case OBJECT_TYPE_DIR:
			{
				ulCreateOption = FILE_DIRECTORY_FILE;
				break;
			}
		default:
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ObjType error. (%d) File(%wZ)",
					ObjType, pFileName->Get());

				__leave;
			}
		}

		if (bRelation)
			pFileObj = pData->Iopb->TargetFileObject;
		else
		{
			InitializeObjectAttributes(
				&Oa,
				pFileName->Get(),
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
				NULL,
				NULL
				);

			ntStatus = FltCreateFile(
				CMinifilter::ms_pMfIns->m_pFltFilter,
				pData->Iopb->TargetInstance,
				&Handle,
				GENERIC_READ | GENERIC_WRITE | DELETE,
				&Oa,
				&Iosb,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				NULL,
				FILE_OPEN,
				ulCreateOption,
				NULL,
				0,
				IO_IGNORE_SHARE_ACCESS_CHECK
				);
			if (!NT_SUCCESS(ntStatus))
			{
				if (STATUS_SHARING_VIOLATION == ntStatus)
				{
					ntStatus = FltCreateFile(
						CMinifilter::ms_pMfIns->m_pFltFilter,
						pData->Iopb->TargetInstance,
						&Handle,
						GENERIC_READ | GENERIC_WRITE | DELETE,
						&Oa,
						&Iosb,
						NULL,
						FILE_ATTRIBUTE_NORMAL,
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_OPEN,
						ulCreateOption,
						NULL,
						0,
						IO_IGNORE_SHARE_ACCESS_CHECK
						);
					if (!NT_SUCCESS(ntStatus))
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile 2 failed. (%x) File(%wZ)",
							ntStatus, pFileName->Get());

						__leave;
					}
				}
				else
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile 1 failed. (%x) File(%wZ)",
						ntStatus, pFileName->Get());

					__leave;
				}
			}

			ntStatus = ObReferenceObjectByHandle(
				Handle,
				0,
				NULL,
				KernelMode,
				(PVOID *)&pFileObj,
				NULL
				);
			if (!NT_SUCCESS(ntStatus))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ObReferenceObjectByHandle failed. (%x) File(%wZ)",
					ntStatus, pFileName->Get());

				__leave;
			}
		}

		DispositionInfo.DeleteFile = bDelTag;

		ntStatus = FltSetInformationFile(
			pData->Iopb->TargetInstance,
			pFileObj,
			&DispositionInfo,
			sizeof(FILE_DISPOSITION_INFORMATION),
			FileDispositionInformation
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltSetInformationFile failed. (%x) File(%wZ)",
				ntStatus, pFileName->Get());

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (!bRelation && pFileObj)
		{
			ObDereferenceObject(pFileObj);
			pFileObj = NULL;
		}

		if (Handle)
		{
			FltClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}

FILE_OBJECT_TYPE
	CFile::GetObjType(
	__in PFLT_CALLBACK_DATA		pData,
	__in CKrnlStr			*	pFileName,
	__in BOOLEAN				bRelation
	)
{
	FILE_OBJECT_TYPE					ObjType				= OBJECT_TYPE_NULL;

	NTSTATUS					ntStatus			= STATUS_UNSUCCESSFUL;
	FILE_STANDARD_INFORMATION	FileStdInfo			= {0};
	HANDLE						Handle				= NULL;
	IO_STATUS_BLOCK				Iosb				= {0};
	OBJECT_ATTRIBUTES			Oa					= {0};
	BOOLEAN						bDirectory			= FALSE;
	BOOLEAN						bCreateUseOigInfo	= FALSE;

	CFileName					FileName;


	__try
	{
		if (!pData || !pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pData(%p) pFileName(%p)",
				pData, pFileName);

			__leave;
		}

		if (L'\\' != *(pFileName->GetString()))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pFileName error. File(%wZ)",
				pFileName->Get());

			__leave;
		}

		if (FileName.IsVolume(pData, pFileName))
		{
			ObjType = OBJECT_TYPE_VOLUME;
			__leave;
		}

		if (L'\\' == *(pFileName->GetString() + pFileName->GetLenCh() - 1))
		{
			ObjType = OBJECT_TYPE_DIR;
			__leave;
		}

		switch (pData->Iopb->MajorFunction)
		{
		case IRP_MJ_CREATE:
			{
				InitializeObjectAttributes(
					&Oa,
					pFileName->Get(),
					OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
					NULL,
					NULL
					);

				ntStatus = FltCreateFile(
					CMinifilter::ms_pMfIns->m_pFltFilter,
					pData->Iopb->TargetInstance,
					&Handle,
					FILE_READ_ATTRIBUTES,
					&Oa,
					&Iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					NULL,
					FILE_OPEN,
					FILE_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0,
					IO_IGNORE_SHARE_ACCESS_CHECK
					);
				if (NT_SUCCESS(ntStatus))
				{
					ObjType = OBJECT_TYPE_DIR;
					__leave;
				}

				if (STATUS_NOT_A_DIRECTORY == ntStatus)
				{
					ObjType = OBJECT_TYPE_FILE;
					__leave;
				}

				if (STATUS_SHARING_VIOLATION == ntStatus)
				{
					ObjType = OBJECT_TYPE_DIR;
					__leave;
				}

				if (STATUS_OBJECT_NAME_INVALID == ntStatus ||
					STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ||
					STATUS_OBJECT_PATH_INVALID == ntStatus ||
					STATUS_OBJECT_PATH_NOT_FOUND == ntStatus)
				{
					if (!CreateUseOigInfo(pData, pFileName, &ObjType))
					{
						// 						KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"CreateUseOigInfo failed. File(%wZ)",
						// 							pFileName->Get());

						__leave;
					}

					bCreateUseOigInfo = TRUE;
					__leave;
				}
				else if (STATUS_DELETE_PENDING == ntStatus)
					__leave;

				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[IRP_MJ_CREATE] FltCreateFile failed. (%x) File(%wZ)",
					ntStatus, pFileName->Get());

				break;
			}
		case IRP_MJ_READ:
		case IRP_MJ_WRITE:
		case IRP_MJ_QUERY_INFORMATION:
		case IRP_MJ_SET_INFORMATION:
		case IRP_MJ_DIRECTORY_CONTROL:
		case IRP_MJ_CLEANUP:
			{
				if (bRelation)
				{
					ntStatus = FltIsDirectory(
						pData->Iopb->TargetFileObject,
						pData->Iopb->TargetInstance,
						&bDirectory
						);
					if (NT_SUCCESS(ntStatus))
					{
						if (bDirectory)
							ObjType = OBJECT_TYPE_DIR;
						else
							ObjType = OBJECT_TYPE_FILE;

						__leave;
					}

					if (STATUS_FILE_DELETED == ntStatus)
					{
						// 						KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"FltIsDirectory failed. (%x) (%x) File(%wZ)",
						// 							ntStatus, pData->Iopb->MajorFunction, pFileName->Get());

						__leave;
					}

					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltIsDirectory failed. (%x) (%x) File(%wZ)",
						ntStatus, pData->Iopb->MajorFunction, pFileName->Get());

					ntStatus = FltQueryInformationFile(
						pData->Iopb->TargetInstance,
						pData->Iopb->TargetFileObject,
						&FileStdInfo,
						sizeof(FILE_STANDARD_INFORMATION),
						FileStandardInformation,
						NULL
						);
					if (NT_SUCCESS(ntStatus))
					{
						if (FileStdInfo.Directory)
							ObjType = OBJECT_TYPE_DIR;
						else
							ObjType = OBJECT_TYPE_FILE;

						__leave;
					}

					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltQueryInformationFile failed. (%x) (%x) File(%wZ)",
						ntStatus, pData->Iopb->MajorFunction, pFileName->Get());

					__leave;
				}

				InitializeObjectAttributes(
					&Oa,
					pFileName->Get(),
					OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
					NULL,
					NULL
					);

				ntStatus = FltCreateFile(
					CMinifilter::ms_pMfIns->m_pFltFilter,
					pData->Iopb->TargetInstance,
					&Handle,
					FILE_READ_ATTRIBUTES,
					&Oa,
					&Iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					NULL,
					FILE_OPEN,
					FILE_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0,
					IO_IGNORE_SHARE_ACCESS_CHECK
					);
				if (NT_SUCCESS(ntStatus) || STATUS_SHARING_VIOLATION == ntStatus)
				{
					ObjType = OBJECT_TYPE_DIR;
					__leave;
				}

				if (STATUS_NOT_A_DIRECTORY == ntStatus)
				{
					ObjType = OBJECT_TYPE_FILE;
					__leave;
				}

				if (STATUS_OBJECT_NAME_INVALID == ntStatus ||
					STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ||
					STATUS_OBJECT_PATH_INVALID == ntStatus ||
					STATUS_OBJECT_PATH_NOT_FOUND == ntStatus)
				{
					// 					KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. (%x) (%x) File(%wZ)",
					// 					ntStatus, pData->Iopb->MajorFunction, pFileName->Get());
					;
				}
				else
				{
					if (STATUS_DELETE_PENDING == ntStatus && IRP_MJ_DIRECTORY_CONTROL == pData->Iopb->MajorFunction)
						__leave;

					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. (%x) (%x) File(%wZ)",
						ntStatus, pData->Iopb->MajorFunction, pFileName->Get());
				}

				break;
			}
		default:
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pData->Iopb->MajorFunction error. MajorFunction(%x)",
					pData->Iopb->MajorFunction);

				__leave;
			}
		}
	}
	__finally
	{
		if (Handle)
		{
			FltClose(Handle);
			Handle = NULL;
		}

		if (bCreateUseOigInfo)
		{
			if (!DelFile(pData, pFileName, TRUE, FALSE, ObjType))
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"DelFile failed. File(%wZ)",
				pFileName->Get());
		}
	}

	return ObjType;
}

BOOLEAN
	CFile::CreateUseOigInfo(
	__in		PFLT_CALLBACK_DATA		pData,
	__in		CKrnlStr			*	pFileName,
	__out_opt	FILE_OBJECT_TYPE			*	ObjType
	)
{
	BOOLEAN				bRet				= FALSE;

	OBJECT_ATTRIBUTES	Oa					= {0};
	NTSTATUS			ntStatus			= STATUS_UNSUCCESSFUL;
	HANDLE				hFile				= NULL;
	IO_STATUS_BLOCK		Iosb				= {0};
	ULONG				ulCreateOptions		= 0;


	__try
	{
		if (!pData || !pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pData(%p) pFileName(%p)",
				pData, pFileName);

			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			pFileName->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		ulCreateOptions = (pData->Iopb->Parameters.Create.Options) & 0x00FFFFFF;
		if (FlagOn(ulCreateOptions, FILE_DELETE_ON_CLOSE))
			ulCreateOptions &= ~FILE_DELETE_ON_CLOSE;

		ntStatus = FltCreateFile(
			CMinifilter::ms_pMfIns->m_pFltFilter,
			pData->Iopb->TargetInstance,
			&hFile,
			pData->Iopb->Parameters.Create.SecurityContext->DesiredAccess,
			&Oa,
			&Iosb,
			&(pData->Iopb->Parameters.Create.AllocationSize),
			pData->Iopb->Parameters.Create.FileAttributes,
			pData->Iopb->Parameters.Create.ShareAccess,
			(pData->Iopb->Parameters.Create.Options) >> 24,
			ulCreateOptions,
			pData->Iopb->Parameters.Create.EaBuffer,
			pData->Iopb->Parameters.Create.EaLength,
			IO_FORCE_ACCESS_CHECK
			);
		if (!NT_SUCCESS(ntStatus))
		{
			// 			KdPrintKrnl(LOG_PRINTF_LEVEL_WARNING, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. (%x) File(%wZ)",
			// 				ntStatus, &pData->Iopb->TargetFileObject->FileName);

			__leave;
		}

		if (ObjType)
		{
			if (FlagOn(ulCreateOptions, FILE_DIRECTORY_FILE))
				*ObjType = OBJECT_TYPE_DIR;
			else
				*ObjType = OBJECT_TYPE_FILE;
		}

		bRet = TRUE;
	}
	__finally
	{
		if (hFile)
		{
			FltClose(hFile);
			hFile = NULL;
		}
	}

	return bRet;
}

BOOLEAN
	CFile::GetFileAttributes(
	__in PFLT_CALLBACK_DATA	pData,
	__in CKrnlStr*			pFileName,
	__in PULONG				pFileAttributes
	)
{
	BOOLEAN					bRet			= FALSE;

	OBJECT_ATTRIBUTES		Oa				= {0};
	NTSTATUS				ntStatus		= STATUS_UNSUCCESSFUL;
	HANDLE					Handle			= NULL;
	IO_STATUS_BLOCK			Iosb			= {0};
	PFILE_OBJECT			pFileObj		= NULL;
	FILE_BASIC_INFORMATION	FileBasicInfo	= {0};
	ULONG					ulRet			= 0;


	__try
	{
		if (!pData || !pFileName || !pFileAttributes)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pData(%p) pFileName(%p) pFileAttributes(%p)",
				pData, pFileName, pFileAttributes);

			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			pFileName->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);
		ntStatus = FltCreateFile(
			CMinifilter::ms_pMfIns->m_pFltFilter,
			pData->Iopb->TargetInstance,
			&Handle,
			GENERIC_READ | GENERIC_WRITE,
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
		if (!NT_SUCCESS(ntStatus))
		{
			if (ntStatus == STATUS_SHARING_VIOLATION)
			{
				ntStatus = FltCreateFile(
					CMinifilter::ms_pMfIns->m_pFltFilter,
					pData->Iopb->TargetInstance,
					&Handle,
					GENERIC_READ | GENERIC_WRITE,
					&Oa,
					&Iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					FILE_OPEN,
					FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0,
					IO_IGNORE_SHARE_ACCESS_CHECK
					);
				if (!NT_SUCCESS(ntStatus))
				{
					if (STATUS_SHARING_VIOLATION == ntStatus)
					{
						ntStatus = FltCreateFile(
							CMinifilter::ms_pMfIns->m_pFltFilter,
							pData->Iopb->TargetInstance,
							&Handle,
							GENERIC_READ,
							&Oa,
							&Iosb,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ | FILE_SHARE_DELETE,
							FILE_OPEN,
							FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
							NULL,
							0,
							IO_IGNORE_SHARE_ACCESS_CHECK
							);
						if (!NT_SUCCESS(ntStatus))
						{
							KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [1] (%x) File(%wZ)",
								ntStatus, pFileName->Get());

							__leave;
						}
					}
					else
					{
						KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [2] (%x) File(%wZ)",
							ntStatus, pFileName->Get());

						__leave;
					}
				}
			}
			else if (ntStatus == STATUS_ACCESS_DENIED)
			{
				ntStatus = FltCreateFile(
					CMinifilter::ms_pMfIns->m_pFltFilter,
					pData->Iopb->TargetInstance,
					&Handle,
					GENERIC_READ,
					&Oa,
					&Iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					FILE_OPEN,
					FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0,
					IO_IGNORE_SHARE_ACCESS_CHECK
					);
				if (!NT_SUCCESS(ntStatus))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [3] (%x) File(%wZ)",
						ntStatus, pFileName->Get());

					__leave;
				}
			}
			else
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [4] (%x) File(%wZ)",
					ntStatus, pFileName->Get());

				__leave;
			}
		}

		ntStatus = ObReferenceObjectByHandle(
			Handle,
			0,
			NULL,
			KernelMode,
			(PVOID *)&pFileObj,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ObReferenceObjectByHandle failed (%x)",
				ntStatus);

			__leave;
		}

		// 获取属性
		ntStatus = FltQueryInformationFile(
			pData->Iopb->TargetInstance,
			pFileObj,
			&FileBasicInfo,
			sizeof(FILE_BASIC_INFORMATION),
			FileBasicInformation,
			&ulRet
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltQueryInformationFile failed (%x) File(%wZ)",
				ntStatus, pFileName->Get());

			__leave;
		}

		*pFileAttributes = FileBasicInfo.FileAttributes;
		bRet = TRUE;
	}
	__finally
	{
		if (pFileObj)
		{
			ObDereferenceObject(pFileObj);
			pFileObj = NULL;
		}

		if (Handle)
		{
			FltClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}

BOOLEAN
	CFile::SetFileAttributes(
	__in PFLT_CALLBACK_DATA	pData,
	__in CKrnlStr*			pFileName,
	__in ULONG				ulFileAttributes
	)
{
	BOOLEAN					bRet				= FALSE;

	OBJECT_ATTRIBUTES		Oa					= {0};
	NTSTATUS				ntStatus			= STATUS_UNSUCCESSFUL;
	HANDLE					Handle				= NULL;
	IO_STATUS_BLOCK			Iosb				= {0};
	PFILE_OBJECT			pFileObj			= NULL;
	FILE_BASIC_INFORMATION	FileBasicInfo		= {0};
	FILE_BASIC_INFORMATION	FileBasicInfoNew	= {0};
	ULONG					ulRet				= 0;


	__try
	{
		if (!pData || !pFileName)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pData(%p) pFileName(%p)",
				pData, pFileName);

			__leave;
		}

		InitializeObjectAttributes(
			&Oa,
			pFileName->Get(),
			OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);
		ntStatus = FltCreateFile(
			CMinifilter::ms_pMfIns->m_pFltFilter,
			pData->Iopb->TargetInstance,
			&Handle,
			GENERIC_READ | GENERIC_WRITE,
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
		if (!NT_SUCCESS(ntStatus))
		{
			if (ntStatus == STATUS_SHARING_VIOLATION)
			{
				ntStatus = FltCreateFile(
					CMinifilter::ms_pMfIns->m_pFltFilter,
					pData->Iopb->TargetInstance,
					&Handle,
					GENERIC_READ | GENERIC_WRITE,
					&Oa,
					&Iosb,
					NULL,
					FILE_ATTRIBUTE_NORMAL,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					FILE_OPEN,
					FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0,
					IO_IGNORE_SHARE_ACCESS_CHECK
					);
				if (!NT_SUCCESS(ntStatus))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [ALL] (%x) File(%wZ)",
						ntStatus, pFileName->Get());

					__leave;
				}
			}
			else if (ntStatus == STATUS_ACCESS_DENIED)
			{
				ntStatus = FltCreateFile(
					CMinifilter::ms_pMfIns->m_pFltFilter,
					pData->Iopb->TargetInstance,
					&Handle,
					FILE_WRITE_ATTRIBUTES,
					&Oa,
					&Iosb,
					NULL,
					0,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					FILE_OPEN,
					FILE_NON_DIRECTORY_FILE | FILE_COMPLETE_IF_OPLOCKED,
					NULL,
					0,
					IO_IGNORE_SHARE_ACCESS_CHECK
					);
				if (!NT_SUCCESS(ntStatus))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [FILE_WRITE_ATTRIBUTES] (%x) File(%wZ)",
						ntStatus, pFileName->Get());

					__leave;
				}
			}
			else
			{
				if (STATUS_DELETE_PENDING != ntStatus)
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltCreateFile failed. [NULL] (%x) File(%wZ)",
					ntStatus, pFileName->Get());

				__leave;
			}
		}

		ntStatus = ObReferenceObjectByHandle(
			Handle,
			0,
			NULL,
			KernelMode,
			(PVOID *)&pFileObj,
			NULL
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ObReferenceObjectByHandle failed (%x)",
				ntStatus);

			__leave;
		}

		// 获取属性
		ntStatus = FltQueryInformationFile(
			pData->Iopb->TargetInstance,
			pFileObj,
			&FileBasicInfo,
			sizeof(FILE_BASIC_INFORMATION),
			FileBasicInformation,
			&ulRet
			);
		if (!NT_SUCCESS(ntStatus))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltQueryInformationFile failed (%x) File(%wZ)",
				ntStatus, pFileName->Get());

			__leave;
		}

		RtlCopyMemory(&FileBasicInfoNew, &FileBasicInfo, ulRet);

		// 设置属性
		if (FileBasicInfoNew.FileAttributes != ulFileAttributes)
		{
			FileBasicInfoNew.FileAttributes = ulFileAttributes;

			ntStatus = FltSetInformationFile(
				pData->Iopb->TargetInstance,
				pFileObj,
				&FileBasicInfoNew,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation
				);
			if (!NT_SUCCESS(ntStatus))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FltSetInformationFile failed (%x) File(%wZ)",
					ntStatus, pFileName->Get());

				__leave;
			}
		}

		bRet = TRUE;
	}
	__finally
	{
		if (pFileObj)
		{
			ObDereferenceObject(pFileObj);
			pFileObj = NULL;
		}

		if (Handle)
		{
			FltClose(Handle);
			Handle = NULL;
		}
	}

	return bRet;
}
