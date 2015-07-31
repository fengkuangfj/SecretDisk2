/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		FileName.h
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

#define MOD_FILE_NAME			L"文件名"
#define MEMORY_TAG_FILE_NAME	'MNLF'		// FLNM

typedef enum _NAME_TYPE
{
	NAME_TYPE_NULL		= 0x00000000,
	TYPE_APP			= 0x00000001,
	TYPE_SYM			= 0x00000002,
	TYPE_DEV			= 0x00000004,
	TYPE_UNKNOW			= 0x00000007,
	TYPE_FULL_PATH		= 0x00000008
} NAME_TYPE, *PNAME_TYPE, *LPNAME_TYPE;

typedef struct _VOLUME_NAME_INFO
{
	CKrnlStr		AppName;		// like "C:"
	CKrnlStr		SymName;		// like "\\??\\C:"
	CKrnlStr		DevName;		// like "\\Device\\HarddiskVolume12" consider volumes more than 10

	PFLT_INSTANCE	pFltInstance;
	ULONG			ulSectorSize;
	BOOLEAN			bRemoveable;
	BOOLEAN			bOnlyDevName;

	LIST_ENTRY		List;			// struct EntryPointer
} VOLUME_NAME_INFO, *PVOLUME_NAME_INFO, *LPVOLUME_NAME_INFO;

class CFileName
{
public:
	CFileName();

	~CFileName();

	VOID
		Init();

	BOOLEAN
		Unload();

	static
		BOOLEAN
		ToDev(
		__in	CKrnlStr * pName,
		__inout	CKrnlStr * pDevName
		);

	static
		BOOLEAN
		ToApp(
		__in	CKrnlStr * pName,
		__inout CKrnlStr * pAppName
		);

	/*
	* 函数说明：
	*		获取文件名
	*
	* 参数：
	*		pData
	*		pFltVol	
	*		pName		文件名
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*
	* 最后修改时间：
	*		2015/7/3-16:25
	*/
	static
		BOOLEAN
		GetFileFullPath(
		__in	PFLT_CALLBACK_DATA		pData,
		__in	PFLT_VOLUME				pFltVol,
		__out	CKrnlStr			*	pDevName
		);

	/*
	* 函数说明：
	*		获得父路径
	*
	* 参数：
	*		pPath			文件路径
	*		pParentPath		文件父路径
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*
	* 最后修改时间：
	*		2015/7/3-16:25
	*/
	static
		BOOLEAN
		GetParentPath(
		__in	CKrnlStr * pPath,
		__out	CKrnlStr * pParentPath
		);

	/*
	* 函数说明：
	*		从PFLT_VOLUME中获取卷的设备名
	*
	* 参数：
	*		pFltVol
	*		pName		卷的设备名
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*
	* 最后修改时间：
	*		2015/7/3-16:29
	*/
	static
		BOOLEAN
		GetVolDevNameFromFltVol(
		__in	PFLT_VOLUME		pFltVol,
		__out	CKrnlStr	*	pDevName
		);

