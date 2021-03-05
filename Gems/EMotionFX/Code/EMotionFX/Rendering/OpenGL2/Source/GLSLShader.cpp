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

#include <MCore/Source/Config.h>
#include "GLInclude.h"
#include "GLSLShader.h"
#include "GraphicsManager.h"
#include <QFile>
#include <QTextStream>


namespace RenderGL
{
    // constructor
    GLSLShader::ShaderParameter::ShaderParameter(const char* name, GLint loc, bool isAttrib)
    {
        mName           = name;
        mType           = 0;
        mSize           = 0;
        mLocation       = loc;
        mIsAttribute    = isAttrib;
        mTextureUnit    = MCORE_INVALIDINDEX32;
    }


    // constructor
    GLSLShader::GLSLShader()
    {
        mProgram      = 0;
        mVertexShader = 0;
        mPixelShader  = 0;
        mTextureUnit  = 0;

        mUniforms.SetMemoryCategory(MEMCATEGORY_RENDERING);
        mAttributes.SetMemoryCategory(MEMCATEGORY_RENDERING);
        mActivatedAttribs.SetMemoryCategory(MEMCATEGORY_RENDERING);
        mActivatedTextures.SetMemoryCategory(MEMCATEGORY_RENDERING);

        // pre-alloc data for uniforms and attributes
        mUniforms.Reserve(10);
        mAttributes.Reserve(10);
        mActivatedAttribs.Reserve(10);
        mActivatedTextures.Reserve(10);
    }


    // destructor
    GLSLShader::~GLSLShader()
    {
        glDetachObjectARB(mProgram, mVertexShader);
        glDetachObjectARB(mProgram, mPixelShader);
        glDeleteObjectARB(mVertexShader);
        glDeleteObjectARB(mPixelShader);
        glDeleteObjectARB(mProgram);
    }


    // Activate
    void GLSLShader::Activate()
    {
        GetGraphicsManager()->SetShader(this);
    }


    // Deactivate
    void GLSLShader::Deactivate()
    {
        const uint32 numAttribs = mActivatedAttribs.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            const uint32 index = mActivatedAttribs[i];
            glDisableVertexAttribArrayARB(mAttributes[index].mLocation);
        }

        const uint32 numTextures = mActivatedTextures.GetLength();
        for (uint32 i = 0; i < numTextures; ++i)
        {
            const uint32 index = mActivatedTextures[i];
            assert(mUniforms[index].mType == GL_SAMPLER_2D_ARB);
            glActiveTextureARB(GL_TEXTURE0_ARB + mUniforms[index].mTextureUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        mActivatedAttribs.Clear(false);
        mActivatedTextures.Clear(false);
    }


    // GetType
    uint32 GLSLShader::GetType() const
    {
        return TypeID;
    }


    bool GLSLShader::CompileShader(const GLenum type, unsigned int* outShader, const char* filename)
    {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            AZ_Error("EMotionFX", false, "[GLSL] Failed to open shader file '%s'.", filename);
            return false;
        }

        mFileName = filename;

        AZStd::string text;
        text.reserve(4096);
        text = "#version 120\n";

        // build define string
        const uint32 numDefines = mDefines.GetLength();
        for (uint32 n = 0; n < numDefines; ++n)
        {
            text += AZStd::string::format("#define %s\n", mDefines[n].c_str());
        }

        // read file into a big string
        QTextStream textStream(&file);
        text += textStream.readAll().toUtf8().data();

        // create shader
        const char* textPtr = text.c_str();
        GLhandleARB shader = glCreateShaderObjectARB(type);
        glShaderSourceARB(shader, 1, (const GLcharARB**)&textPtr, nullptr);
        glCompileShaderARB(shader);

        // check for errors
        int success = 0;
        glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &success);

        InfoLog(shader);

        if (success == false)
        {
            MCore::LogError("[GLSL] Failed to compile shader '%s'.", filename);
            return false;
        }

