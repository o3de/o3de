/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/numeric.h>
#include <MCore/Source/Config.h>
#include <MCore/Source/LogManager.h>
#include "GLSLShader.h"
#include "GraphicsManager.h"
#include <QFile>
#include <QTextStream>


namespace RenderGL
{
    // constructor
    GLSLShader::ShaderParameter::ShaderParameter(const char* name, GLint loc, bool isAttrib)
    {
        m_name           = name;
        m_type           = 0;
        m_size           = 0;
        m_location       = loc;
        m_isAttribute    = isAttrib;
        m_textureUnit    = MCORE_INVALIDINDEX32;
    }


    // constructor
    GLSLShader::GLSLShader()
    {
        m_program      = 0;
        m_vertexShader = 0;
        m_pixelShader  = 0;
        m_textureUnit  = 0;

        // pre-alloc data for uniforms and attributes
        m_uniforms.reserve(10);
        m_attributes.reserve(10);
        m_activatedAttribs.reserve(10);
        m_activatedTextures.reserve(10);
    }


    // destructor
    GLSLShader::~GLSLShader()
    {
        glDetachShader(m_program, m_vertexShader);
        glDetachShader(m_program, m_pixelShader);
        glDeleteShader(m_vertexShader);
        glDeleteShader(m_pixelShader);
        glDeleteShader(m_program);
    }


    // Activate
    void GLSLShader::Activate()
    {
        GetGraphicsManager()->SetShader(this);
    }


