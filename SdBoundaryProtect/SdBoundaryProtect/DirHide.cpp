/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		DirHide.cpp
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
#include "DirHide.h"

/*
* 函数说明：
*		隐藏文件夹文件
*
* 参数：
*		pData
*		Rule			规则
*		ParentPath		父目录
*
* 返回值：
*		无
*
* 备注：
*		无
*/
BOOLEAN 
	CDirHide::BreakLink(
	__inout PFLT_CALLBACK_DATA		pData,
	__in	CKrnlStr			*	pRuleEx,
	__in	CKrnlStr			*	pParentPath
	)
{
	BOOLEAN					bRet					= TRUE;

	CKrnlStr				Name;
	CKrnlStr				Path;
	CKrnlStr				Rule;

	PWCHAR					pWcharName				= NULL;
	ULONG_PTR				ulNextOffset			= 0;
	ULONG_PTR				ulNameOffset			= 0;
	ULONG_PTR				ulNameLengthOffset		= 0;
	FILE_INFORMATION_CLASS	FileInfoClass;
	LPVOID					lpStartFileInfo			= NULL;
	LPVOID					lpCurrentFileInfo		= NULL;
	LPVOID					lpPreviousFileInfo		= NULL;
	LPVOID					lpNextFileInfo 			= NULL;
	ULONG_PTR				ulFileInfoSize			= 0;


	__try
	{
		if (!pData || !pRuleEx || !pParentPath)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pData(%p) RuleEx(%p) ParentPath(%p)",
				pData, pRuleEx, pParentPath);

			__leave;
		}

		if (!Rule.Set(pRuleEx))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Rule.Set failed. Rule(%wZ)",
				pRuleEx->Get());

			__leave;
		}

		if (!Rule.Shorten(Rule.GetLenCh() - 2))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Rule.Shorten failed. Rule(%wZ)",
				Rule.Get());

			__leave;
		}

		if (pData->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress)
			lpCurrentFileInfo = MmGetSystemAddressForMdlSafe(
			pData->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
			NormalPagePriority
			);
		else
			lpCurrentFileInfo = pData->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

		if (!lpCurrentFileInfo)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pCurrentFileInfo error");
			__leave;
		}

		FileInfoClass = pData->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
		if (!GetDirInfoOffset(FileInfoClass, &ulNameOffset, &ulNameLengthOffset, &ulFileInfoSize))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetDirInfoOffset failed");
			__leave;
		}

		lpStartFileInfo = lpCurrentFileInfo;
		lpPreviousFileInfo = lpCurrentFileInfo;

		do 
		{
			ulNextOffset = *(PULONG)lpCurrentFileInfo;
			lpNextFileInfo = (LPVOID)((ULONG_PTR)lpCurrentFileInfo + ulNextOffset);  

			pWcharName = (PWCHAR)new(MEMORY_TAG_DIR_HIDE) CHAR[*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) + sizeof(WCHAR)];
			RtlCopyMemory(pWcharName, (PWCHAR)(((PCHAR)lpCurrentFileInfo) + ulNameOffset), *((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)));

			if (!Name.Set(pWcharName, wcslen(pWcharName)))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Name.Set failed. Name(%lS)",
					pWcharName);

				__leave;
			}

			if (!Path.Set(pParentPath))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Path.Set failed. Path(%wZ)",
					pParentPath->Get());

				__leave;
			}

			if (!Path.Append(L"\\", wcslen(L"\\")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Path.Append failed. Path(%wZ)",
					Path.Get());

				__leave;
			}

			if (!Path.Append(&Name))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Path.Append failed. Path(%wZ) Name(%wZ)",
					Path.Get(), Name.Get());

				__leave;
			}

			if ((FsRtlIsNameInExpression(pRuleEx->Get(), Path.Get(), TRUE, NULL) || Rule.Equal(&Path, TRUE))
				&&
				!Name.Equal(L".", wcslen(L"."), FALSE)
				&&
				!Name.Equal(L"..", wcslen(L".."), FALSE))
			{
				if (lpCurrentFileInfo == lpStartFileInfo)
				{
					if (ulNextOffset)
					{
						// 盘符根目录下有多个文件，并且第一个需要隐藏
						RtlCopyMemory((PWCHAR)(((PCHAR)lpCurrentFileInfo) + ulNameOffset), L".", sizeof(WCHAR));
						*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) = sizeof(WCHAR);
					}
					else
					{
						// 盘符根目录只有1个文件或文件夹，并且恰好是需要隐藏的
						pData->IoStatus.Status = STATUS_NO_MORE_FILES;
					}
				}
				else
					*(PULONG_PTR)lpPreviousFileInfo += ulNextOffset;

				if (pData->IoStatus.Information > ulFileInfoSize + (*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) - sizeof(WCHAR)))
					pData->IoStatus.Information -= (ulFileInfoSize + (*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) - sizeof(WCHAR)));
			}
			else
				lpPreviousFileInfo = lpCurrentFileInfo;

			lpCurrentFileInfo = lpNextFileInfo;

			delete[] pWcharName;
			pWcharName = NULL;
		} while (ulNextOffset);

		if (lpPreviousFileInfo != lpNextFileInfo)
			*(PULONG_PTR)lpPreviousFileInfo = 0;

		bRet = TRUE;
	}
	__finally
	{
		delete[] pWcharName;
		pWcharName = NULL;
	}

	return bRet;
}

