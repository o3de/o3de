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

#include <AzCore/std/string/string.h>
#include <MCore/Source/Config.h>
#include "GLInclude.h"

#include "VertexBuffer.h"


namespace RenderGL
{
    // constructor
    VertexBuffer::VertexBuffer()
    {
        mBufferID       = MCORE_INVALIDINDEX32;
        mNumVertices    = 0;
    }


    // destructor
    VertexBuffer::~VertexBuffer()
    {
        glDeleteBuffers(1, &mBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    // activate
    void VertexBuffer::Activate()
    {
        MCORE_ASSERT(mBufferID != MCORE_INVALIDINDEX32);
        glBindBuffer(GL_ARRAY_BUFFER, mBufferID);
    }


    // deactivate the vertex buffer
    void VertexBuffer::Deactivate()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // initialize vertex buffer
    bool VertexBuffer::Init(uint32 numBytesPerVertex, uint32 numVertices, EUsageMode usage, void* vertexData)
    {
        if (numVertices == 0 || numBytesPerVertex == 0)
        {
            return true;
        }

        GLenum usageGL;
        switch (usage)
        {
        case USAGE_STATIC:
            usageGL = GL_STATIC_DRAW;
            break;
        case USAGE_STREAM:
            usageGL = GL_STREAM_DRAW;
            break;
        case USAGE_DYNAMIC:
            usageGL = GL_DYNAMIC_DRAW;
            break;
        default:
            usageGL = GL_STATIC_DRAW;
        }

        // generate the buffer and bind it
        glGenBuffers(1, &mBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, mBufferID);
        glBufferData(GL_ARRAY_BUFFER, numBytesPerVertex * numVertices, vertexData, usageGL);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // adjust the number of vertices
        mNumVertices = numVertices;
        return true;
    }


    // lock the buffer
    void* VertexBuffer::Lock(ELockMode lockMode)
    {
        if (mNumVertices == 0)
        {
            return nullptr;
        }

        // lock the buffer
        GLenum lockModeGL;
        switch (lockMode)
        {
        case LOCK_WRITEONLY:
            lockModeGL = GL_WRITE_ONLY;
            break;
        case LOCK_READONLY:
            lockModeGL = GL_READ_ONLY;
            break;
        case LOCK_READWRITE:
            lockModeGL = GL_READ_WRITE;
            break;
        default:
            lockModeGL = GL_WRITE_ONLY;
        }

        glBindBuffer(GL_ARRAY_BUFFER, mBufferID);
        void* data = glMapBuffer(GL_ARRAY_BUFFER, lockModeGL);

        // is the data valid?
        if (data == nullptr)
        {
            AZStd::string strLock = " ";
            switch (lockMode)
            {
            case LOCK_WRITEONLY:
                strLock = "write-only";
                break;
            case LOCK_READONLY:
                strLock = "read-only";
                break;
            case LOCK_READWRITE:
                strLock = "read-write";
                break;
            }

            GLint errorCode = glGetError();
            MCore::LogError("Failed to lock OpenGL %s vertex buffer [glGetError=%d or 0x%x].", strLock.c_str(), errorCode, errorCode);
        }

        return data;
    }


    // unlock the buffer
    void VertexBuffer::Unlock()
    {
        if (mNumVertices == 0)
        {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, mBufferID);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    bool VertexBuffer::GetIsSuccess() const
    {
        return (glGetError() == GL_NO_ERROR);
    }

    bool VertexBuffer::GetHasError() const
    {
        return (glGetError() != GL_NO_ERROR);
    }
}