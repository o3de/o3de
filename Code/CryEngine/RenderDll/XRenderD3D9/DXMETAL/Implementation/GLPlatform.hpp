/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Platform specific DXGL requirements implementation

#ifndef __GLPLATFORM__
#define __GLPLATFORM__

namespace NCryMetal
{
    enum ELogSeverity
    {
        eLS_Info,
        eLS_Warning,
        eLS_Error
    };
}

#define CUSTOM_SCOPED_PROFILE(_Name)

#include "GLCryPlatform.hpp"

#define DXGL_PROFILING 0

#if defined(_MSC_VER)
#define _DXGL_LOG_MSG(_TEXT, _FILE, _LINE, _FUNC) "DXGL: " _TEXT " : [@" _FUNC "] " _FILE "(" #_LINE ")"
#define DXGL_LOG_MSG(_TEXT) _DXGL_LOG_MSG(_TEXT, __FILE__, __LINE__, __FUNCSIG__)
#else
#define _DXGL_LOG_MSG(_TEXT, _FILE, _FUNC) "DXGL: " _TEXT " : [@" _FUNC "] " _FILE "(%d)"
#define DXGL_LOG_MSG(_TEXT) _DXGL_LOG_MSG(_TEXT, __FILE__, "?") // GCC can't resolve __FUNCTION__ in  the preprocessor
#endif

#if defined(_MSC_VER)
#define DXGL_WARNING(_Format, ...) NCryMetal::LogMessage(NCryMetal::eLS_Warning, DXGL_LOG_MSG(_Format), __VA_ARGS__)
#define DXGL_ERROR(_Format, ...)   NCryMetal::LogMessage(NCryMetal::eLS_Error,   DXGL_LOG_MSG(_Format), __VA_ARGS__)
#define DXGL_INFO(_Format, ...)    NCryMetal::LogMessage(NCryMetal::eLS_Info,    DXGL_LOG_MSG(_Format), __VA_ARGS__)
#else
#define DXGL_WARNING(_Format, ...) NCryMetal::LogMessage(NCryMetal::eLS_Warning, DXGL_LOG_MSG(_Format), ##__VA_ARGS__, __LINE__)
#define DXGL_ERROR(_Format, ...)   NCryMetal::LogMessage(NCryMetal::eLS_Error,   DXGL_LOG_MSG(_Format), ##__VA_ARGS__, __LINE__)
#define DXGL_INFO(_Format, ...)    NCryMetal::LogMessage(NCryMetal::eLS_Info,    DXGL_LOG_MSG(_Format), ##__VA_ARGS__, __LINE__)
#endif

#define DXGL_NOT_IMPLEMENTED NCryMetal::BreakUnique(__FILE__, __LINE__, __func__);
#define DXMETAL_NOT_IMPLEMENTED NCryMetal::BreakUnique(__FILE__, __LINE__, __func__);
#define DXGL_TODO(_DESC)
#define DXMETAL_TODO(_DESC)

#define DXGL_ARRAY_SIZE(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#define _DXGL_QUOTE(X) #X
#define DXGL_QUOTE(X) _DXGL_QUOTE(X)

#define DXGL_DECLARE_PTR(_Qualifier, _TypeName) \
    _Qualifier _TypeName;                       \
    typedef _smart_ptr<_TypeName> _TypeName ## Ptr;
#define DXGL_DECLARE_REF_COUNTED(_Qualifier, _TypeName) \
    DXGL_DECLARE_PTR(_Qualifier, _TypeName)             \
    _Qualifier _TypeName: public NCryMetal::SRefCounted < _TypeName >

#if DXGL_PROFILING
#define DXGL_SCOPED_PROFILE(_Name)                \
    SScopedProfileLabel kDXGLProfileLabel(_Name); \
    CUSTOM_SCOPED_PROFILE(_Name)
#else
#define DXGL_SCOPED_PROFILE(_Name)
#endif

namespace NCryMetal
{
    template <typename T>
    struct SRefCounted
    {
        uint32 m_uRefCnt;

        SRefCounted()
            : m_uRefCnt(0)
        {
        }

        void AddRef()
        {
            ++m_uRefCnt;
        }

        void Release()
        {
            if (--m_uRefCnt == 0)
            {
                delete static_cast<T*>(this);
            }
        }
    };

    struct SSpinlockBitMaskWord
    {
        typedef volatile LONG Word;
        enum
        {
            BIT_SIZE = sizeof(Word) * 8
        };
        Word m_aMask;

        void Set(uint32 uIndex, bool bFlag)
        {
            while (true)
            {
                Word kOld(m_aMask);
                Word uFlagMask(1 << uIndex);
                Word kNew(bFlag ? (kOld | uFlagMask) : (kOld & ~uFlagMask));
                if (CompareExchange(&m_aMask, kNew, kOld) == kOld)
                {
                    break;
                }
            }
        }

        bool Get(uint32 uIndex) const
        {
            CRY_ASSERT(uIndex <= BIT_SIZE);
            return ((m_aMask >> uIndex) & 1) != 0;
        }

        void Init(bool bValue)
        {
            m_aMask = bValue ? ~(LONG)0 : (LONG)0;
        }
    };

    struct SUnsafeBitMaskWord
    {
        typedef uint32 Word;
        enum
        {
            BIT_SIZE = sizeof(Word) * 8
        };
        Word m_aMask;

        SUnsafeBitMaskWord operator|(const SUnsafeBitMaskWord& kOther)
        {
            SUnsafeBitMaskWord kRes = {m_aMask | kOther.m_aMask};
            return kRes;
        }

        SUnsafeBitMaskWord operator&(const SUnsafeBitMaskWord& kOther)
        {
            SUnsafeBitMaskWord kRes = {m_aMask& kOther.m_aMask};
            return kRes;
        }

