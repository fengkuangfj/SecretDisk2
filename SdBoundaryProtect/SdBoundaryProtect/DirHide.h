/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		DirHide.h
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

#pragma once

#define MOD_DIR_HIDE		L"目录隐藏"
#define MEMORY_TAG_DIR_HIDE	'DHTD'		// DTHD

class CDirHide
{
public:
	static
		BOOLEAN 
		BreakLink(
		__inout PFLT_CALLBACK_DATA	pData,
		__in	CKrnlStr*			RuleEx,
		__in	CKrnlStr*			ParentPath
		);

private:
	static
		BOOLEAN
		GetDirInfoOffset(
		__in	FILE_INFORMATION_CLASS	FileInfoClass,
		__inout	PULONG_PTR				pNameOffset,
		__inout	PULONG_PTR				pNameLengthOffset,
		__inout	PULONG_PTR				pFileInfoSize
		);
};