        *outShader = shader;
        return true;
    }


    // LogInfo
    void GLSLShader::InfoLog(GLhandleARB object)
    {
        // get the info log
        int logLen     = 0;
        int logWritten = 0;

        glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLen);
        if (logLen > 1)
        {
            char* text = (char*)MCore::Allocate(logLen);

            glGetInfoLogARB(object, logLen, &logWritten, text);

            // if there are any defines, print that out too
            if (mDefines.GetLength() > 0)
            {
                AZStd::string dStr;
                const uint32 numDefines = mDefines.GetLength();
                for (uint32 n = 0; n < numDefines; ++n)
                {
                    if (n < numDefines - 1)
                    {
                        dStr += mDefines[n] + " ";
                    }
                    else
                    {
                        dStr += mDefines[n];
                    }
                }

                MCore::LogDetailedInfo("[GLSL] Compiling shader '%s', with defines %s", mFileName.c_str(), dStr.c_str());
            }
            else
            {
                MCore::LogDetailedInfo("[GLSL] Compiling shader '%s'", mFileName.c_str());
            }

            MCore::LogDetailedInfo(AZStd::string(text).c_str());
            MCore::Free(text);
        }
    }


    // Init
    bool GLSLShader::Init(const char* vFile, const char* pFile, MCore::Array<AZStd::string>& defines)
    {
        /*const char* args[] = { "unroll all",
                               "inline all",
                               "O3",
                               nullptr };*/

        mDefines = defines;

        glUseProgramObjectARB(0);

        // compile shaders
        if (vFile && CompileShader(GL_VERTEX_SHADER_ARB, &mVertexShader, vFile) == false)
        {
            return false;
        }

        if (pFile && CompileShader(GL_FRAGMENT_SHADER_ARB, &mPixelShader, pFile) == false)
        {
            return false;
        }

        // create program
        mProgram = glCreateProgramObjectARB();
        if (vFile)
        {
            glAttachObjectARB(mProgram, mVertexShader);
        }

        if (pFile)
        {
            glAttachObjectARB(mProgram, mPixelShader);
        }

        // link
        glLinkProgramARB(mProgram);

        // check for linking errors
        GLint success = 0;
        glGetObjectParameterivARB(mProgram, GL_OBJECT_LINK_STATUS_ARB, &success);

        if (success == 0)
        {
            MCore::LogInfo("[OpenGL] Failed to link shaders '%s' and '%s' ", vFile, pFile);
            InfoLog(mProgram);
            return false;
        }

        // validate shaders
        glValidateProgramARB(mProgram);
        glGetObjectParameterivARB(mProgram, GL_OBJECT_VALIDATE_STATUS_ARB, &success);

        if (success == 0)
        {
            MCore::LogInfo("Failed to validate shaders '%s' and '%s' ", vFile, pFile);
            InfoLog(mProgram);
            return false;
        }


        //glUseProgramObjectARB(mProgram);
        return true;
    }


    // FindAttribute
    GLSLShader::ShaderParameter* GLSLShader::FindAttribute(const char* name)
    {
        const uint32 index = FindAttributeIndex(name);
        if (index == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return &mAttributes[index];
    }


    // FindAttributeIndex
    uint32 GLSLShader::FindAttributeIndex(const char* name)
    {
        const uint32 numAttribs = mAttributes.GetLength();
        for (uint32 i = 0; i < numAttribs; ++i)
        {
            if (AzFramework::StringFunc::Equal(mAttributes[i].mName.c_str(), name, false /* no case */))
            {
                // if we don't have a valid parameter location, an attribute by this name doesn't exist
                // we just cached the fact that it doesn't exist, instead of failing glGetAttribLocation every time
                if (mAttributes[i].mLocation >= 0)
                {
                    return i;
                }

                return MCORE_INVALIDINDEX32;
            }
        }

        // the parameter wasn't cached, try to retrieve it
        const GLint loc = glGetAttribLocationARB(mProgram, name);
        mAttributes.Add(ShaderParameter(name, loc, true));

        if (loc < 0)
        {
            return MCORE_INVALIDINDEX32;
        }

        return mAttributes.GetLength() - 1;
    }


    // FindAttributeLocation
    uint32 GLSLShader::FindAttributeLocation(const char* name)
    {
        ShaderParameter* p = FindAttribute(name);
        if (p == nullptr)
        {
            return MCORE_INVALIDINDEX32;
        }

        return p->mLocation;
    }


    // FindUniform
    GLSLShader::ShaderParameter* GLSLShader::FindUniform(const char* name)
    {
        const uint32 index = FindUniformIndex(name);
        if (index == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return &mUniforms[index];
    }


    // FindUniformIndex
    uint32 GLSLShader::FindUniformIndex(const char* name)
    {
        const uint32 numUniforms = mUniforms.GetLength();
        for (uint32 i = 0; i < numUniforms; ++i)
        {
            if (AzFramework::StringFunc::Equal(mUniforms[i].mName.c_str(), name, false /* no case */))
            {
                if (mUniforms[i].mLocation >= 0)
                {
                    return i;
                }

                return MCORE_INVALIDINDEX32;
            }
        }

        // the parameter wasn't cached, try to retrieve it
        const GLint loc = glGetUniformLocationARB(mProgram, name);
        mUniforms.Add(ShaderParameter(name, loc, false));

        if (loc < 0)
        {
            return MCORE_INVALIDINDEX32;
        }

        return mUniforms.GetLength() - 1;
    }


    // SetAttribute
    void GLSLShader::SetAttribute(const char* name, uint32 dim, uint32 type, uint32 stride, size_t offset)
    {
        const uint32 index = FindAttributeIndex(name);
        if (index == MCORE_INVALIDINDEX32)
        {
            return;
        }

        ShaderParameter* param = &mAttributes[index];

        glEnableVertexAttribArrayARB(param->mLocation);
        glVertexAttribPointerARB(param->mLocation, dim, type, GL_FALSE, stride, (GLvoid*)offset);

        mActivatedAttribs.Add(index);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, float value)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform1fARB(param->mLocation, value);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, bool value)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform1fARB(param->mLocation, (float)value);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const MCore::RGBAColor& color)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform4fvARB(param->mLocation, 1, (float*)&color);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector2& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform2fvARB(param->mLocation, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector3& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform3fvARB(param->mLocation, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector4& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform4fvARB(param->mLocation, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Matrix4x4& matrix)
    {
        SetUniform(name, matrix, true);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Matrix4x4& matrix, bool transpose)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniformMatrix4fvARB(param->mLocation, 1, !transpose, (float*)&matrix);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Matrix4x4* matrices, uint32 count)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniformMatrix4fvARB(param->mLocation, count, GL_FALSE, (float*)matrices);
    }


    void GLSLShader::SetUniform(const char* name, const float* values, uint32 numFloats)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        // update the value
        glUniform1fvARB(param->mLocation, numFloats, values);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, Texture* texture)
    {
        const uint32 index = FindUniformIndex(name);
        if (index == MCORE_INVALIDINDEX32)
        {
            return;
        }

        mUniforms[index].mType = GL_SAMPLER_2D_ARB; // why is this being set here?

        // if the texture doesn't have a sampler unit assigned, give it one
        if (mUniforms[index].mTextureUnit == MCORE_INVALIDINDEX32)
        {
            mUniforms[index].mTextureUnit = mTextureUnit;
            mTextureUnit++;
        }

        if (texture == nullptr)
        {
            texture = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
        }

        glActiveTextureARB(GL_TEXTURE0_ARB + mUniforms[index].mTextureUnit);
        glBindTexture(GL_TEXTURE_2D, texture->GetID());
        glUniform1iARB(mUniforms[index].mLocation, mUniforms[index].mTextureUnit);

        mActivatedTextures.Add(index);
    }


    // link a texture to a given uniform
    void GLSLShader::SetUniformTextureID(const char* name, uint32 textureID)
    {
        const uint32 index = FindUniformIndex(name);
        if (index == MCORE_INVALIDINDEX32)
        {
            return;
        }

        mUniforms[index].mType = GL_SAMPLER_2D_ARB; // why is this being set here?

        // if the texture doesn't have a sampler unit assigned, give it one
        if (mUniforms[index].mTextureUnit == MCORE_INVALIDINDEX32)
        {
            mUniforms[index].mTextureUnit = mTextureUnit;
            mTextureUnit++;
        }

        if (textureID == MCORE_INVALIDINDEX32)
        {
            textureID = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture()->GetID();
        }

        glActiveTextureARB(GL_TEXTURE0_ARB + mUniforms[index].mTextureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1iARB(mUniforms[index].mLocation, mUniforms[index].mTextureUnit);

        mActivatedTextures.Add(index);
    }


    // check if the given attribute string is defined in the shader
    bool GLSLShader::CheckIfIsDefined(const char* attributeName)
    {
        // get the number of defines and iterate through them
        const uint32 numDefines = mDefines.GetLength();
        for (uint32 i = 0; i < numDefines; ++i)
        {
            // compare the given attribute with the current define and return if they are equal
            if (AzFramework::StringFunc::Equal(mDefines[i].c_str(), attributeName, false /* no case */))
            {
                return true;
            }
        }

        // we haven't found the attribute, return failure
        return false;
    }
}
