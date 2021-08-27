/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>
#include <AzCore/IO/Path/Path.h>
#include "PostProcessShader.h"
#include "GraphicsManager.h"


namespace RenderGL
{
    // default constructor
    PostProcessShader::PostProcessShader()
    {
        m_rt = nullptr;
    }


    // destructor
    PostProcessShader::~PostProcessShader()
    {
    }

    // Activate
    void PostProcessShader::ActivateRT(RenderTexture* target)
    {
        // Activate rt
        m_rt = target;
        m_rt->Activate();

        GLSLShader::Activate();
    }


    // Activate
    void PostProcessShader::ActivateRT(Texture* source, RenderTexture* target)
    {
        ActivateRT(target);

        // get the GBuffer
        GBuffer* buffer = GetGraphicsManager()->GetGBuffer();

        // setup the uniform shader values
        SetUniform("inputSize", AZ::Vector2(static_cast<float>(source->GetWidth()), static_cast<float>(source->GetHeight())));
        SetUniform("inputMap", source);
        SetUniformTextureID("shadedMap",    buffer->GetTextureID(GBuffer::COMPONENT_SHADED));
        SetUniformTextureID("glowMap",      buffer->GetTextureID(GBuffer::COMPONENT_GLOW));
    }


    void PostProcessShader::ActivateFromGBuffer(RenderTexture* target)
    {
        ActivateRT(target);

        // get the GBuffer
        GBuffer* buffer = GetGraphicsManager()->GetGBuffer();

        // setup the uniform shader values
        SetUniform("inputSize", AZ::Vector2(static_cast<float>(target->GetWidth()), static_cast<float>(target->GetHeight())));
        SetUniformTextureID("shadedMap",        buffer->GetTextureID(GBuffer::COMPONENT_SHADED));
        SetUniformTextureID("inputMap",         buffer->GetTextureID(GBuffer::COMPONENT_SHADED));
        SetUniformTextureID("glowMap",          buffer->GetTextureID(GBuffer::COMPONENT_GLOW));
    }


    // Deactivate
    void PostProcessShader::Deactivate()
    {
        GLSLShader::Deactivate();

        m_rt->Deactivate();
        m_rt = nullptr;
    }


    // Init
    bool PostProcessShader::Init(AZ::IO::PathView filename)
    {
        AZStd::vector<AZStd::string> defines;
        return GLSLShader::Init(nullptr, filename, defines);
    }


    // Render
    void PostProcessShader::Render()
    {
        const float w = static_cast<float>(m_rt->GetWidth());
        const float h = static_cast<float>(m_rt->GetHeight());

        // Setup ortho projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // texture coordinates scaled wrong?
        glDisable(GL_DEPTH_TEST);
        glDepthMask(0);

        glDisable(GL_BLEND);

        // Draw quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        glColor3f(1, 1, 1);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(w, 0.0f);
        glColor3f(1, 1, 1);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(w, h);
        glColor3f(1, 1, 1);

        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, h);
        glColor3f(1, 1, 1);
        glEnd();

        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);
    }
}
