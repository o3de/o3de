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

#include "IndexBuffer.h"


namespace RenderGL
{
    // default constructor
    IndexBuffer::IndexBuffer()
    {
        mBufferID   = MCORE_INVALIDINDEX32;
        mNumIndices = 0;
    }


    // destructor
    IndexBuffer::~IndexBuffer()
    {
        glDeleteBuffers(1, &mBufferID);
    }


    // activate
    void IndexBuffer::Activate()
    {
        assert(mBufferID != MCORE_INVALIDINDEX32);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferID);
    }


    // initialize the index buffer
    bool IndexBuffer::Init(EIndexSize indexSize, uint32 numIndices, EUsageMode usage, void* indexData)
    {
        if (numIndices == 0)
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

        // generate the buffer ID and bind it
        glGenBuffers(1, &mBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (uint32)indexSize * numIndices, indexData, usageGL);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        /*
            // check for errors
            if (HasError())
            {
                String strUsage = " ";
                String strType  = " ";

                switch (usage)
                {
                    case USAGE_STATIC:
                        strUsage = "static";
                        break;
                    case USAGE_STREAM:
                        strUsage = "stream";
                        break;
                    case USAGE_DYNAMIC:
                        strUsage = "dynamic";
                        break;
                }

                switch (indexSize)
                {
                    case INDEXSIZE_16BIT:
                        strType = "16-bit";
                        break;
                    case INDEXSIZE_32BIT:
                        strType = "32-bit";
                        break;
                }

                LogError( "Failed to create %s %s OpenGL index buffer of %d bytes (%d indices).", strUsage.AsChar(), strType.AsChar(), (uint32)indexSize * numIndices, numIndices );
                return false;
            }
        */
        // adjust the number of indices
        mNumIndices = numIndices;

        return true;
    }


    // lock the buffer
    void* IndexBuffer::Lock(ELockMode lockMode)
    {
        if (mNumIndices == 0)
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

        // lock the buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferID);
        void* data = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, lockModeGL);

        // check for failure
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

            MCore::LogError("Failed to lock %s OpenGL index buffer.", strLock.c_str());
        }

        return data;
    }


    // unlock the buffer
    void IndexBuffer::Unlock()
    {
        if (mNumIndices == 0)
        {
            return;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBufferID);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }

    bool IndexBuffer::GetIsSuccess() const
    {
        return (glGetError() == GL_NO_ERROR);
    }

    bool IndexBuffer::GetHasError() const
    {
        return (glGetError() != GL_NO_ERROR);
    }
}