        bool operator==(const SUnsafeBitMaskWord& kOther)
        {
            return m_aMask == kOther.m_aMask;
        }

        bool operator!=(const SUnsafeBitMaskWord& kOther)
        {
            return m_aMask != kOther.m_aMask;
        }

        SUnsafeBitMaskWord& operator=(const SUnsafeBitMaskWord& kOther)
        {
            m_aMask = kOther.m_aMask;
            return *this;
        }

        void Set(uint32 uIndex, bool bFlag)
        {
            CRY_ASSERT(uIndex <= BIT_SIZE);
            Word uFlagMask(1 << uIndex);
            m_aMask = bFlag ? (m_aMask | uFlagMask) : (m_aMask & ~uFlagMask);
        }

        bool Get(uint32 uIndex) const
        {
            CRY_ASSERT(uIndex <= BIT_SIZE);
            return ((m_aMask >> uIndex) & 1) != 0;
        }

        void Init(bool bValue)
        {
            m_aMask = bValue ? ~(Word)0 : (Word)0;
        }
    };

    template <uint32 uNumWords, typename BitMaskWord>
    struct SBitMaskMultiWord
    {
        typedef BitMaskWord Word;
        Word m_akWords[uNumWords];

        void Set(uint32 uIndex, bool bFlag)
        {
            m_akWords[uIndex / Word::BIT_SIZE].Set(uIndex % Word::BIT_SIZE, bFlag);
        }

        bool Get(uint32 uIndex) const
        {
            return m_akWords[uIndex / Word::BIT_SIZE].Get(uIndex % Word::BIT_SIZE);
        }

        void Init(bool bValue)
        {
            memset(m_akWords, sizeof(m_akWords), bValue ? 0xFF : 0);
        }

        SBitMaskMultiWord& operator=(const SBitMaskMultiWord& kOther)
        {
            memcpy(&m_akWords, kOther.m_akWords, sizeof(m_akWords));
        }
    };

    template <uint32 uNumWords, typename BitMaskWord>
    SBitMaskMultiWord<uNumWords, BitMaskWord> operator|(const SBitMaskMultiWord<uNumWords, BitMaskWord>& kLeft, const SBitMaskMultiWord<uNumWords, BitMaskWord>& kRight)
    {
        SBitMaskMultiWord<uNumWords, BitMaskWord> kRes;
        for (uint32 uIndex = 0; uIndex < uNumWords; ++uIndex)
        {
            kRes.m_akWords[uIndex] = kLeft.m_akWords[uIndex] | kRight.m_akWords[uIndex];
        }
        return kRes;
    }

    template <uint32 uNumWords, typename BitMaskWord>
    SBitMaskMultiWord<uNumWords, BitMaskWord> operator&(const SBitMaskMultiWord<uNumWords, BitMaskWord>& kLeft, const SBitMaskMultiWord<uNumWords, BitMaskWord>& kRight)
    {
        SBitMaskMultiWord<uNumWords, BitMaskWord> kRes;
        for (uint32 uIndex = 0; uIndex < uNumWords; ++uIndex)
        {
            kRes.m_akWords[uIndex] = kLeft.m_akWords[uIndex] & kRight.m_akWords[uIndex];
        }
        return kRes;
    }

    template <uint32 uNumWords, typename BitMaskWord>
    bool operator==(const SBitMaskMultiWord<uNumWords, BitMaskWord>& kLeft, const SBitMaskMultiWord<uNumWords, BitMaskWord>& kRight)
    {
        for (uint32 uIndex = 0; uIndex < uNumWords; ++uIndex)
        {
            if (!(kLeft.m_akWords[uIndex] == kRight.m_akWords[uIndex]))
            {
                return false;
            }
        }
        return true;
    }

    template <uint32 uNumWords, typename BitMaskWord>
    bool operator!=(const SBitMaskMultiWord<uNumWords, BitMaskWord>& kLeft, const SBitMaskMultiWord<uNumWords, BitMaskWord>& kRight)
    {
        for (uint32 uIndex = 0; uIndex < uNumWords; ++uIndex)
        {
            if ((kLeft.m_akWords[uIndex] != kRight.m_akWords[uIndex]))
            {
                return true;
            }
        }
        return false;
    }

    template <uint32 uNumWords, typename BitMaskWord>
    struct SBitMaskImpl
    {
        typedef SBitMaskMultiWord<uNumWords, BitMaskWord> Type;
    };
    template <typename BitMaskWord>
    struct SBitMaskImpl<1, BitMaskWord>
    {
        typedef BitMaskWord Type;
    };
    template <uint32 uSize, typename BitMaskWord>
    struct SBitMask
    {
        typedef typename SBitMaskImpl<(uSize + BitMaskWord::BIT_SIZE - 1) / BitMaskWord::BIT_SIZE, BitMaskWord>::Type Type;
    };

    struct SList
    {
        typedef SLockFreeSingleLinkedListHeader TListHeader;
        typedef SLockFreeSingleLinkedListEntry TListEntry;

        SList()
        {
            CryInitializeSListHead(m_kHeader);
        }

        void Push(TListEntry& kEntry)
        {
            CryInterlockedPushEntrySList(m_kHeader, kEntry);
        }

        TListEntry* Pop()
        {
            return alias_cast<TListEntry*>(CryInterlockedPopEntrySList(m_kHeader));
        }

        TListHeader m_kHeader;
    };

    struct SScopedProfileLabel
    {
        const char* m_szName;

        SScopedProfileLabel(const char* szName)
            : m_szName(szName)
        {
            DXGLProfileLabelPush(m_szName);
        }

        ~SScopedProfileLabel()
        {
            DXGLProfileLabelPop(m_szName);
        }
    };
}

#endif //__GLPLATFORM__
