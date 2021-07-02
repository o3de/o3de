/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/Config.h>
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
        glDetachShader(mProgram, mVertexShader);
        glDetachShader(mProgram, mPixelShader);
        glDeleteShader(mVertexShader);
        glDeleteShader(mPixelShader);
        glDeleteShader(mProgram);
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
            glDisableVertexAttribArray(mAttributes[index].mLocation);
        }

        const uint32 numTextures = mActivatedTextures.GetLength();
        for (uint32 i = 0; i < numTextures; ++i)
        {
            const uint32 index = mActivatedTextures[i];
            assert(mUniforms[index].mType == GL_SAMPLER_2D);
            glActiveTexture(GL_TEXTURE0 + mUniforms[index].mTextureUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        mActivatedAttribs.Clear(false);
        mActivatedTextures.Clear(false);
    }

    bool GLSLShader::Validate()
    {
        int success = 0;
        glValidateProgram(mProgram);
        glGetProgramiv(mProgram, GL_VALIDATE_STATUS, &success);

        if (success == 0)
        {
            MCore::LogInfo("Failed to validate program '%s'", mFileName.c_str());
            InfoLog(mProgram, &QOpenGLExtraFunctions::glGetProgramInfoLog);
            return false;
        }
        return true;
    }


    // GetType
    uint32 GLSLShader::GetType() const
    {
        return TypeID;
    }


    bool GLSLShader::CompileShader(const GLenum type, unsigned int* outShader, AZ::IO::PathView filename)
    {
        QFile file(QString::fromUtf8(filename.Native().data(), aznumeric_caster(filename.Native().size())));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            AZ_Error("EMotionFX", false, "[GLSL] Failed to open shader file '%.*s'.", AZ_STRING_ARG(filename.Native()));
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
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, (const GLchar**)&textPtr, nullptr);
        glCompileShader(shader);

        // check for errors
        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        InfoLog(shader, &QOpenGLExtraFunctions::glGetShaderInfoLog);

        if (success == false)
        {
            MCore::LogError("[GLSL] Failed to compile shader '%.*s'.", AZ_STRING_ARG(filename.Native()));
            return false;
        }

        *outShader = shader;
        return true;
    }


    // LogInfo
    template<class T>
    void GLSLShader::InfoLog(GLuint object, T func)
    {
        // get the info log
        int logLen     = 0;
        int logWritten = 0;

        auto getIv = func == &QOpenGLExtraFunctions::glGetProgramInfoLog ? &QOpenGLExtraFunctions::glGetProgramiv : & QOpenGLExtraFunctions::glGetShaderiv;
        AZStd::invoke(getIv, static_cast<QOpenGLExtraFunctions*>(this), object, GL_INFO_LOG_LENGTH, &logLen);
        if (logLen > 1)
        {
            AZStd::string text;
            text.resize(logLen);

            AZStd::invoke(func, static_cast<QOpenGLExtraFunctions*>(this), object, logLen, &logWritten, text.data());

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

            MCore::LogDetailedInfo(text.c_str());
        }
    }


    // Init
    bool GLSLShader::Init(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName, MCore::Array<AZStd::string>& defines)
    {
        initializeOpenGLFunctions();
        /*const char* args[] = { "unroll all",
                               "inline all",
                               "O3",
                               nullptr };*/

        mDefines = defines;

        glUseProgram(0);

        // compile shaders
        if (!vertexFileName.empty() && CompileShader(GL_VERTEX_SHADER, &mVertexShader, vertexFileName) == false)
        {
            return false;
        }

        if (!pixelFileName.empty() && CompileShader(GL_FRAGMENT_SHADER, &mPixelShader, pixelFileName) == false)
        {
            return false;
        }

        // create program
        mProgram = glCreateProgram();
        if (!vertexFileName.empty())
        {
            glAttachShader(mProgram, mVertexShader);
        }

        if (!pixelFileName.empty())
        {
            glAttachShader(mProgram, mPixelShader);
        }

        // link
        glLinkProgram(mProgram);

        // check for linking errors
        GLint success = 0;
        glGetProgramiv(mProgram, GL_LINK_STATUS, &success);

        if (!success)
        {
            MCore::LogInfo("[OpenGL] Failed to link shaders '%.*s' and '%.*s' ", AZ_STRING_ARG(vertexFileName.Native()), AZ_STRING_ARG(pixelFileName.Native()));
            InfoLog(mProgram, &QOpenGLExtraFunctions::glGetProgramInfoLog);
            return false;
        }

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
        const GLint loc = glGetAttribLocation(mProgram, name);
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
        const GLint loc = glGetUniformLocation(mProgram, name);
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

        glEnableVertexAttribArray(param->mLocation);
        glVertexAttribPointer(param->mLocation, dim, type, GL_FALSE, stride, (GLvoid*)offset);

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

        glUniform1f(param->mLocation, value);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, bool value)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform1f(param->mLocation, (float)value);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const MCore::RGBAColor& color)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform4fv(param->mLocation, 1, (float*)&color);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector2& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform2fv(param->mLocation, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector3& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform3fv(param->mLocation, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector4& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform4fv(param->mLocation, 1, (float*)&vector);
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

        glUniformMatrix4fv(param->mLocation, 1, !transpose, (float*)&matrix);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Matrix4x4* matrices, uint32 count)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniformMatrix4fv(param->mLocation, count, GL_FALSE, (float*)matrices);
    }


    void GLSLShader::SetUniform(const char* name, const float* values, uint32 numFloats)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        // update the value
        glUniform1fv(param->mLocation, numFloats, values);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, Texture* texture)
    {
        const uint32 index = FindUniformIndex(name);
        if (index == MCORE_INVALIDINDEX32)
        {
            return;
        }

        mUniforms[index].mType = GL_SAMPLER_2D; // why is this being set here?

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

        glActiveTexture(GL_TEXTURE0 + mUniforms[index].mTextureUnit);
        glBindTexture(GL_TEXTURE_2D, texture->GetID());
        glUniform1i(mUniforms[index].mLocation, mUniforms[index].mTextureUnit);

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

        mUniforms[index].mType = GL_SAMPLER_2D; // why is this being set here?

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

        glActiveTexture(GL_TEXTURE0 + mUniforms[index].mTextureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(mUniforms[index].mLocation, mUniforms[index].mTextureUnit);

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
