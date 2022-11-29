/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_SIMPLESTRINGPOOL_H
#define CRYINCLUDE_CRYSYSTEM_SIMPLESTRINGPOOL_H
#pragma once

#include "ISystem.h"
#include <StlUtils.h>

//TODO: Pull most of this into a cpp file!


struct SStringData
{
    SStringData(const char* szString, int nStrLen)
        : m_szString(szString)
        , m_nStrLen(nStrLen)
    {
    }

    const char* m_szString;
    int m_nStrLen;
    bool operator==(const SStringData& other) const
    {
        if (m_nStrLen != other.m_nStrLen)
        {
            return false;
        }

        return strcmp(m_szString, other.m_szString) == 0;
    }
private:
};

template<>
inline const char* stl::constchar_cast(const SStringData& in)
{
    return in.m_szString;
}


/////////////////////////////////////////////////////////////////////
// String pool implementation.
// Inspired by expat implementation.
/////////////////////////////////////////////////////////////////////
class CSimpleStringPool
{
public:
    enum
    {
        STD_BLOCK_SIZE = 1u << 16
    };
    struct BLOCK
    {
        BLOCK* next;
        int size;
        char s[1];
    };
    unsigned int m_blockSize;
    BLOCK* m_blocks;
    BLOCK* m_free_blocks;
    const char* m_end;
    char* m_ptr;
    char* m_start;
    int nUsedSpace;
    int nUsedBlocks;
    bool m_reuseStrings;

    typedef AZStd::unordered_map<SStringData, char*, stl::hash_string<SStringData> > TStringToExistingStringMap;
    TStringToExistingStringMap m_stringToExistingStringMap;

    static size_t g_nTotalAllocInXmlStringPools;

    CSimpleStringPool()
    {
        m_blockSize = STD_BLOCK_SIZE  - offsetof(BLOCK, s);
        m_blocks = 0;
        m_start = 0;
        m_ptr = 0;
        m_end = 0;
        nUsedSpace = 0;
        nUsedBlocks = 0;
        m_free_blocks = 0;
        m_reuseStrings = false;
    }

    explicit CSimpleStringPool(bool reuseStrings)
        : m_blockSize(STD_BLOCK_SIZE - offsetof(BLOCK, s))
        , m_blocks(NULL)
        , m_free_blocks(NULL)
        , m_end(0)
        , m_ptr(0)
        , m_start(0)
        , nUsedSpace(0)
        , nUsedBlocks(0)
        , m_reuseStrings(reuseStrings)
    {
    }

