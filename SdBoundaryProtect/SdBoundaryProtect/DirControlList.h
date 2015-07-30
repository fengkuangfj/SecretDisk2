/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		DirControlList.h
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

#define MOD_DIR_CONTROL_LIST		L"目录控制列表"
#define MEMORY_TAG_DIR_CONTROL_LIST	'LCTD'								// DTCL
#define OFFSETOF(TYPE, MEMBER)		((size_t) & ((TYPE *)0)->MEMBER)

typedef struct _DIR_CONTROL_LIST
{
	DIR_CONTROL_TYPE	Type;				// 保护类型

	CKrnlStr			RuleEx;				// 规则路径表达式
	CKrnlStr			ParentDirRuleEx;	// 规则路径父目录规则表达式

	LIST_ENTRY			List;
} DIR_CONTROL_LIST, *PDIR_CONTROL_LIST, *LPDIR_CONTROL_LIST;

typedef struct _REGISTER_DIR_INFO
{
	DIR_CONTROL_TYPE	Type;			// 保护类型

	CKrnlStr			FileName;
} REGISTER_DIR_INFO, *PREGISTER_DIR_INFO, *LPREGISTER_DIR_INFO;

class CDirControlList
{
public:
	/*++
	*
	* Routine Description:
	*
	*		构造
	*
	* Arguments:
	*
	*		无
	*
	* Return Value:
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
	CDirControlList();

	/*++
	*
	* Routine Description:
	*
	*		析构
	*
	* Arguments:
	*
	*		无
	*
	* Return Value:
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
	~CDirControlList();

	/*++
	*
	* Routine Description:
	*
	*		初始化模块
	*
	* Arguments:
	*
	*		无
	*
	* Return Value:
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
	VOID
		Init();

	/*++
	*
	* Routine Description:
	*
	*		卸载模块
	*
	* Arguments:
	*
	*		无
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
		Unload();

	/*++
	*
	* Routine Description:
	*
	*		查询指定文件是否命中规则
	*
	* Arguments:
	*
	*		FileName - 规则表达式
	*
	* Return Value:
	*
	*		TRUE	- 命中
	*		FALSE	- 未命中
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
		IsIn(
		__in CKrnlStr			*	pFileName,
		__in DIR_CONTROL_TYPE		DirControlType
		);

	/*++
	*
	* Routine Description:
	*
	*		隐藏文件文件夹
	*
	*		1、基于断链
	*		2、断的是挂在"要隐藏的文件或文件夹的父目录"下的链
	*		3、这里判断的是文件夹FileName下有没有要隐藏的文件文件夹，如果有，则隐藏
	*
	* Arguments:
	*
	*		FileName	- 规则表达式
	*		pData		- 
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
		Filter(
		__in	CKrnlStr			*	pFileName,
		__inout PFLT_CALLBACK_DATA		pData
		);

	/*++
	*
	* Routine Description:
	*
	*		插入一条规则
	*
	* Arguments:
	*
	*		lpRegisterDirInfo - 要添加的文件夹信息
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
		Insert(
		__in LPREGISTER_DIR_INFO lpRegisterDirInfo
		);

	/*++
	*
	* Routine Description:
	*
	*		清理模块
	*
	* Arguments:
	*
	*		无
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
		Clear();

	/*++
	*
	* Routine Description:
	*
	*		删除一条规则
	*
	* Arguments:
	*
	*		RuleEx - 规则表达式
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
		Delete(
		__in CKrnlStr* pRule
		);

	ULONG
		GetCount();

	BOOLEAN
		Fill(
		__in LPCOMM_INFO	lpCommInfo,
		__in ULONG			ulCount
		);

private:
	static LIST_ENTRY	ms_ListHead;
	static ERESOURCE	ms_Lock;
	static KSPIN_LOCK	ms_SpLock;
	static ULONG		ms_ulCount;

	KIRQL				m_Irql;
	LONG				m_LockRef;

	/*++
	*
	* Routine Description:
	*
	*		获得锁
	*
	* Arguments:
	*
	*		无
	*
	* Return Value:
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
	VOID
		GetLock();

	/*++
	*
	* Routine Description:
	*
	*		释放锁
	*
	* Arguments:
	*
	*		无
	*
	* Return Value:
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
	VOID
		FreeLock();

	/*++
	*
	* Routine Description:
	*
	*		获得一条规则
	*
	* Arguments:
	*
	*		RuleEx - 规则表达式
	*
	* Return Value:
	*
	*		!NULL	- 成功
	*		NULL	- 失败
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
	LPDIR_CONTROL_LIST
		Get(
		__in CKrnlStr* pRule
		);
};
