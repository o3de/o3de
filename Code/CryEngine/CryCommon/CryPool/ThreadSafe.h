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

#ifndef CRYINCLUDE_CRYPOOL_THREADSAFE_H
#define CRYINCLUDE_CRYPOOL_THREADSAFE_H
#pragma once


#include <CryThread.h>

namespace NCryPoolAlloc
{
    template<class TAllocator>
    class CThreadSafe
        :   public TAllocator
    {
        CryCriticalSection              m_Mutex;
    public:

        template<class T>
        ILINE   T                                       Allocate(size_t Size, size_t Align = 1)
        {
            CryAutoLock<CryCriticalSection> lock(m_Mutex);
            return TAllocator::template Allocate<T>(Size, Align);
        }


        template<class T>
        ILINE   bool                                Free(T pData, bool ForceBoundsCheck = false)
        {
            CryAutoLock<CryCriticalSection> lock(m_Mutex);
            return TAllocator::Free(pData, ForceBoundsCheck);
        }

        template<class T>
        ILINE   bool                                Resize(T** pData, size_t Size, size_t Alignment)
        {
            CryAutoLock<CryCriticalSection> lock(m_Mutex);
            return TAllocator::Resize(pData, Size, Alignment);
        }
    };
}




#endif // CRYINCLUDE_CRYPOOL_THREADSAFE_H

