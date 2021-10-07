/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/Config.h>
#include <MCore/Source/LogManager.h>

#include "RenderTexture.h"
#include "GraphicsManager.h"

#include <QOpenGLFunctions>

namespace RenderGL
{
    // constructor
    RenderTexture::RenderTexture()
    {
        m_format         = 0;
        m_width          = 0;
        m_height         = 0;
        m_frameBuffer    = 0;
        m_depthBuffer    = 0;
        m_texture        = 0;
    }


    // destructor
    RenderTexture::~RenderTexture()
    {
        glDeleteTextures(1, &m_texture);
        glDeleteRenderbuffers(1, &m_depthBuffer);
        glDeleteFramebuffers(1, &m_frameBuffer);
    }


    // activate the render texture
    void RenderTexture::Activate()
    {
        // check if the render texture is already active
        //if (GetGraphicsManager()->GetRenderTexture() == this)
        //  return;

        // get the width and height of the current used viewport
        float glDimensions[4];
        glGetFloatv(GL_VIEWPORT, glDimensions);
        m_prevWidth  = (uint32)glDimensions[2];
        m_prevHeight = (uint32)glDimensions[3];

        // bind the render texture and frame buffer
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

        // setup the new viewport
        glViewport(0, 0, m_width, m_height);
        GetGraphicsManager()->SetRenderTexture(this);
    }


    // clear the render texture
    void RenderTexture::Clear(const MCore::RGBAColor& color)
    {
        glClearColor(color.m_r, color.m_g, color.m_b, color.m_a);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }


    // deactivate the render texture
    void RenderTexture::Deactivate()
    {
        // undbind the frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // reset viewport to original dimensions
        glViewport(0, 0, m_prevWidth, m_prevHeight);
        GetGraphicsManager()->SetRenderTexture(nullptr);
    }


    // initialize the render texture
    bool RenderTexture::Init(int32 format, uint32 width, uint32 height, AZ::u32 depthBuffer)
    {
        m_format      = format;
        m_width       = width;
        m_height      = height;
        m_depthBuffer = depthBuffer;

        // check if the graphics hardware is capable of rendering to textures, return false if not
        if (hasOpenGLFeature(Framebuffers))
        {
            return false;
        }

        // create surface IDs
        glGenFramebuffers(1, &m_frameBuffer);
        glGenTextures(1, &m_texture);

        // if the depth buffer was not specified, generate it
        if (m_depthBuffer == 0)
        {
            glGenRenderbuffers(1, &m_depthBuffer);
        }

        // check if initalization of the texture, the frame buffer and the depth buffer worked okay
        if (m_frameBuffer == 0 || m_depthBuffer == 0 || m_texture == 0)
        {
            MCore::LogWarning("[OpenGL] RenderTexture failed to init (5d, %d, %d)", m_frameBuffer, m_depthBuffer, m_texture);
            return false;
        }

        // create the frame buffer object
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

        // setup channels
        GLenum glChannels = GL_RGBA;
        if (m_format == GL_ALPHA16F_ARB || m_format == GL_ALPHA32F_ARB)
        {
            glChannels = GL_ALPHA;
        }

        // create render target
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_width, m_height, 0, glChannels, GL_FLOAT, nullptr);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

        // create depth buffer
        if (depthBuffer == 0)
        {
            glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width, m_height);
        }

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            MCore::LogWarning("[OpenGL] RenderTexture did not init correctly");
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }


    // render on screen as fullscreen quad
    void RenderTexture::Render()
    {
        const float w = static_cast<float>(GetWidth());
        const float h = static_cast<float>(GetHeight());

        // disable shaders
        GetGraphicsManager()->SetShader(nullptr);
        glDisable(GL_BLEND);

        // Setup ortho projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDepthMask(0);

        Deactivate();

        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GetID());

        // draw the quad
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
        glBindTexture(GL_TEXTURE_2D, 0);
    }
} // namespace RenderGL
