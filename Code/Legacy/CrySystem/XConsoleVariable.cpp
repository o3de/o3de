/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : implementation of the CXConsoleVariable class.

#include "CrySystem_precompiled.h"
#include "XConsole.h"
#include "XConsoleVariable.h"

#include <IConsole.h>
#include <ISystem.h>

#include <algorithm>

namespace
{
    using stack_string = AZStd::fixed_string<512>;

    uint64 AlphaBit64(char c)
    {
        return (c >= 'a' && c <= 'z' ? 1U << (c - 'z' + 31) : 0) |
               (c >= 'A' && c <= 'Z' ? 1LL << (c - 'Z' + 63) : 0);
    }

    int64 TextToInt64(const char* s, int64 nCurrent, bool bBitfield)
    {
        int64 nValue = 0;
        if (s)
        {
            char* e;
            if (bBitfield)
            {
                // Bit manipulation.
                if (*s == '^')
                // Bit number
#if defined(_MSC_VER)
                {
                    nValue = 1LL << _strtoi64(++s, &e, 10);
                }
#else
                {
                    nValue = 1LL << strtoll(++s, &e, 10);
                }
#endif
                else
                // Full number
#if defined(_MSC_VER)
                {
                    nValue = _strtoi64(s, &e, 10);
                }
#else
                {
                    nValue = strtoll(s, &e, 10);
                }
#endif
                // Check letter codes.
                for (; (*e >= 'a' && *e <= 'z') || (*e >= 'A' && *e <= 'Z'); e++)
                {
                    nValue |= AlphaBit64(*e);
                }

                if (*e == '+')
                {
                    nValue = nCurrent | nValue;
                }
                else if (*e == '-')
                {
                    nValue = nCurrent & ~nValue;
                }
                else if (*e == '^')
                {
                    nValue = nCurrent ^ nValue;
                }
            }
            else
#if defined(_MSC_VER)
            {
                nValue = _strtoi64(s, &e, 10);
            }
#else
            {
                nValue = strtoll(s, &e, 10);
            }
#endif
        }
        return nValue;
    }

