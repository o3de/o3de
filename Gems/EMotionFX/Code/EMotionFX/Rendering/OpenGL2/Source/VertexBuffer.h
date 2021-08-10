/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_VERTEXBUFFER_H
#define __RENDERGL_VERTEXBUFFER_H

#include "RenderGLConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/LogManager.h>

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>
#include <EMotionFX/Rendering/OpenGL2/Source/GLExtensions.h>

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
        : private QOpenGLExtraFunctions
        , private RenderGL::GLExtensionFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(VertexBuffer, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:

        VertexBuffer();
        ~VertexBuffer();

        void Activate();
        void Deactivate();

        MCORE_INLINE uint32 GetBufferID() const         { return m_bufferId; }
        MCORE_INLINE uint32 GetNumVertices() const      { return m_numVertices; }

        bool Init(uint32 numBytesPerVertex, uint32 numVertices, EUsageMode usage, void* vertexData = nullptr);

        void* Lock(ELockMode lockMode = LOCK_WRITEONLY);
        void Unlock();

    private:
        uint32      m_bufferId;      // the buffer ID
        uint32      m_numVertices;   // the number of vertices

        bool GetIsSuccess();
        bool GetHasError();
    };
} // namespace RenderGL

#endif
