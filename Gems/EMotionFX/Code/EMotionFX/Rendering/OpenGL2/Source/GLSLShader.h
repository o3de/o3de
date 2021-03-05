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

#ifndef __RENDERGL_GLSLSHADER_H
#define __RENDERGL_GLSLSHADER_H

#include <AzCore/std/string/string.h>
#include "Shader.h"

// include OpenGL
#include "GLInclude.h"
#include <MCore/Source/Array.h>


namespace RenderGL
{
    class RENDERGL_API GLSLShader
        : public Shader
    {
        MCORE_MEMORYOBJECTCATEGORY(GLSLShader, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        GLSLShader();
        virtual ~GLSLShader();

        void Activate() override;
        void Deactivate() override;

        uint32 FindAttributeLocation(const char* name);
        uint32 GetType() const override;

        MCORE_INLINE unsigned int GetProgram() const                                    { return mProgram; }
        bool CheckIfIsDefined(const char* attributeName);

        bool Init(const char* vertexFileName, const char* pixelFileName, MCore::Array<AZStd::string>& defines);
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

        uint32 FindAttributeIndex(const char* name);
        uint32 FindUniformIndex(const char* name);
        ShaderParameter* FindAttribute(const char* name);
        ShaderParameter* FindUniform(const char* name);

        bool CompileShader(const GLenum type, unsigned int* outShader, const char* filename);
        void InfoLog(unsigned int object);

        AZStd::string                   mFileName;

        MCore::Array<uint32>            mActivatedAttribs;
        MCore::Array<uint32>            mActivatedTextures;
        MCore::Array<ShaderParameter>   mUniforms;
        MCore::Array<ShaderParameter>   mAttributes;
        MCore::Array<AZStd::string>     mDefines;

        unsigned int                    mVertexShader;
        unsigned int                    mPixelShader;
        unsigned int                    mProgram;

        uint32                          mTextureUnit;
    };
}


#endif
