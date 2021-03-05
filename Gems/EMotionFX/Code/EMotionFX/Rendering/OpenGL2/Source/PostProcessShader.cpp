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

#include <AzCore/Math/Vector2.h>
#include "PostProcessShader.h"
#include "GraphicsManager.h"


namespace RenderGL
{
    // default constructor
    PostProcessShader::PostProcessShader()
    {
        mRT = nullptr;
    }


    // destructor
    PostProcessShader::~PostProcessShader()
    {
    }

    // Activate
    void PostProcessShader::ActivateRT(RenderTexture* target)
    {
        // Activate rt
        mRT = target;
        mRT->Activate();

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

        mRT->Deactivate();
        mRT = nullptr;
    }


    // Init
    bool PostProcessShader::Init(const char* filename)
    {
        MCore::Array<AZStd::string> defines;
        return GLSLShader::Init(nullptr, filename, defines);
    }


    // Render
    void PostProcessShader::Render()
    {
        const float w = static_cast<float>(mRT->GetWidth());
        const float h = static_cast<float>(mRT->GetHeight());

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
