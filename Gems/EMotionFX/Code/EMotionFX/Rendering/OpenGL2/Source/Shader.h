/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_SHADER__H
#define __RENDERGL_SHADER__H

#include <MCore/Source/Color.h>
#include "RenderGLConfig.h"
#include <AzCore/Math/PackedVector3.h>

namespace AZ
{
    class Matrix4x4;
    class Vector2;
}

namespace RenderGL
{
    // forward declaration
    class Texture;

    class RENDERGL_API Shader
    {
        MCORE_MEMORYOBJECTCATEGORY(Shader, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        Shader() {}
        virtual ~Shader() {}

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;
        virtual bool Validate() = 0;

        virtual uint32 GetType() const = 0;

        virtual void SetAttribute(const char* name, uint32 dim, uint32 type, uint32 stride, size_t offset) = 0;
        virtual void SetUniform(const char* name, float value) = 0;
        virtual void SetUniform(const char* name, bool value) = 0;
        virtual void SetUniform(const char* name, const MCore::RGBAColor& color) = 0;
        virtual void SetUniform(const char* name, const AZ::Vector2& vector) = 0;
        virtual void SetUniform(const char* name, const AZ::Vector3& vector) = 0;
        virtual void SetUniform(const char* name, const AZ::Vector4& vector) = 0;
        virtual void SetUniform(const char* name, const AZ::Matrix4x4& matrix) = 0;
        virtual void SetUniform(const char* name, const AZ::Matrix4x4& matrix, bool transpose) = 0;
        virtual void SetUniform(const char* name, const AZ::Matrix4x4* matrices, uint32 count) = 0;
        virtual void SetUniform(const char* name, Texture* texture) = 0;
        virtual void SetUniform(const char* name, const float* values, uint32 numFloats) = 0;
    };
}

#endif
