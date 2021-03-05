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

#ifndef __RENDERGL_INDEXBUFFER_H
#define __RENDERGL_INDEXBUFFER_H

#include "VertexBuffer.h"


namespace RenderGL
{
    class RENDERGL_API IndexBuffer
    {
        MCORE_MEMORYOBJECTCATEGORY(IndexBuffer, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        enum EIndexSize
        {
            INDEXSIZE_16BIT = 2,
            INDEXSIZE_32BIT = 4
        };

        IndexBuffer();
        ~IndexBuffer();

        void Activate();

        MCORE_INLINE uint32 GetBufferID() const         { return mBufferID; }
        MCORE_INLINE uint32 GetNumIndices() const       { return mNumIndices; }

        bool Init(EIndexSize indexSize, uint32 numIndices, EUsageMode usage, void* indexData = nullptr);

        void* Lock(ELockMode lockMode = LOCK_WRITEONLY);
        void Unlock();

    private:
        uint32      mBufferID;      // the buffer ID
        uint32      mNumIndices;    // the number of indices

        // helpers
        bool GetIsSuccess() const;
        bool GetHasError() const;
    };
}

#endif
