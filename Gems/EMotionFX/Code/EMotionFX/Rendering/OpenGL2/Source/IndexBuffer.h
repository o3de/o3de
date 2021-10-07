/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_INDEXBUFFER_H
#define __RENDERGL_INDEXBUFFER_H

#include "VertexBuffer.h"

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>
#include <EMotionFX/Rendering/OpenGL2/Source/GLExtensions.h>

namespace RenderGL
{
    class RENDERGL_API IndexBuffer
        : private QOpenGLExtraFunctions
        , private RenderGL::GLExtensionFunctions
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

        MCORE_INLINE uint32 GetBufferID() const         { return m_bufferId; }
        MCORE_INLINE uint32 GetNumIndices() const       { return m_numIndices; }

        bool Init(EIndexSize indexSize, uint32 numIndices, EUsageMode usage, void* indexData = nullptr);

        void* Lock(ELockMode lockMode = LOCK_WRITEONLY);
        void Unlock();

    private:
        uint32      m_bufferId;      // the buffer ID
        uint32      m_numIndices;    // the number of indices

        // helpers
        bool GetIsSuccess();
        bool GetHasError();
    };
}

#endif
