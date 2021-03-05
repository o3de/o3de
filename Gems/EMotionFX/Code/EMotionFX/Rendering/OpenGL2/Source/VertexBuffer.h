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

#ifndef __RENDERGL_VERTEXBUFFER_H
#define __RENDERGL_VERTEXBUFFER_H

#include "RenderGLConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/LogManager.h>


namespace RenderGL
{
    // describes the usage of the buffer
    enum EUsageMode
    {
        USAGE_STATIC,   //  = GL_STATIC_DRAW,   // never being updated
        USAGE_STREAM,   //  = GL_STREAM_DRAW,   // update once per frame
        USAGE_DYNAMIC   //  = GL_DYNAMIC_DRAW   // update multiple times per frame
    };

    // the lock modes
    enum ELockMode
    {
        LOCK_WRITEONLY, //  = GL_WRITE_ONLY,    // only write data to the buffer after locking
        LOCK_READONLY,  //  = GL_READ_ONLY,     // only read data from the buffer after locking
        LOCK_READWRITE  //  = GL_READ_WRITE     // both read and write to and from the buffer after locking
    };


    class RENDERGL_API VertexBuffer
    {
        MCORE_MEMORYOBJECTCATEGORY(VertexBuffer, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:

        VertexBuffer();
        ~VertexBuffer();

        void Activate();
        void Deactivate();

        MCORE_INLINE uint32 GetBufferID() const         { return mBufferID; }
        MCORE_INLINE uint32 GetNumVertices() const      { return mNumVertices; }

        bool Init(uint32 numBytesPerVertex, uint32 numVertices, EUsageMode usage, void* vertexData = nullptr);

        void* Lock(ELockMode lockMode = LOCK_WRITEONLY);
        void Unlock();

    private:
        uint32      mBufferID;      // the buffer ID
        uint32      mNumVertices;   // the number of vertices

        bool GetIsSuccess() const;
        bool GetHasError() const;
    };
} // namespace RenderGL

#endif