    int TextToInt(const char* s, int nCurrent, bool bBitfield)
    {
        return (int)TextToInt64(s, nCurrent, bBitfield);
    }

} // namespace

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXConsoleVariableBase::CXConsoleVariableBase(CXConsole* pConsole, const char* sName, int nFlags, const char* help)
    : m_valueMin(0.0f)
    , m_valueMax(100.0f)
    , m_hasCustomLimits(false)
{
    assert(pConsole);

    m_psHelp = (char*)help;
    m_pChangeFunc = NULL;

    m_pConsole = pConsole;

    m_nFlags = nFlags;

    if (nFlags & VF_COPYNAME)
    {
        m_szName = new char[strlen(sName) + 1];
        azstrcpy(m_szName, strlen(sName) + 1, sName);
    }
    else
    {
        m_szName = (char*)sName;
    }

    if (gEnv->IsDedicated())
    {
        m_pDataProbeString = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
CXConsoleVariableBase::~CXConsoleVariableBase()
{
    if (m_nFlags & VF_COPYNAME)
    {
        delete[] m_szName;
    }

    if (gEnv->IsDedicated() && m_pDataProbeString)
    {
        delete[] m_pDataProbeString;
    }
}

//////////////////////////////////////////////////////////////////////////
void CXConsoleVariableBase::ForceSet(const char* s)
{
    int excludeFlags = (VF_CHEAT | VF_READONLY | VF_NET_SYNCED);
    int oldFlags = (m_nFlags & excludeFlags);
    m_nFlags &= ~(excludeFlags);
    Set(s);
    m_nFlags |= oldFlags;
}

//////////////////////////////////////////////////////////////////////////
void CXConsoleVariableBase::ClearFlags(int flags)
{
    m_nFlags &= ~flags;
}

//////////////////////////////////////////////////////////////////////////
int CXConsoleVariableBase::GetFlags() const
{
    return m_nFlags;
}

//////////////////////////////////////////////////////////////////////////
int CXConsoleVariableBase::SetFlags(int flags)
{
    m_nFlags = flags;
    return m_nFlags;
}

//////////////////////////////////////////////////////////////////////////
const char* CXConsoleVariableBase::GetName() const
{
    return m_szName;
}

//////////////////////////////////////////////////////////////////////////
const char* CXConsoleVariableBase::GetHelp()
{
    return m_psHelp;
}

void CXConsoleVariableBase::Release()
{
    m_pConsole->UnregisterVariable(m_szName);
}

void CXConsoleVariableBase::SetOnChangeCallback(ConsoleVarFunc pChangeFunc)
{
    m_pChangeFunc = pChangeFunc;
}

uint64 CXConsoleVariableBase::AddOnChangeFunctor(const AZStd::function<void()>& pChangeFunctor)
{
    static int uniqueIdGenerator = 0;
    int newId = uniqueIdGenerator++;
    m_changeFunctors.push_back(std::make_pair(newId, pChangeFunctor));
    return newId;
}

ConsoleVarFunc CXConsoleVariableBase::GetOnChangeCallback() const
{
    return m_pChangeFunc;
}

void CXConsoleVariableBase::CallOnChangeFunctions()
{
    if (m_pChangeFunc)
    {
        m_pChangeFunc(this);
    }

    const size_t nTotal(m_changeFunctors.size());
    for (size_t nCount = 0; nCount < nTotal; ++nCount)
    {
        m_changeFunctors[nCount].second();
    }
}

void CXConsoleVariableBase::SetLimits(float min, float max)
{
    m_valueMin = min;
    m_valueMax = max;

    // Flag to determine when this variable has custom limits set
    m_hasCustomLimits = true;
}

void CXConsoleVariableBase::GetLimits(float& min, float& max)
{
    min = m_valueMin;
    max = m_valueMax;
}

bool CXConsoleVariableBase::HasCustomLimits()
{
    return m_hasCustomLimits;
}

const char* CXConsoleVariableBase::GetDataProbeString() const
{
    if (gEnv->IsDedicated() && m_pDataProbeString)
    {
        return m_pDataProbeString;
    }
    return GetOwnDataProbeString();
}

void CXConsoleVariableString::Set(const char* s)
{
    if (!s)
    {
        return;
    }

    if ((m_sValue == s) && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    if (m_pConsole->OnBeforeVarChange(this, s))
    {
        m_nFlags |= VF_MODIFIED;
        {
            m_sValue = s;
        }

        CallOnChangeFunctions();

        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableString::Set(float f)
{
    stack_string s = stack_string::format("%g", f);

    if ((m_sValue == s.c_str()) && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    m_nFlags |= VF_MODIFIED;
    Set(s.c_str());
}

void CXConsoleVariableString::Set(int i)
{
    stack_string s = stack_string::format("%d", i);

    if ((m_sValue == s.c_str()) && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    m_nFlags |= VF_MODIFIED;
    Set(s.c_str());
}

const char* CXConsoleVariableInt::GetString() const
{
    static char szReturnString[256];

    sprintf_s(szReturnString, "%d", GetIVal());
    return szReturnString;
}

void CXConsoleVariableInt::Set(const char* s)
{
    int nValue = TextToInt(s, m_iValue, (m_nFlags & VF_BITFIELD) != 0);

    Set(nValue);
}

void CXConsoleVariableInt::Set(float f)
{
    Set((int)f);
}

void CXConsoleVariableInt::Set(int i)
{
    if (i == m_iValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    stack_string s = stack_string::format("%d", i);

    if (m_pConsole->OnBeforeVarChange(this, s.c_str()))
    {
        m_nFlags |= VF_MODIFIED;
        m_iValue = i;

        CallOnChangeFunctions();

        m_pConsole->OnAfterVarChange(this);
    }
}

const char* CXConsoleVariableIntRef::GetString() const
{
    static char szReturnString[256];

    sprintf_s(szReturnString, "%d", m_iValue);
    return szReturnString;
}

void CXConsoleVariableIntRef::Set(const char* s)
{
    int nValue = TextToInt(s, m_iValue, (m_nFlags & VF_BITFIELD) != 0);
    if (nValue == m_iValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    if (m_pConsole->OnBeforeVarChange(this, s))
    {
        m_nFlags |= VF_MODIFIED;
        m_iValue = nValue;

        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableIntRef::Set(float f)
{
    if ((int)f == m_iValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    char sTemp[128];
    sprintf_s(sTemp, "%g", f);

    if (m_pConsole->OnBeforeVarChange(this, sTemp))
    {
        m_nFlags |= VF_MODIFIED;
        m_iValue = (int)f;
        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableIntRef::Set(int i)
{
    if (i == m_iValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    char sTemp[128];
    sprintf_s(sTemp, "%d", i);

    if (m_pConsole->OnBeforeVarChange(this, sTemp))
    {
        m_nFlags |= VF_MODIFIED;
        m_iValue = i;
        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

const char* CXConsoleVariableFloat::GetString() const
{
    static char szReturnString[256];

    sprintf_s(szReturnString, "%g", m_fValue); // %g -> "2.01",   %f -> "2.01000"
    return szReturnString;
}

void CXConsoleVariableFloat::Set(const char* s)
{
    float fValue = 0;
    if (s)
    {
        fValue = (float)atof(s);
    }

    if (fValue == m_fValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    if (m_pConsole->OnBeforeVarChange(this, s))
    {
        m_nFlags |= VF_MODIFIED;
        m_fValue = fValue;

        CallOnChangeFunctions();

        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableFloat::Set(float f)
{
    if (f == m_fValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    stack_string s = stack_string::format("%g", f);

    if (m_pConsole->OnBeforeVarChange(this, s.c_str()))
    {
        m_nFlags |= VF_MODIFIED;
        m_fValue = f;

        CallOnChangeFunctions();

        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableFloat::Set(int i)
{
    if ((float)i == m_fValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    char sTemp[128];
    sprintf_s(sTemp, "%d", i);

    if (m_pConsole->OnBeforeVarChange(this, sTemp))
    {
        m_nFlags |= VF_MODIFIED;
        m_fValue = (float)i;
        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

const char* CXConsoleVariableFloatRef::GetString() const
{
    static char szReturnString[256];

    sprintf_s(szReturnString, "%g", m_fValue);
    return szReturnString;
}

void CXConsoleVariableFloatRef::Set(const char *s)
{
    float fValue = 0;
    if (s)
    {
        fValue = (float)atof(s);
    }
    if (fValue == m_fValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    if (m_pConsole->OnBeforeVarChange(this, s))
    {
        m_nFlags |= VF_MODIFIED;
        m_fValue = fValue;

        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableFloatRef::Set(float f)
{
    if (f == m_fValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    char sTemp[128];
    sprintf_s(sTemp, "%g", f);

    if (m_pConsole->OnBeforeVarChange(this, sTemp))
    {
        m_nFlags |= VF_MODIFIED;
        m_fValue = f;
        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableFloatRef::Set(int i)
{
    if ((float)i == m_fValue && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    char sTemp[128];
    sprintf_s(sTemp, "%d", i);

    if (m_pConsole->OnBeforeVarChange(this, sTemp))
    {
        m_nFlags |= VF_MODIFIED;
        m_fValue = (float)i;
        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}

void CXConsoleVariableStringRef::Set(const char *s)
{
    if ((m_sValue == s) && (m_nFlags & VF_ALWAYSONCHANGE) == 0)
    {
        return;
    }

    if (m_pConsole->OnBeforeVarChange(this, s))
    {
        m_nFlags |= VF_MODIFIED;
        {
            m_sValue = s;
            m_userPtr = m_sValue.c_str();
        }

        CallOnChangeFunctions();
        m_pConsole->OnAfterVarChange(this);
    }
}
