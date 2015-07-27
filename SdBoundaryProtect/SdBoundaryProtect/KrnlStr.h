/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		KrnlStr.h
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

#define MOD_KRNLSTR			L"字符串"
#define MEMORY_TAG_KRNLSTR	'TSNK'		// KNST

/*
* UNICODE_STRING封装类
* 
* 备注：
*
*/
class CKrnlStr
{
public:
	CKrnlStr();

	~CKrnlStr();

	/*
	* 函数说明：
	*		初始化一个字符串
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TURE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Init();

	/*
	* 函数说明：
	*		设置m_Str.Buffer的数据
	*
	* 参数：
	*		pStr	要设置的数据内容
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Set(
		__in PWCHAR pWchStr,
		__in USHORT usLenCh
		);

	/*
	* 函数说明：
	*		设置m_Str.Buffer的数据
	*
	* 参数：
	*		pStr	要设置的数据内容
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Set(
		__in PCHAR	pChStr,
		__in USHORT usLenCh
		);

	/*
	* 函数说明：
	*		设置m_Str的数据
	*
	* 参数：
	*		pUnicodeStr	要设置的数据内容
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Set(
		__in PUNICODE_STRING pUnicodeStr
		);

	/*
	* 函数说明：
	*		拷贝一份副本
	*
	* 参数：
	*		pStr	
	*
	* 返回值：
	*		无
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Set(
		__in CKrnlStr * pCKrnlStr
		);

	/*
	* 函数说明：
	*		获得m_Str.Length字符长度
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		m_Str.Length字符长度
	*
	* 备注：
	*		无
	*/
	USHORT
		GetLenCh();

	/*
	* 函数说明：
	*		获得m_Str.Length字节长度
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		m_Str.Length字节长度
	*
	* 备注：
	*		无
	*/
	USHORT
		GetLenB();

	/*
	* 函数说明：
	*		获得m_Str.Buffer
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		m_Str.Buffer
	*
	* 备注：
	*		无
	*/
	PWCHAR
		GetString();

	/*
	* 函数说明：
	*		获得m_Str
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		m_Str
	*
	* 备注：
	*		无
	*/
	PUNICODE_STRING
		Get();

	/*
	* 函数说明：
	*		清空字符串的内容，不释放Buffer
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Clean();

	/*
	* 函数说明：
	*		判断是否相等
	*
	* 参数：
	*		pStr
	*		IsIgnoreCase 是否忽略大小写
	*
	* 返回值：
	*		TRUE	相等
	*		FALSE	不相等
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Equal(
		__in CKrnlStr *	pCKrnlStr,
		__in BOOLEAN	bIgnoreCase
		);

	/*
	* 函数说明：
	*		判断是否相等
	*
	* 参数：
	*		pWstr
	*		IsIgnoreCase 是否忽略大小写
	*
	* 返回值：
	*		TRUE	相等
	*		FALSE	不相等
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Equal(
		__in PWCHAR		pWchStr,
		__in USHORT		usLenCh,
		__in BOOLEAN	bIgnoreCase
		);

	/*
	* 函数说明：
	*		判断是否相等
	*
	* 参数：
	*		pUstr
	*		IsIgnoreCase 是否忽略大小写
	*
	* 返回值：
	*		TRUE	相等
	*		FALSE	不相等
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Equal(
		__in PCHAR		pChStr,
		__in USHORT		usLenCh,
		__in BOOLEAN	bIgnoreCase
		);

	/*
	* 函数说明：
	*		判断是否相等
	*
	* 参数：
	*		pUnicodeStr
	*		IsIgnoreCase 是否忽略大小写
	*
	* 返回值：
	*		TRUE	相等
	*		FALSE	不相等
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Equal(
		__in PUNICODE_STRING	pUnicodeStr,
		__in BOOLEAN			bIgnoreCase
		);

	/*
	* 函数说明：
	*		连接
	*
	* 参数：
	*		pUstr
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Append(
		__in PCHAR	pChStr,
		__in USHORT	usLenCh
		);

	/*
	* 函数说明：
	*		连接
	*
	* 参数：
	*		pWstr
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Append(
		__in PWCHAR pWchStr,
		__in USHORT	usLenCh
		);

	/*
	* 函数说明：
	*		连接
	*
	* 参数：
	*		pUnicodeStr
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Append(
		__in PUNICODE_STRING pUnicodeStr
		);

	/*
	* 函数说明：
	*		连接
	*
	* 参数：
	*		pStr
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Append(
		__in CKrnlStr * pCKrnlStr
		);

	/*
	* 函数说明：
	*		获得字符串开始位置的指定长度字符串
	*
	* 参数：
	*		ulLen	要获得的长度(字节数)
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		开始不判断len，有可能就是要获得长度0
	*/
	BOOLEAN
		Shorten(
		__in USHORT usLenCh
		);

	/*
	* 函数说明：
	*		扩展字符串长度
	*
	* 参数：
	*		ulLen	要扩展到的长度(字节数)
	*
	* 返回值：
	*		TRUE	成功
	*		FALSE	失败
	*
	* 备注：
	*		开始不判断len，有可能就是要扩展至长度0
	*/
	BOOLEAN
		Lengthen(
		__in USHORT usLenCh
		);

	/*
	* 函数说明：
	*		转大写
	*
	* 参数：
	*		无
	*
	* 返回值：
	*		&m_Str	转为大写的UNICODE_STRING字符串
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		ToUpper();

	/*
	* 函数说明：
	*		在一个字符串中查找一个字符
	*
	* 参数：
	*		wch			要查找的字符
	*		pWchBegin	查找开始位置
	*		pWchEnd		查找结束位置
	*
	* 返回值：
	*		查找字符所在位置
	*
	* 备注：
	*		无
	*/
	PWCHAR
		SearchCharacter(
		__in WCHAR	wch,
		__in PWCHAR	pWchBegin,
		__in PWCHAR	pWchEnd
		);

private:
	UNICODE_STRING m_Str;

	/*
	* 函数说明：
	*		分配
	*
	* 参数：
	*		ulLen		要分配的长度(字符数)
	*
	* 返回值：
	*		TRUE	成功	
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		Alloc(
		__in USHORT usLenCh
		);

	/*
	* 函数说明：
	*		释放
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
	VOID
		Free();

	/*
	* 函数说明：
	*		比较宽字符串是否相等
	*
	* 参数：
	*		pwchPosition1	第一字符串指针
	*		pwchPosition2	第二个字符串指针
	*		ulLen			要比较的长度
	*		bIgnoreCase		是否忽略大小写
	*
	* 返回值：
	*		TURE	成功
	*		FALSE	失败
	*
	* 备注：
	*		无
	*/
	BOOLEAN
		WcharStrEqual(
		__in PWCHAR		pWchPosition1,
		__in USHORT		usLenCh1,
		__in PWCHAR		pWchPosition2,
		__in USHORT		usLenCh2,
		__in USHORT		usLenChCmp,
		__in BOOLEAN	bIgnoreCase
		);
};
