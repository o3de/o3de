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

#ifndef CRYINCLUDE_CRYPOOL_MEMORY_H
#define CRYINCLUDE_CRYPOOL_MEMORY_H
#pragma once

namespace NCryPoolAlloc
{
    class CMemoryDynamic
    {
        size_t                              m_Size;
        uint8*                              m_pData;

    protected:
        ILINE                                   CMemoryDynamic()
            : m_Size(0)
            , m_pData(0){}

    public:
        ILINE void                      InitMem(const size_t S, uint8* pData)
        {
            m_Size      =   S;
            m_pData     =   pData;
            CPA_ASSERT(S);
            CPA_ASSERT(pData);
        }

        ILINE   size_t                  MemSize() const{return m_Size; }
        ILINE   uint8*                  Data(){return m_pData; }
        ILINE   const uint8*        Data() const{return m_pData; }
    };

    template<size_t TSize>
    class CMemoryStatic
    {
        uint8                                   m_Data[TSize];

    protected:
        ILINE                                   CMemoryStatic()
        {
        }
    public:
        ILINE void                      InitMem(const size_t S, uint8* pData)
        {
        }
        ILINE   size_t                  MemSize() const{return TSize; }
        ILINE   uint8*                  Data(){return m_Data; }
        ILINE   const uint8*        Data() const{return m_Data; }
    };
}







#endif // CRYINCLUDE_CRYPOOL_MEMORY_H

