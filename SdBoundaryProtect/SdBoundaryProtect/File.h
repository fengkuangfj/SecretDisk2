/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		File.h
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

#define MOD_FILE		L"文件"
#define MEMORY_TAG_FILE	'ELIF'		// FILE

typedef enum _FILE_OBJECT_TYPE
{
	OBJECT_TYPE_NULL,
	OBJECT_TYPE_FILE,
	OBJECT_TYPE_DIR,
	OBJECT_TYPE_VOLUME,
	OBJECT_TYPE_UNKNOWN
} FILE_OBJECT_TYPE, *PFILE_OBJECT_TYPE, *LPFILE_OBJECT_TYPE;

class CFile
{
public:
	/*
	* 函数说明：
	*		获取对象类型
	*
	* 参数：
	*		pData
	*		pFileName
	*		bRelation	pData和pFileName是否有关联
	*
	* 返回值：
	*		ObjType
	*
	* 备注：
	*		无
	*/
	static
		FILE_OBJECT_TYPE
		GetObjType(
		__in PFLT_CALLBACK_DATA		pData,
		__in CKrnlStr			*	pFileName,
		__in BOOLEAN				bRelation
		);

private:
	CFile();

	~CFile();

	/*
	* 函数说明：
	*		使用pData中的参数Create文件pFileName
	*
	* 参数：
	*		pData	
	*		pFileName
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	static
		BOOLEAN
		CreateUseOigInfo(
		__in		PFLT_CALLBACK_DATA		pData,
		__in		CKrnlStr			*	pFileName,
		__out_opt	FILE_OBJECT_TYPE			*	ObjType		= NULL
		);

	/*
	* 函数说明：
	*		删除文件
	*
	* 参数：
	*		pData	
	*		pFileName		文件名
	*		bDelTag			删除标记
	*		bUserOrgFileObj	pData和pFileName是否有关联
	*		ObjType			对象类型
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	static
		BOOLEAN
		DelFile(
		__in PFLT_CALLBACK_DATA		pData,
		__in CKrnlStr			*	pFileName,
		__in BOOLEAN				bDelTag,
		__in BOOLEAN				bRelation,
		__in FILE_OBJECT_TYPE			ObjType		
		);

	static
		BOOLEAN
		GetFileAttributes(
		__in PFLT_CALLBACK_DATA	pData,
		__in CKrnlStr*			pFileName,
		__in PULONG				pFileAttributes
		);

	static
		BOOLEAN
		SetFileAttributes(
		__in PFLT_CALLBACK_DATA	pData,
		__in CKrnlStr*			pFileName,
		__in ULONG				ulFileAttributes
		);
};
