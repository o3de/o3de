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