/*
* 函数说明：
*		判断当前FileInformationClass的具体类型
*
* 参数：
*		FileInfoClass		传入当前的FileInformationClass
*		ulNameOffset		传出当前FileInformationClass类型的FileName的偏移
*		ulNameLengthOffset	传出当前FileInformationClass类型的FileNameLength的偏移
*
* 返回值：
*		STATUS_INVALID_PARAMETER	失败
*		STATUS_SUCCESS				成功
*
* 备注：
*		此函数目的是为了系统兼容
*		XP中获得的FileInformationClass的类型是FileBothDirectoryInformation
*		其他系统还未测试
*/
BOOLEAN
	CDirHide::GetDirInfoOffset(
	__in	FILE_INFORMATION_CLASS	FileInfoClass,
	__inout	PULONG_PTR				pNameOffset,
	__inout	PULONG_PTR				pNameLengthOffset,
	__inout	PULONG_PTR				pFileInfoSize
	)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pNameOffset || !pNameLengthOffset || !pFileInfoSize)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input arguments error. pNameOffset(%p) pNameLengthOffset(%p) pFileInfoSize(%p)",
				pNameOffset, pNameLengthOffset, pFileInfoSize);

			__leave;
		}

		switch (FileInfoClass)
		{
		case FileBothDirectoryInformation:
			{
				*pNameOffset = OFFSETOF(FILE_BOTH_DIR_INFORMATION, FileName);
				*pNameLengthOffset = OFFSETOF(FILE_BOTH_DIR_INFORMATION, FileNameLength);
				*pFileInfoSize = sizeof(FILE_BOTH_DIR_INFORMATION);
				break;
			}
		case FileDirectoryInformation:
			{
				*pNameOffset = OFFSETOF(FILE_DIRECTORY_INFORMATION, FileName);
				*pNameLengthOffset = OFFSETOF(FILE_DIRECTORY_INFORMATION, FileNameLength);
				*pFileInfoSize = sizeof(FILE_DIRECTORY_INFORMATION);
				break;
			}
		case FileFullDirectoryInformation:
			{
				*pNameOffset = OFFSETOF(FILE_FULL_DIR_INFORMATION, FileName);
				*pNameLengthOffset = OFFSETOF(FILE_FULL_DIR_INFORMATION, FileNameLength);
				*pFileInfoSize = sizeof(FILE_FULL_DIR_INFORMATION);
				break;
			}
		case FileNamesInformation:
			{
				*pNameOffset = OFFSETOF(FILE_NAMES_INFORMATION, FileName);
				*pNameLengthOffset = OFFSETOF(FILE_NAMES_INFORMATION, FileNameLength);
				*pFileInfoSize = sizeof(FILE_NAMES_INFORMATION);
				break;
			}
		case FileIdBothDirectoryInformation:
			{
				*pNameOffset = OFFSETOF(FILE_ID_BOTH_DIR_INFORMATION, FileName);
				*pNameLengthOffset = OFFSETOF(FILE_ID_BOTH_DIR_INFORMATION, FileNameLength);
				*pFileInfoSize = sizeof(FILE_ID_BOTH_DIR_INFORMATION);
				break;
			}
		case FileIdFullDirectoryInformation:
			{
				*pNameOffset = OFFSETOF(FILE_ID_FULL_DIR_INFORMATION, FileName);
				*pNameLengthOffset = OFFSETOF(FILE_ID_FULL_DIR_INFORMATION, FileNameLength);
				*pFileInfoSize = sizeof(FILE_ID_FULL_DIR_INFORMATION);
				break;
			}
		default:
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"FileInfoClass error. (%d)",
					FileInfoClass);

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
