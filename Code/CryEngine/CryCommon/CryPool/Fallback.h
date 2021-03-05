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

#ifndef CRYINCLUDE_CRYPOOL_FALLBACK_H
#define CRYINCLUDE_CRYPOOL_FALLBACK_H
#pragma once


namespace NCryPoolAlloc
{
    enum EFallbackMode
    {
        EFM_DISABLED,
        EFM_ENABLED,
        EFM_ALWAYS
    };
    template<class TAllocator>
    class CFallback
        :   public TAllocator
    {
        EFallbackMode                           m_Fallback;
    public:
        ILINE                                           CFallback()
            : m_Fallback(EFM_DISABLED)
        {
        }

        template<class T>
        ILINE   T                                       Allocate(size_t Size, size_t Align = 1)
        {
            if (EFM_ALWAYS == m_Fallback)
            {
                return reinterpret_cast<T>(CPA_ALLOC(Align, Size));
            }
            T pRet  =   TAllocator::template Allocate<T>(Size, Align);
            if (!pRet && EFM_ENABLED == m_Fallback)
            {
                return reinterpret_cast<T>(CPA_ALLOC(Align, Size));
            }
            return pRet;
        }

        template<class T>
        ILINE   bool                                Free(T Handle)
        {
            if (!Handle)
            {
                return true;
            }
            if (EFM_ALWAYS == m_Fallback)
            {
                CPA_FREE(Handle);
                return true;
            }

            if (EFM_ENABLED == m_Fallback && TAllocator::InBounds(Handle, true))
            {
                CPA_FREE(Handle);
                return true;
            }
            return TAllocator::template Free<T>(Handle);
        }

        void                                            FallbackMode(EFallbackMode M){m_Fallback = M; }
        EFallbackMode                           FallbaclMode() const{return m_Fallback; }
    };
}


#endif // CRYINCLUDE_CRYPOOL_FALLBACK_H