    // Deactivate
    void GLSLShader::Deactivate()
    {
        for (const size_t index : m_activatedAttribs)
        {
            glDisableVertexAttribArray(m_attributes[index].m_location);
        }

        for (const size_t index : m_activatedTextures)
        {
            assert(m_uniforms[index].m_type == GL_SAMPLER_2D);
            glActiveTexture(GL_TEXTURE0 + m_uniforms[index].m_textureUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        m_activatedAttribs.clear();
        m_activatedTextures.clear();
    }

    bool GLSLShader::Validate()
    {
        int success = 0;
        glValidateProgram(m_program);
        glGetProgramiv(m_program, GL_VALIDATE_STATUS, &success);

        if (success == 0)
        {
            MCore::LogInfo("Failed to validate program '%s'", m_fileName.c_str());
            InfoLog(m_program, &QOpenGLExtraFunctions::glGetProgramInfoLog);
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

        m_fileName = filename;

        AZStd::string text;
        text.reserve(4096);
        text = "#version 120\n";

        // build define string
        for (const AZStd::string& define : m_defines)
        {
            text += AZStd::string::format("#define %s\n", define.c_str());
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
            if (!m_defines.empty())
            {
                AZStd::string dStr;
                for (const AZStd::string& define : m_defines)
                {
                    if (!dStr.empty())
                    {
                        dStr.append(" ");
                    }
                    dStr.append(define);
                }

                MCore::LogDetailedInfo("[GLSL] Compiling shader '%s', with defines %s", m_fileName.c_str(), dStr.c_str());
            }
            else
            {
                MCore::LogDetailedInfo("[GLSL] Compiling shader '%s'", m_fileName.c_str());
            }

            MCore::LogDetailedInfo(text.c_str());
        }
    }


    // Init
    bool GLSLShader::Init(AZ::IO::PathView vertexFileName, AZ::IO::PathView pixelFileName, AZStd::vector<AZStd::string>& defines)
    {
        initializeOpenGLFunctions();
        /*const char* args[] = { "unroll all",
                               "inline all",
                               "O3",
                               nullptr };*/

        m_defines = defines;

        glUseProgram(0);

        // compile shaders
        if (!vertexFileName.empty() && CompileShader(GL_VERTEX_SHADER, &m_vertexShader, vertexFileName) == false)
        {
            return false;
        }

        if (!pixelFileName.empty() && CompileShader(GL_FRAGMENT_SHADER, &m_pixelShader, pixelFileName) == false)
        {
            return false;
        }

        // create program
        m_program = glCreateProgram();
        if (!vertexFileName.empty())
        {
            glAttachShader(m_program, m_vertexShader);
        }

        if (!pixelFileName.empty())
        {
            glAttachShader(m_program, m_pixelShader);
        }

        // link
        glLinkProgram(m_program);

        // check for linking errors
        GLint success = 0;
        glGetProgramiv(m_program, GL_LINK_STATUS, &success);

        if (!success)
        {
            MCore::LogInfo("[OpenGL] Failed to link shaders '%.*s' and '%.*s' ", AZ_STRING_ARG(vertexFileName.Native()), AZ_STRING_ARG(pixelFileName.Native()));
            InfoLog(m_program, &QOpenGLExtraFunctions::glGetProgramInfoLog);
            return false;
        }

        return true;
    }


    // FindAttribute
    GLSLShader::ShaderParameter* GLSLShader::FindAttribute(const char* name)
    {
        const size_t index = FindAttributeIndex(name);
        if (index == InvalidIndex)
        {
            return nullptr;
        }

        return &m_attributes[index];
    }


    // FindAttributeIndex
    size_t GLSLShader::FindAttributeIndex(const char* name)
    {
        const auto foundAttribute = AZStd::find_if(begin(m_attributes), end(m_attributes), [name](const auto& attribute)
        {
            return AzFramework::StringFunc::Equal(attribute.m_name.c_str(), name, false /* no case */) &&
                // if we don't have a valid parameter location, an attribute by this name doesn't exist
                // we just cached the fact that it doesn't exist, instead of failing glGetAttribLocation every time
                attribute.m_location >= 0;
        });
        if (foundAttribute != end(m_attributes))
        {
            return AZStd::distance(begin(m_attributes), foundAttribute);
        }

        // the parameter wasn't cached, try to retrieve it
        const GLint loc = glGetAttribLocation(m_program, name);
        m_attributes.emplace_back(name, loc, true);

        if (loc < 0)
        {
            return InvalidIndex;
        }

        return m_attributes.size() - 1;
    }


    // FindAttributeLocation
    size_t GLSLShader::FindAttributeLocation(const char* name)
    {
        ShaderParameter* p = FindAttribute(name);
        if (p == nullptr)
        {
            return InvalidIndex;
        }

        return p->m_location;
    }


    // FindUniform
    GLSLShader::ShaderParameter* GLSLShader::FindUniform(const char* name)
    {
        const size_t index = FindUniformIndex(name);
        if (index == InvalidIndex)
        {
            return nullptr;
        }

        return &m_uniforms[index];
    }


    // FindUniformIndex
    size_t GLSLShader::FindUniformIndex(const char* name)
    {
        const auto foundUniform = AZStd::find_if(begin(m_uniforms), end(m_uniforms), [name](const auto& uniform)
        {
            return AzFramework::StringFunc::Equal(uniform.m_name.c_str(), name, false /* no case */) &&
                uniform.m_location >= 0;
        });
        if (foundUniform != end(m_uniforms))
        {
            return AZStd::distance(begin(m_uniforms), foundUniform);
        }

        // the parameter wasn't cached, try to retrieve it
        const GLint loc = glGetUniformLocation(m_program, name);
        m_uniforms.emplace_back(name, loc, false);

        if (loc < 0)
        {
            return InvalidIndex;
        }

        return m_uniforms.size() - 1;
    }


    // SetAttribute
    void GLSLShader::SetAttribute(const char* name, uint32 dim, uint32 type, uint32 stride, size_t offset)
    {
        const size_t index = FindAttributeIndex(name);
        if (index == InvalidIndex)
        {
            return;
        }

        ShaderParameter* param = &m_attributes[index];

        glEnableVertexAttribArray(param->m_location);
        glVertexAttribPointer(param->m_location, dim, type, GL_FALSE, stride, (GLvoid*)offset);

        m_activatedAttribs.emplace_back(index);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, float value)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform1f(param->m_location, value);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, bool value)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform1f(param->m_location, (float)value);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const MCore::RGBAColor& color)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform4fv(param->m_location, 1, (float*)&color);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector2& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform2fv(param->m_location, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector3& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform3fv(param->m_location, 1, (float*)&vector);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Vector4& vector)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniform4fv(param->m_location, 1, (float*)&vector);
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

        glUniformMatrix4fv(param->m_location, 1, !transpose, (float*)&matrix);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, const AZ::Matrix4x4* matrices, uint32 count)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        glUniformMatrix4fv(param->m_location, count, GL_FALSE, (float*)matrices);
    }


    void GLSLShader::SetUniform(const char* name, const float* values, uint32 numFloats)
    {
        ShaderParameter* param = FindUniform(name);
        if (param == nullptr)
        {
            return;
        }

        // update the value
        glUniform1fv(param->m_location, numFloats, values);
    }


    // SetUniform
    void GLSLShader::SetUniform(const char* name, Texture* texture)
    {
        const size_t index = FindUniformIndex(name);
        if (index == InvalidIndex)
        {
            return;
        }

        m_uniforms[index].m_type = GL_SAMPLER_2D; // why is this being set here?

        // if the texture doesn't have a sampler unit assigned, give it one
        if (m_uniforms[index].m_textureUnit == MCORE_INVALIDINDEX32)
        {
            m_uniforms[index].m_textureUnit = m_textureUnit;
            m_textureUnit++;
        }

        if (texture == nullptr)
        {
            texture = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture();
        }

        glActiveTexture(GL_TEXTURE0 + m_uniforms[index].m_textureUnit);
        glBindTexture(GL_TEXTURE_2D, texture->GetID());
        glUniform1i(m_uniforms[index].m_location, m_uniforms[index].m_textureUnit);

        m_activatedTextures.emplace_back(index);
    }


    // link a texture to a given uniform
    void GLSLShader::SetUniformTextureID(const char* name, uint32 textureID)
    {
        const size_t index = FindUniformIndex(name);
        if (index == InvalidIndex)
        {
            return;
        }

        m_uniforms[index].m_type = GL_SAMPLER_2D; // why is this being set here?

        // if the texture doesn't have a sampler unit assigned, give it one
        if (m_uniforms[index].m_textureUnit == MCORE_INVALIDINDEX32)
        {
            m_uniforms[index].m_textureUnit = m_textureUnit;
            m_textureUnit++;
        }

        if (textureID == MCORE_INVALIDINDEX32)
        {
            textureID = GetGraphicsManager()->GetTextureCache()->GetWhiteTexture()->GetID();
        }

        glActiveTexture(GL_TEXTURE0 + m_uniforms[index].m_textureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(m_uniforms[index].m_location, m_uniforms[index].m_textureUnit);

        m_activatedTextures.emplace_back(index);
    }


    // check if the given attribute string is defined in the shader
    bool GLSLShader::CheckIfIsDefined(const char* attributeName) const
    {
        // get the number of defines and iterate through them
        return AZStd::any_of(begin(m_defines), end(m_defines), [attributeName](const AZStd::string& define)
        {
            return AzFramework::StringFunc::Equal(define.c_str(), attributeName, false /* no case */);
        });
    }
}