	/*++
	*
	* Routine Description:
	*
	*		插入一条VOLUME_NAME_INFO
	*
	* Arguments:
	*
	*		pAppName
	*
	*		pSymName
	*
	*		pDevName
	*
	*		bOnlyDevName
	*
	*		bRemoveable
	*
	*		pFltInstance
	*
	*		ulSectorSize
	*
	* Return Value:
	*
	*		TRUE	- 成功
	*		FALSE	- 失败
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
	BOOLEAN
		InsertVolNameInfo(
		__in_opt	CKrnlStr		*	pAppName,
		__in_opt	CKrnlStr		*	pSymName,
		__in		CKrnlStr		*	pDevName,
		__in		BOOLEAN				bOnlyDevName,
		__in		BOOLEAN				bRemoveable,
		__in		PFLT_INSTANCE		pFltInstance,
		__in		ULONG				ulSectorSize
		);

	BOOLEAN
		DelVolNameInfo(
		__in CKrnlStr * pDevName
		);

	static
		BOOLEAN
		SpliceFilePath(
		__in	CKrnlStr * pDirPath,
		__in	CKrnlStr * pFileName,
		__inout	CKrnlStr * pFilePath
		);

	LPVOLUME_NAME_INFO
		GetVolNameInfo(
		__in CKrnlStr	*	pName,
		__in NAME_TYPE		NameType
		);

	/*
	* 函数说明：
	*		判断对象是否是卷
	*
	* 参数：
	*		pData
	*		pFileName	对象名
	*
	* 返回值：
	*		TRUE	是卷
	*		FALSE	不是卷
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		IsVolume(
		__in PFLT_CALLBACK_DATA		pData,
		__in CKrnlStr			*	pDevName
		);

	/*
	* 函数说明：
	*		根据文件句柄获取文件路径
	*
	* 参数：
	*		pData
	*		hFile		文件句柄
	*		pFileName	文件路径
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
		GetPathByHandle(
		__in	PFLT_CALLBACK_DATA		pData,
		__in	PCFLT_RELATED_OBJECTS	pFltObjects,
		__in	HANDLE					hFile,
		__out	CKrnlStr			*	pDevName
		);

	static
		BOOLEAN
		IsExpression(
		__in CKrnlStr * pFileName
		);

	static
		BOOLEAN
		ParseAppOrSymName(
		__in	CKrnlStr	*	pName,
		__inout CKrnlStr	*	pVolName,
		__inout CKrnlStr	*	pPartName,
		__inout PBOOLEAN		pbDisk,
		__inout PNAME_TYPE		pNameType
		);

	/*++
	*
	* Routine Description:
	*
	*		是否是标准卷解挂
	*
	* Arguments:
	*
	*		pVolDevName
	*
	*		pFltInstance
	*
	* Return Value:
	*
	*		TRUE	- 是
	*		FALSE	- 不是
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
	BOOLEAN
		IsDisMountStandard(
		__in CKrnlStr		*	pVolDevName,
		__in PFLT_INSTANCE		pFltInstance
		);

	static
		BOOLEAN
		GetVolAppNameByQueryObj(
		__in	CKrnlStr *	pName,
		__inout CKrnlStr *	pAppName,
		__inout PUSHORT		pusCutOffset
		);


	BOOLEAN
		GetFltInstance(
		__in	CKrnlStr		*	pFileName,
		__out	PFLT_INSTANCE	*	pPFltInstance,
		__in	NAME_TYPE			NameType
		);

	BOOLEAN
		GetSectorSize(
		__in	CKrnlStr	*	pDevName,
		__inout ULONG		*	pUlSectorSize
		);

private:
	static LIST_ENTRY	ms_ListHead;	
	static ERESOURCE	ms_Lock;
	static KSPIN_LOCK	ms_SpLock;

	KIRQL				m_Irql;
	LONG				m_LockRef;

	VOID 
		GetLock();

	VOID 
		FreeLock();

	/*
	* 函数说明：
	*		从PFLT_CALLBACK_DATA和PFLT_VOLUME中将文件名拼接出来
	*
	* 参数：
	*		pData
	*		pFltVol	
	*		pName		文件名	
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*
	* 最后修改时间：
	*		2015/7/3-16:27
	*/
	static
		BOOLEAN
		GetFileFullPathFromDataAndFltVol(
		__in	PFLT_CALLBACK_DATA		pData,
		__in	PFLT_VOLUME				pFltVol,
		__out	CKrnlStr			*	pName
		);

	LPVOLUME_NAME_INFO
		GetVolNameInfoByVolAppName(
		__in CKrnlStr * pAppName
		);

	LPVOLUME_NAME_INFO
		GetVolNameInfoByVolSymName(
		__in CKrnlStr * pSymName
		);

	LPVOLUME_NAME_INFO
		GetVolNameInfoByVolDevName(
		__in CKrnlStr * pDevName
		);

	/*
	* 函数说明：
	*		获得卷的设备名
	*
	* 参数：
	*		pName		卷的符号链接名
	*		pDevName	卷的设备名
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
		GetVolDevNameByQueryObj(
		__in	CKrnlStr * pName,
		__out	CKrnlStr * pDevName
		);

};