    ~CSimpleStringPool()
    {
        BLOCK* pBlock = m_blocks;
        while (pBlock)
        {
            BLOCK* temp = pBlock->next;
            g_nTotalAllocInXmlStringPools -= (offsetof(BLOCK, s) + pBlock->size * sizeof(char));
            azfree(pBlock);
            pBlock = temp;
        }
        pBlock = m_free_blocks;
        while (pBlock)
        {
            BLOCK* temp = pBlock->next;
            g_nTotalAllocInXmlStringPools -= (offsetof(BLOCK, s) + pBlock->size * sizeof(char));
            azfree(pBlock);
            pBlock = temp;
        }
        m_blocks = 0;
        m_ptr = 0;
        m_start = 0;
        m_end = 0;
    }
    void SetBlockSize(unsigned int nBlockSize)
    {
        if (nBlockSize > 1024 * 1024)
        {
            nBlockSize = 1024 * 1024;
        }
        unsigned int size = 512;
        while (size < nBlockSize)
        {
            size *= 2;
        }

        m_blockSize = size - offsetof(BLOCK, s);
    }
    void Clear()
    {
        BLOCK* pLast = m_free_blocks;
        if (pLast)
        {
            while (pLast->next)
            {
                pLast = pLast->next;
            }

            pLast->next = m_blocks;
        }
        else
        {
            m_free_blocks = m_blocks;
        }

        m_blocks = 0;
        m_start = 0;
        m_ptr = 0;
        m_end = 0;
        nUsedSpace = 0;
        if (m_reuseStrings)
        {
            m_stringToExistingStringMap.clear();
        }
    }
    char* Append(const char* ptr, int nStrLen)
    {
        // If a string does not fit within the remainder of the string pool, a new pool will be allocated with at least
        // nStrLen + 1 size, which means this code does take care of incredibly large strings.

        if (m_reuseStrings)
        {
            if (char* existingString = FindExistingString(ptr, nStrLen))
            {
                return existingString;
            }
        }

        char* ret = m_ptr;
        if (m_ptr && nStrLen + 1 < (m_end - m_ptr))
        {
            memcpy(m_ptr, ptr, nStrLen);
            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
        }
        else
        {
            int nNewBlockSize = std::max(nStrLen + 1, (int)m_blockSize);
            AllocBlock(nNewBlockSize, nStrLen + 1);
            PREFAST_ASSUME(m_ptr);
            memcpy(m_ptr, ptr, nStrLen);
            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
            ret = m_start;
        }

        if (m_reuseStrings)
        {
            assert(!FindExistingString(ptr, nStrLen));
            m_stringToExistingStringMap[SStringData(ret, nStrLen)] = ret;
        }

        nUsedSpace += nStrLen;
        return ret;
    }
    char* ReplaceString(const char* str1, const char* str2)
    {
        if (m_reuseStrings)
        {
            CryFatalError("Can't replace strings in an xml node that reuses strings");
        }

        int nStrLen1 = static_cast<int>(strlen(str1));
        int nStrLen2 = static_cast<int>(strlen(str2));

        // undo ptr1 add.
        if (m_ptr != m_start)
        {
            m_ptr = m_ptr - nStrLen1 - 1;
        }

        assert(m_ptr == str1);

        int nStrLen = nStrLen1 + nStrLen2;

        char* ret = m_ptr;
        if (m_ptr && nStrLen + 1 < (m_end - m_ptr))
        {
            if (m_ptr != str1)
            {
                memcpy(m_ptr, str1, nStrLen1);
            }
            memcpy(m_ptr + nStrLen1, str2, nStrLen2);
            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
        }
        else
        {
            int nNewBlockSize = std::max(nStrLen + 1, (int)m_blockSize);
            if (m_ptr == m_start)
            {
                ReallocBlock(nNewBlockSize * 2); // Reallocate current block.
                PREFAST_ASSUME(m_ptr);
                memcpy(m_ptr + nStrLen1, str2, nStrLen2);
            }
            else
            {
                AllocBlock(nNewBlockSize, nStrLen + 1);
                PREFAST_ASSUME(m_ptr);
                memcpy(m_ptr, str1, nStrLen1);
                memcpy(m_ptr + nStrLen1, str2, nStrLen2);
            }

            m_ptr = m_ptr + nStrLen;
            *m_ptr++ = 0; // add null termination.
            ret = m_start;
        }
        nUsedSpace += nStrLen;
        return ret;
    }

private:
    CSimpleStringPool(const CSimpleStringPool&);
    CSimpleStringPool& operator = (const CSimpleStringPool&);

private:
    void AllocBlock(int blockSize, int nMinBlockSize)
    {
        if (m_free_blocks)
        {
            BLOCK* pBlock = m_free_blocks;
            BLOCK* pPrev = 0;
            while (pBlock)
            {
                if (pBlock->size >= nMinBlockSize)
                {
                    // Reuse free block
                    if (pPrev)
                    {
                        pPrev->next = pBlock->next;
                    }
                    else
                    {
                        m_free_blocks = pBlock->next;
                    }

                    pBlock->next = m_blocks;
                    m_blocks = pBlock;
                    m_ptr = pBlock->s;
                    m_start = pBlock->s;
                    m_end = pBlock->s + pBlock->size;
                    return;
                }
                pPrev = pBlock;
                pBlock = pBlock->next;
            }
        }
        size_t nMallocSize = offsetof(BLOCK, s) + blockSize * sizeof(char);
        g_nTotalAllocInXmlStringPools += nMallocSize;

        BLOCK* pBlock = (BLOCK*)azmalloc(nMallocSize);
        ;
        assert(pBlock);
        pBlock->size = blockSize;
        pBlock->next = m_blocks;
        m_blocks = pBlock;
        m_ptr = pBlock->s;
        m_start = pBlock->s;
        m_end = pBlock->s + blockSize;
        nUsedBlocks++;
    }
    void ReallocBlock(int blockSize)
    {
        if (m_reuseStrings)
        {
            CryFatalError("Can't replace strings in an xml node that reuses strings");
        }

        BLOCK* pThisBlock = m_blocks;
        BLOCK* pPrevBlock = m_blocks->next;
        m_blocks = pPrevBlock;

        size_t nMallocSize = offsetof(BLOCK, s) + blockSize * sizeof(char);
        if (pThisBlock)
        {
            g_nTotalAllocInXmlStringPools -= (offsetof(BLOCK, s) + pThisBlock->size * sizeof(char));
        }
        g_nTotalAllocInXmlStringPools += nMallocSize;


        BLOCK* pBlock = (BLOCK*)azrealloc(pThisBlock, nMallocSize);
        assert(pBlock);
        pBlock->size = blockSize;
        pBlock->next = m_blocks;
        m_blocks = pBlock;
        m_ptr = pBlock->s;
        m_start = pBlock->s;
        m_end = pBlock->s + blockSize;
    }

    char* FindExistingString(const char* szString, int nStrLen)
    {
        SStringData testData(szString, nStrLen);
        char* szResult = stl::find_in_map(m_stringToExistingStringMap, testData, NULL);
        assert(!szResult || !_stricmp(szResult, szString));
        return szResult;
    }
};


#endif // CRYINCLUDE_CRYSYSTEM_SIMPLESTRINGPOOL_H
