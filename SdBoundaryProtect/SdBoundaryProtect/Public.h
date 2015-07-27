/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		Public.h
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

#define MOD_PUBLIC				L"Public"
#define MEMORY_TAG_PUBLIC		'CLBP'			// PBLC


typedef enum _LOG_PRINTF_LEVEL
{
	LOG_PRINTF_LEVEL_INFO,
	LOG_PRINTF_LEVEL_WARNING,
	LOG_PRINTF_LEVEL_ERROR
} LOG_PRINTF_LEVEL, *PLOG_PRINTF_LEVEL, *LPLOG_PRINTF_LEVEL;

typedef enum _LOG_RECORED_LEVEL
{
	LOG_RECORED_LEVEL_NEED,
	LOG_RECORED_LEVEL_NEEDNOT
} LOG_RECORED_LEVEL, *PLOG_RECORED_LEVEL, *LPLOG_RECORED_LEVEL;

#define KdPrintKrnl(PrintfLevel, RecoredLevel, FMT, ...) PrintKrnl(PrintfLevel, RecoredLevel, __FUNCTION__, FMT, __VA_ARGS__)

#define _ENABLE_DEVELOPER_TEST_
// #define _ENABLE_PROC_PROTECT_

extern "C"
	UCHAR*
	PsGetProcessImageFileName(
	__in PEPROCESS pEprocess
	);

VOID
	PrintKrnl(
	__in							LOG_PRINTF_LEVEL	PrintfLevel,
	__in							LOG_RECORED_LEVEL	RecoredLevel,
	__in							PCHAR				pFuncName,
	__in __drv_formatString(printf)	PWCHAR				Fmt,
	...
	);

LPVOID 
	__cdecl  operator
	new(
	size_t	size,
	ULONG	ulPoolTag
	);

VOID 
	__cdecl operator
	delete(
	LPVOID lpPointer
	);

VOID 
	__cdecl operator
	delete[](
	LPVOID lpPointer
	);
