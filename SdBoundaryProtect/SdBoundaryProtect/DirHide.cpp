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
	__inout PFLT_CALLBACK_DATA	pData,
	__in	CKrnlStr*			RuleEx,
	__in	CKrnlStr*			ParentPath
	)
{
	BOOLEAN					bRet					= TRUE;

	CKrnlStr				Name;
	CKrnlStr				Path;
	CKrnlStr				Rule;
	PWCHAR					pWcharName				= NULL;

	//FILE_INFORMATION_CLASS 相关域偏移量
	ULONG_PTR				ulNextOffset			= 0;
	ULONG_PTR				ulNameOffset			= 0;
	ULONG_PTR				ulNameLengthOffset		= 0;

	FILE_INFORMATION_CLASS	FileInfoClass;

	LPVOID					lpStartFileInfo			= NULL;
	LPVOID					lpCurrentFileInfo		= NULL;
	LPVOID					lpPreviousFileInfo		= NULL;
	LPVOID					lpNextFileInfo 			= NULL;
	ULONG_PTR				ulFileInfoSize			= 0;


	if (!pData || !RuleEx || !RuleEx->Get() || !ParentPath || !ParentPath->Get())
	{
		KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"[CDirProtect::BreakLink : input argument error");
		return FALSE;
	}

	// 获得规则字符串
	if (!Rule.Set(RuleEx))
	{
		KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Rule.Set failed");
		return FALSE;
	}

	if (!Rule.Shorten(Rule.GetLenCh() - 2))
	{
		KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Rule.Shorten failed");
		return FALSE;
	}

	// 获得缓存地址
	if (pData->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress)
		lpCurrentFileInfo = MmGetSystemAddressForMdlSafe(
		pData->Iopb->Parameters.DirectoryControl.QueryDirectory.MdlAddress,
		NormalPagePriority
		);
	else
		lpCurrentFileInfo = pData->Iopb->Parameters.DirectoryControl.QueryDirectory.DirectoryBuffer;

	if (!lpCurrentFileInfo)
	{
		KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"pCurrentFileInfo is NULL");
		return FALSE;
	}

	//判断FileInformationClass的类型
	FileInfoClass = pData->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
	if (!GetDirInfoOffset(FileInfoClass, &ulNameOffset, &ulNameLengthOffset, &ulFileInfoSize))
	{
		KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"GetDirInfoOffset failed");
		return FALSE;
	}

	lpStartFileInfo = lpCurrentFileInfo;
	lpPreviousFileInfo = lpCurrentFileInfo;

	do 
	{
		// Byte offset of the next FILE_BOTH_DIR_INFORMATION entry
		ulNextOffset = *(PULONG)lpCurrentFileInfo;

		// 后继结点指针 
		lpNextFileInfo = (LPVOID)((ULONG_PTR)lpCurrentFileInfo + ulNextOffset);  

		// 获得当前文件(夹)的名字
		pWcharName = (PWCHAR)new(MEMORY_TAG_DIR_HIDE) CHAR[*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) + sizeof(WCHAR)];
		RtlCopyMemory(pWcharName, (PWCHAR)(((PCHAR)lpCurrentFileInfo) + ulNameOffset), *((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)));

		if (!Name.Set(pWcharName, wcslen(pWcharName)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Name.Set failed");
			bRet = FALSE;
			break;
		}

		// 获得当前文件(夹)的路径
		if (!Path.Set(ParentPath))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Path.Set failed");
			bRet = FALSE;
			break;
		}

		if (*(Path.GetString() + Path.GetLenCh() - 1) != L'\\')
		{
			if (!Path.Append(L"\\", wcslen(L"\\")))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Path.Append failed");
				bRet = FALSE;
				break;
			}
		}

		if (!Path.Append(&Name))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Path.Append failed");
			bRet = FALSE;
			break;
		}

		// 判断需不需要隐藏
		if ((FsRtlIsNameInExpression(RuleEx->Get(), Path.Get(), TRUE, NULL) || Rule.Equal(&Path, TRUE))
			&&
			!Name.Equal(L".", wcslen(L"."), FALSE)
			&&
			!Name.Equal(L"..", wcslen(L".."), FALSE))
		{
			// 真正的隐藏操作

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
			{
				// 更改前驱结点中指向下一结点的偏移量，略过要隐藏的文件的文件结点，达到隐藏目的
				*(PULONG_PTR)lpPreviousFileInfo += ulNextOffset;
			}

			// 修正查询结果
			if (pData->IoStatus.Information > ulFileInfoSize + (*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) - sizeof(WCHAR)))
				pData->IoStatus.Information -= (ulFileInfoSize + (*((PUSHORT)(((PCHAR)lpCurrentFileInfo) + ulNameLengthOffset)) - sizeof(WCHAR)));
		}
		else
			lpPreviousFileInfo = lpCurrentFileInfo;

		// 当前指针后移      
		lpCurrentFileInfo = lpNextFileInfo;

		delete[] pWcharName;
		pWcharName = NULL;
	} while (ulNextOffset);

	if (lpPreviousFileInfo != lpNextFileInfo)
		*(PULONG_PTR)lpPreviousFileInfo = 0;

	delete[] pWcharName;
	pWcharName = NULL;

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
	BOOLEAN bRet = TRUE;


	__try
	{
		if (!pNameOffset || !pNameLengthOffset)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input argument error");
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
			bRet = FALSE;
		} 
	}
	__finally
	{
		;
	}

	return bRet;
}
