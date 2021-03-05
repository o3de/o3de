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

#pragma once

// include required headers
#include "StandardHeaders.h"


namespace MCore
{
    /**
     * The abstract data class, which represents a continuous block of memory.
     * Anything can be stored inside this piece of memory.
     */
    class MCORE_API AbstractData
    {
    public:
        AbstractData()
            : mData(nullptr)
            , mNumBytes(0)
            , mMaxNumBytes(0)                              { }
        AbstractData(uint32 numBytes)
            : mData(nullptr)
            , mNumBytes(0)
            , mMaxNumBytes(0)               { Resize(numBytes); }
        AbstractData(void* data, uint32 numBytes)
            : mData(nullptr)
            , mNumBytes(0)
            , mMaxNumBytes(0)   { InitFrom(data, numBytes); }
        AbstractData(const AbstractData& other)
            : mData(nullptr)
            , mNumBytes(0)
            , mMaxNumBytes(0)     { InitFrom(other.GetPointer(), other.GetNumBytes()); }
        ~AbstractData()                                                                             { Release(); }

        void Release()                                                          { MCore::Free(mData); mData = nullptr; mNumBytes = 0; mMaxNumBytes = 0; }
        void Clear()                                                            { mNumBytes = 0; }
        void Resize(uint32 numBytes)
        {
            // if we need to empty it
            if (numBytes == 0)
            {
                mNumBytes = 0;
                return;
            }

            //Release();
            if (mMaxNumBytes < numBytes)
            {
                mData           = MCore::Realloc(mData, numBytes, MCORE_MEMCATEGORY_ABSTRACTDATA);
                mNumBytes       = numBytes;
                mMaxNumBytes    = numBytes;
            }
            else
            {
                mNumBytes       = numBytes;
            }
        }

        void Reserve(uint32 numBytes)
        {
            if (mMaxNumBytes < numBytes)
            {
                mData           = MCore::Realloc(mData, numBytes, MCORE_MEMCATEGORY_ABSTRACTDATA);
                mMaxNumBytes    = numBytes;
            }
        }

        void Shrink()
        {
            if (mMaxNumBytes > mNumBytes)
            {
                mData           = MCore::Realloc(mData, mNumBytes, MCORE_MEMCATEGORY_ABSTRACTDATA);
                mMaxNumBytes    = mNumBytes;
            }
        }

        MCORE_INLINE void* GetPointer() const                                   { return mData; }
        MCORE_INLINE void* GetPointer()                                         { return mData; }
        MCORE_INLINE void CopyDataFrom(const void* data)                        { MCORE_ASSERT(mData); MCore::MemCopy(mData, data, mNumBytes); }
        MCORE_INLINE void InitFrom(const void* data, uint32 numBytes)
        {
            Resize(numBytes);
            if (numBytes == 0)
            {
                return;
            }
            MCORE_ASSERT(mData);
            MCore::MemCopy(mData, data, mNumBytes);
        }
        MCORE_INLINE uint32 GetNumBytes() const                                 { return mNumBytes; }
        MCORE_INLINE uint32 GetMaxNumBytes() const                              { return mMaxNumBytes; }

        MCORE_INLINE const AbstractData& operator=(const AbstractData& other)   { InitFrom(other.GetPointer(), other.GetNumBytes()); return *this; }

    private:
        void*   mData;
        uint32  mNumBytes;
        uint32  mMaxNumBytes;
    };
}   // namespace MCore
