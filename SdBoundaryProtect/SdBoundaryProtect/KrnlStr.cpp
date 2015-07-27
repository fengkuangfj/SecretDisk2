/*++
*
* Copyright (c) 2015 - 2016  北京凯锐立德科技有限公司
*
* Module Name:
*
*		KrnlStr.cpp
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
#include "KrnlStr.h"


CKrnlStr::CKrnlStr()
{
	RtlZeroMemory(&m_Str, sizeof(m_Str));
}

CKrnlStr::~CKrnlStr()
{
	Free();
}

BOOLEAN
	CKrnlStr::Alloc(
	__in USHORT usLenCh
	)
{
	BOOLEAN bRet	= FALSE;

	USHORT	usLenB	= 0;


	__try
	{
		if (!usLenCh)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		usLenB = usLenCh * sizeof(WCHAR);
		if (usLenB > m_Str.MaximumLength)
		{
			Free();

			m_Str.MaximumLength = usLenB;
			m_Str.Buffer = (PWCHAR)new(KRNLSTR_TAG) CHAR[m_Str.MaximumLength];
		}
		else
			Clean();

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

VOID
	CKrnlStr::Free()
{
	if (m_Str.Buffer)
	{
		delete[] m_Str.Buffer;
		m_Str.Buffer = NULL;
	}

	m_Str.Length = 0;
	m_Str.MaximumLength = 0;
}

BOOLEAN 
	CKrnlStr::Set(
	__in CKrnlStr * pCKrnlStr 
	)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pCKrnlStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		bRet = Set(pCKrnlStr->Get());
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::ToUpper()
{
	BOOLEAN bRet			= FALSE;

	PWCHAR	pWchPositionCur	= NULL;
	PWCHAR	pWchPositionEnd	= NULL;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"buffer not alloc");
			__leave;
		}

		pWchPositionCur = m_Str.Buffer;
		pWchPositionEnd = m_Str.Buffer + (m_Str.Length / sizeof(WCHAR));
		for (; pWchPositionCur < pWchPositionEnd; pWchPositionCur++)
		{
			if ((L'a' <= *pWchPositionCur) && (L'z' >= *pWchPositionCur))
				*pWchPositionCur = *pWchPositionCur - (L'a' - L'A');
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN 
	CKrnlStr::Set(
	__in PWCHAR pWchStr,
	__in USHORT usLenCh
	)
{
	BOOLEAN	bRet		= FALSE;

	USHORT	usLenBNew	= 0;
	USHORT	usLenChNew	= 0;


	__try
	{
		if (!pWchStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (pWchStr == m_Str.Buffer)
		{
			bRet = TRUE;
			__leave;
		}

		usLenChNew = usLenCh;
		usLenBNew = usLenChNew * sizeof(WCHAR);
		if (usLenBNew >= m_Str.MaximumLength)
		{
			if (!Alloc(usLenChNew + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}
		}
		else
			RtlZeroMemory(m_Str.Buffer, m_Str.Length);

		m_Str.Length = usLenBNew;
		RtlCopyMemory(m_Str.Buffer, pWchStr, m_Str.Length);

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN 
	CKrnlStr::Set(
	__in PCHAR	pChStr,
	__in USHORT usLenCh
	)
{
	BOOLEAN bRet		= FALSE;

	USHORT	i			= 0;
	USHORT	usLenChNew	= 0;
	USHORT	usLenbNew	= 0;


	__try
	{
		if (!pChStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if ((PWCHAR)pChStr == m_Str.Buffer)
			__leave;

		usLenChNew = usLenCh;
		usLenbNew = usLenChNew * sizeof(WCHAR);
		if (usLenbNew >= m_Str.MaximumLength)
		{
			if (!Alloc(usLenChNew + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}
		}
		else
			RtlZeroMemory(m_Str.Buffer, m_Str.Length);

		m_Str.Length = usLenbNew;

		for (; i < usLenChNew; i++)
			m_Str.Buffer[i] = (WCHAR)pChStr[i];

		m_Str.Buffer[i] = UNICODE_NULL;

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN 
	CKrnlStr::Set(
	__in PUNICODE_STRING pUnicodeStr
	)
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!pUnicodeStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (pUnicodeStr == &m_Str)
		{
			bRet = TRUE;
			__leave;
		}

		if (!Set(pUnicodeStr->Buffer, pUnicodeStr->Length / sizeof(WCHAR)))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Set failed. Str(%wZ)",
				pUnicodeStr);

			__leave;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

USHORT 
	CKrnlStr::GetLenCh()
{
	return m_Str.Length / sizeof(WCHAR);
}

USHORT 
	CKrnlStr::GetLenB()
{
	return m_Str.Length;
}

PWCHAR 
	CKrnlStr::GetString()
{
	return m_Str.Buffer;
}

PUNICODE_STRING
	CKrnlStr::Get()
{
	return &m_Str;
}

BOOLEAN
	CKrnlStr::Equal(
	__in CKrnlStr *	pCKrnlStr,
	__in BOOLEAN	bIgnoreCase
	)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pCKrnlStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		bRet = Equal(pCKrnlStr->Get(), bIgnoreCase);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Equal(
	__in PWCHAR		pWchStr,
	__in USHORT		usLenCh,
	__in BOOLEAN	bIgnoreCase
	)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"buffer not alloc");
			__leave;
		}

		if (!pWchStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (m_Str.Length != (usLenCh * sizeof(WCHAR)))
			__leave;

		bRet = WcharStrEqual(
			m_Str.Buffer,
			usLenCh,
			pWchStr,
			usLenCh,
			usLenCh,
			bIgnoreCase
			);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Equal(
	__in PCHAR		pChStr,
	__in USHORT		usLenCh,
	__in BOOLEAN	bIgnoreCase
	)
{
	BOOLEAN		bRet		= FALSE;

	CKrnlStr	KrnlStr;


	__try
	{
		if (!pChStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (!KrnlStr.Set(pChStr, usLenCh))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"KrnlStr.Set failed. Str(%hs)",
				pChStr);

			__leave;
		}

		bRet = Equal(&KrnlStr, bIgnoreCase);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Equal(
	__in PUNICODE_STRING	pUnicodeStr,
	__in BOOLEAN			bIgnoreCase
	)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pUnicodeStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"buffer not alloc");
			__leave;
		}

		bRet = Equal(pUnicodeStr->Buffer, pUnicodeStr->Length / sizeof(WCHAR), bIgnoreCase);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Append(
	__in CKrnlStr * pCKrnlStr
	)
{
	BOOLEAN	bRet = FALSE;


	__try
	{
		if (!pCKrnlStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		bRet = Append(pCKrnlStr->GetString(), pCKrnlStr->GetLenCh());
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Append(
	__in PCHAR	pChStr,
	__in USHORT	usLenCh
	)
{
	BOOLEAN		bRet		= FALSE;

	CKrnlStr	StrTmp;


	__try
	{
		if (!pChStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		if (!StrTmp.Set(pChStr, usLenCh))
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"StrTmp.Set failed. Str(%hs)",
				pChStr);

			__leave;
		}

		bRet = Append(&StrTmp);
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Append(
	__in PWCHAR pWchStr,
	__in USHORT	usLenCh
	)
{
	BOOLEAN		bRet		= FALSE;

	USHORT		usLenBPre	= 0;
	USHORT		usLenBPost	= 0;
	USHORT		usLenBNew	= 0;

	CKrnlStr	StrTemp;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"buffer not alloc");
			__leave;
		}

		if (!pWchStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		usLenBPre = m_Str.Length;
		usLenBPost = usLenCh * sizeof(WCHAR);
		usLenBNew = usLenBPre + usLenBPost;

		if (usLenBNew >= m_Str.MaximumLength)
		{
			if (!StrTemp.Set(&m_Str))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"StrTmp.Set failed. Str(%wZ)",
					&m_Str);

				__leave;
			}

			if (!Alloc(usLenBNew / sizeof(WCHAR) + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}

			m_Str.Length = usLenBPre;
			RtlCopyMemory(m_Str.Buffer, StrTemp.GetString(), m_Str.Length);		
		}

		m_Str.Length = usLenBNew;
		RtlCopyMemory(m_Str.Buffer + usLenBPre / sizeof(WCHAR), pWchStr, usLenBPost);

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Append(
	__in PUNICODE_STRING pUnicodeStr
	)
{
	BOOLEAN	 bRet = FALSE;


	__try
	{
		if (!pUnicodeStr)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error");
			__leave;
		}

		bRet = Append(pUnicodeStr->Buffer, pUnicodeStr->Length / sizeof(WCHAR));
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Shorten(
	__in USHORT usLenCh
	)
{
	BOOLEAN bRet	= FALSE;

	USHORT	usLenB	= 0;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"buffer not alloc");
			__leave;
		}

		usLenB = usLenCh * sizeof(WCHAR);

		if (usLenB > m_Str.Length)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"ulLen > m_Str.ulLen. you are lengthen");
			__leave;
		}

		if (usLenB < m_Str.Length)
		{
			RtlZeroMemory(m_Str.Buffer + usLenCh, m_Str.Length - usLenB);
			m_Str.Length = usLenB;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Lengthen(
	__in USHORT usLenCh
	)
{
	BOOLEAN		bRet	= FALSE;

	USHORT		usLenB	= 0;

	CKrnlStr	StrTemp;


	__try
	{
		usLenB = usLenCh * sizeof(WCHAR);

		if (usLenB < m_Str.Length)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"usLenB < m_Str.Length. you are shorten");
			__leave;
		}

		if (usLenB < m_Str.MaximumLength)
			m_Str.Length = usLenB;
		else if (usLenB > m_Str.MaximumLength)
		{
			if (m_Str.Length)
			{
				if (!StrTemp.Set(&m_Str))
				{
					KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"StrTemp.Set failed. Str(%wZ)",
						&m_Str);

					__leave;
				}
			}

			if (!Alloc(usLenCh + 1))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}

			if (StrTemp.GetLenB())
				RtlCopyMemory(m_Str.Buffer, StrTemp.GetString(), StrTemp.GetLenB());

			m_Str.Length = usLenB;
		}

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Clean()
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!m_Str.Buffer)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"buffer not alloc");
			__leave;
		}

		RtlZeroMemory(m_Str.Buffer, m_Str.MaximumLength);
		m_Str.Length = 0; 

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

BOOLEAN
	CKrnlStr::Init()
{
	BOOLEAN bRet = FALSE;


	__try
	{
		if (!m_Str.Buffer)
		{
			if (!Alloc(MAX_PATH))
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Alloc failed");
				__leave;
			}
		}
		else 
		{
			if (!Clean())
			{
				KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"Clean failed");
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

BOOLEAN
	CKrnlStr::WcharStrEqual(
	__in PWCHAR		pWchPosition1,
	__in USHORT		usLenCh1,
	__in PWCHAR		pWchPosition2,
	__in USHORT		usLenCh2,
	__in USHORT		usLenChCmp,
	__in BOOLEAN	bIgnoreCase
	)
{
	BOOLEAN	bRet			= FALSE;

	USHORT	i				= 0;
	USHORT	usLenChTemp		= 0;
	PWCHAR	pWchTemp		= NULL;
	USHORT	usDifference	= L'a' - L'A';


	__try
	{
		if (!pWchPosition1 || !pWchPosition2 || !usLenChCmp)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pWchPosition1(%p) pWchPosition2(%p) usLenB(%d)",
				pWchPosition1, pWchPosition2, usLenChCmp);

			__leave;
		}

		if (usLenChCmp > usLenCh1 && usLenChCmp > usLenCh2)
			__leave;

		if (usLenCh1 > usLenCh2)
		{
			pWchTemp = pWchPosition1;
			pWchPosition1 = pWchPosition2;
			pWchPosition2 = pWchTemp;

			usLenChTemp = usLenCh1;
			usLenCh1 = usLenCh2;
			usLenCh2 = usLenChTemp;
		}

		if (usLenChCmp > usLenCh1 && usLenChCmp <= usLenCh2)
			__leave;

		for (; i < usLenChCmp; i++, pWchPosition1++, pWchPosition2++)
		{
			if (*pWchPosition1 != *pWchPosition2)
			{
				if ((L'A' <= *pWchPosition1) && (L'Z'>= *pWchPosition1))
				{
					if (bIgnoreCase)
					{
						if ((*pWchPosition1 + usDifference) == *pWchPosition2)
							continue;
					}
				}
				else if ((L'a' <= *pWchPosition1) && (L'z'>= *pWchPosition1))
				{
					if (bIgnoreCase)
					{
						if ((*pWchPosition1 - usDifference) == *pWchPosition2)
							continue;
					}
				}

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

PWCHAR
	CKrnlStr::SearchCharacter(
	__in WCHAR	wch,
	__in PWCHAR	pWchBegin,
	__in PWCHAR	pWchEnd
	)
{
	PWCHAR	pRet			= NULL;

	USHORT	i				= 0;
	PWCHAR	pWchPositionCur = NULL;


	__try
	{
		if (!pWchBegin || !pWchEnd)
		{
			KdPrintKrnl(LOG_PRINTF_LEVEL_ERROR, LOG_RECORED_LEVEL_NEED, L"input parameter error. pBegin(%p) pEnd(%p)",
				pWchBegin, pWchEnd);

			__leave;
		}

		if (pWchBegin < pWchEnd)
		{
			for (pWchPositionCur = pWchBegin; pWchPositionCur < pWchEnd; pWchPositionCur++)
			{
				if (wch == *pWchPositionCur)
				{
					pRet = pWchPositionCur;
					break;
				}
			}
		}
		else
		{
			for (pWchPositionCur = pWchBegin; pWchPositionCur > pWchEnd; pWchPositionCur--)
			{
				if (wch == *pWchPositionCur)
				{
					pRet = pWchPositionCur;
					break;
				}
			}
		}
	}
	__finally
	{
		;
	}

	return pRet;
}
