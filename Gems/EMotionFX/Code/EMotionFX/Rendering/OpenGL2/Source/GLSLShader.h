/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_GLSLSHADER_H
#define __RENDERGL_GLSLSHADER_H

#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path.h>
#include "Shader.h"

// include OpenGL
#include <AzCore/std/containers/vector.h>

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>


namespace RenderGL
{
    class RENDERGL_API GLSLShader
        : public Shader
        , protected QOpenGLExtraFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(GLSLShader, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        GLSLShader();
        virtual ~GLSLShader();

        void Activate() override;
        void Deactivate() override;
        bool Validate() override;

        size_t FindAttributeLocation(const char* name);
        uint32 GetType() const override;

        MCORE_INLINE unsigned int GetProgram() const                                    { return mProgram; }
        bool CheckIfIsDefined(const char* attributeName) const;

        bool Init(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName, AZStd::vector<AZStd::string>& defines);
        void SetAttribute(const char* name, uint32 dim, uint32 type, uint32 stride, size_t offset) override;

        void SetUniform(const char* name, float value) override;
        void SetUniform(const char* name, bool value) override;
        void SetUniform(const char* name, const MCore::RGBAColor& color) override;
        void SetUniform(const char* name, const AZ::Vector2& vector) override;
        void SetUniform(const char* name, const AZ::Vector3& vector) override;
        void SetUniform(const char* name, const AZ::Vector4& vector) override;
        void SetUniform(const char* name, const AZ::Matrix4x4& matrix) override;
        void SetUniform(const char* name, const AZ::Matrix4x4& matrix, bool transpose) override;
        void SetUniform(const char* name, const AZ::Matrix4x4* matrices, uint32 count) override;
        void SetUniform(const char* name, Texture* texture) override;
        void SetUniformTextureID(const char* name, uint32 textureID);
        void SetUniform(const char* name, const float* values, uint32 numFloats) override;

        static const uint32 TypeID = 0x00000001;

    private:
        struct ShaderParameter
        {
            ShaderParameter(const char* name, GLint loc, bool isAttrib);

            AZStd::string       mName;
            GLint               mLocation;
            GLenum              mType;
            uint32              mSize;
            uint32              mTextureUnit;
            bool                mIsAttribute;
        };

        size_t FindAttributeIndex(const char* name);
        size_t FindUniformIndex(const char* name);
        ShaderParameter* FindAttribute(const char* name);
        ShaderParameter* FindUniform(const char* name);

        bool CompileShader(const GLenum type, unsigned int* outShader, AZ::IO::PathView filename);
        template<class T>
        void InfoLog(GLuint object, T func);

        AZ::IO::Path                    mFileName;

        AZStd::vector<size_t>            mActivatedAttribs;
        AZStd::vector<size_t>            mActivatedTextures;
        AZStd::vector<ShaderParameter>   mUniforms;
        AZStd::vector<ShaderParameter>   mAttributes;
        AZStd::vector<AZStd::string>     mDefines;

        unsigned int                    mVertexShader;
        unsigned int                    mPixelShader;
        unsigned int                    mProgram;

        uint32                          mTextureUnit;
    };
}


#endif
