/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		ProcWhiteList.h
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

#define MOD_PROC_WHITE_LIST			L"进程白名单"
#define MEMORY_TAG_PROC_WHILTE_LIST	'LWCP'			// PCWL

typedef struct _PROC_WHITE_LIST
{
	ULONG		ulPid;	//进程ID

	LIST_ENTRY  List; 
} PROC_WHITE_LIST, *PPROC_WHITE_LIST, *LPPROC_WHITE_LIST;

class CProcWhiteList
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
	CProcWhiteList();

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
	~CProcWhiteList();

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
	*		判断指定进程是否在白名单中
	*
	* Arguments:
	*
	*		ulPid - 进程id
	*
	* Return Value:
	*
	*		TRUE	- 在
	*		FALSE	- 不在
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
		__in ULONG ulPid
		);

private:
	static LIST_ENTRY		ms_ListHead;
	static ERESOURCE		ms_Lock;
	static KSPIN_LOCK		ms_SpLock;

	KIRQL					m_Irql;
	LONG					m_LockRef;

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
	*		删除指定进程的PROC_WHITE_LIST结构
	*
	* Arguments:
	*
	*		ulPid - 进程id
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
		__in ULONG ulPid
		);

	/*++
	*
	* Routine Description:
	*
	*		插入指定进程的PROC_WHITE_LIST结构
	*
	* Arguments:
	*
	*		ulPid - 进程id
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
		__in ULONG ulPid
		);

	/*++
	*
	* Routine Description:
	*
	*		获得指定进程的PROC_WHITE_LIST结构
	*
	* Arguments:
	*
	*		ulPid - 进程id
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
	LPPROC_WHITE_LIST
		Get(
		__in ULONG ulPid
		);



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
};
