/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include <MCore/Source/Config.h>
#include <MCore/Source/LogManager.h>

#include <AzCore/std/containers/vector.h>
#include "GBuffer.h"
#include "RenderTexture.h"
#include "GLSLShader.h"
#include "GraphicsManager.h"
#include "TextureCache.h"

namespace RenderGL
{
    // constructor
    GBuffer::GBuffer()
    {
        m_fbo            = 0;
        m_depthBufferId  = 0;
        m_width          = 100;
        m_height         = 100;

        m_renderTargetA  = nullptr;
        m_renderTargetB  = nullptr;
        m_renderTargetC  = nullptr;
        m_renderTargetD  = nullptr;
        m_renderTargetE  = nullptr;

        for (uint32 i = 0; i < NUM_COMPONENTS; ++i)
        {
            m_components[i] = 0;
        }
    }


    // destructor
    GBuffer::~GBuffer()
    {
        Release();
    }


    // initialize
    bool GBuffer::Init(uint32 width, uint32 height)
    {
        initializeOpenGLFunctions();
        Release();

        // get the maximum number of draw buffers
        GLint maxBuffers = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxBuffers);
        //MCore::LogDetailedInfo("[OpenGL] Maximum draw buffers = %d", maxBuffers);
        if (maxBuffers < 2)
        {
            return false;
        }

        m_width  = width;
        m_height = height;

        // create the FBO
        glGenFramebuffers(1, &m_fbo);
        for (uint32 i = 0; i < NUM_COMPONENTS; ++i)
        {
            glGenTextures(1, &m_components[i]);
        }

        glGenTextures(1, &m_depthBufferId);

        // bind the fbo
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // init normals texture
        glBindTexture(GL_TEXTURE_2D, m_components[COMPONENT_SHADED]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_components[COMPONENT_SHADED], 0);

        // init glow color texture
        glBindTexture(GL_TEXTURE_2D, m_components[COMPONENT_GLOW]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_components[COMPONENT_GLOW], 0);

        // init the depth buffer
        glBindTexture(GL_TEXTURE_2D, m_depthBufferId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthBufferId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthBufferId, 0);

        // unbind
        glBindTexture(GL_TEXTURE_2D, 0);

        // make sure it all works
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            MCore::LogWarning("[OpenGL] GBuffer did not init correctly");
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // create the render textures
        if (ResizeTextures(width, height) == false)
        {
            MCore::LogWarning("[OpenGL] GBuffer resizing of textures failed");
            return false;
        }

        return true;
    }


    // release
    void GBuffer::Release()
    {
        if (m_fbo)
        {
            glDeleteFramebuffers(1, &m_fbo);
            for (uint32 i = 0; i < NUM_COMPONENTS; ++i)
            {
                glDeleteTextures(1, &m_components[i]);
            }

            glDeleteTextures(1, &m_depthBufferId);
        }

        delete m_renderTargetA;
        delete m_renderTargetB;
        delete m_renderTargetC;
        delete m_renderTargetD;
        delete m_renderTargetE;

        m_renderTargetA = nullptr;
        m_renderTargetB = nullptr;
        m_renderTargetC = nullptr;
        m_renderTargetD = nullptr;
        m_renderTargetE = nullptr;
    }


    // resize the gbuffer
    bool GBuffer::Resize(uint32 width, uint32 height)
    {
        // init the gbuffer
        return Init(width, height);
    }


    // resize render textures
    bool GBuffer::ResizeTextures(uint32 screenWidth, uint32 screenHeight)
    {
        delete m_renderTargetA;
        delete m_renderTargetB;
        delete m_renderTargetC;
        delete m_renderTargetD;
        delete m_renderTargetE;

        m_renderTargetA  = new RenderTexture();
        m_renderTargetB  = new RenderTexture();
        m_renderTargetC  = new RenderTexture();
        m_renderTargetD  = new RenderTexture();
        m_renderTargetE  = new RenderTexture();

        if (m_renderTargetA->Init(GL_RGBA16F, screenWidth, screenHeight) == false)
        {
            return false;
        }

        if (m_renderTargetB->Init(GL_RGBA16F, screenWidth, screenHeight) == false)
        {
            return false;
        }

        if (m_renderTargetC->Init(GL_RGBA16F, screenWidth, screenHeight) == false)
        {
            return false;
        }

        if (m_renderTargetD->Init(GL_RGBA16F, screenWidth / 2, screenHeight / 2) == false)
        {
            return false;
        }

        if (m_renderTargetE->Init(GL_RGBA16F, screenWidth / 2, screenHeight / 2) == false)
        {
            return false;
        }

        return true;
    }


    // activate
    void GBuffer::Activate()
    {
        glPushAttrib(GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);

        // bind the render texture and frame buffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, bufs);

        GLenum code = glGetError();
        if (code != GL_NO_ERROR)
        {
            //MCore::LogWarning("[OpenGL] GBuffer::Activate - glDrawBuffers call returns an error (%d)", code);
        }

        // setup the new viewport
        glViewport(0, 0, m_width, m_height);
    }


    // clear
    void GBuffer::Clear(const MCore::RGBAColor& color)
    {
        glClearColor(color.m_r, color.m_g, color.m_b, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }


    // deactivate
    void GBuffer::Deactivate()
    {
        glPopAttrib();

        // undbind the frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE0, 0);
    }


    // render the textures
    void GBuffer::Render()
    {
        glUseProgram(0);
        glEnable(GL_TEXTURE_2D);
        glActiveTexture(GL_TEXTURE0);

        //-----------------------------
        // render the main image
        //-----------------------------
        glBindTexture(GL_TEXTURE_2D, m_components[COMPONENT_SHADED]);

        // setup ortho projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, m_width, m_height, 0, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_DEPTH_TEST);
        glDepthMask(0);

        // Draw quad
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        glColor3f(1.0f, 1.0f, 1.0f);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(static_cast<float>(m_width), 0.0f);
        glColor3f(1.0f, 1.0f, 1.0f);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(static_cast<float>(m_width), static_cast<float>(m_height));
        glColor3f(1.0f, 1.0f, 1.0f);

        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, static_cast<float>(m_height));
        glColor3f(1.0f, 1.0f, 1.0f);
        glEnd();

        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);
        glBindTexture(GL_TEXTURE_2D, 0);
        //-----------------------------


        //-----------------------------
        // render the small images
        //-----------------------------
        uint32 xStart = 10;
        uint32 yStart = m_height - 110;

        for (uint32 i = 0; i < NUM_COMPONENTS; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, m_components[i]);

            const float w = static_cast<float>(m_width);
            const float h = static_cast<float>(m_height);

            // Setup ortho projection
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, w, h, 0, -1, 1);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glDisable(GL_DEPTH_TEST);
            glDepthMask(0);

            // Draw quad
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(static_cast<float>(xStart), static_cast<float>(yStart));
            glColor3f(1.0f, 1.0f, 1.0f);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(static_cast<float>(xStart) + 150, static_cast<float>(yStart));
            glColor3f(1.0f, 1.0f, 1.0f);

            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(static_cast<float>(xStart) + 150, static_cast<float>(yStart) + 100);
            glColor3f(1.0f, 1.0f, 1.0f);

            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(static_cast<float>(xStart), static_cast<float>(yStart) + 100);
            glColor3f(1.0f, 1.0f, 1.0f);
            glEnd();

            glEnable(GL_DEPTH_TEST);
            glDepthMask(1);

            glBindTexture(GL_TEXTURE_2D, 0);

            xStart += 160;
        }
    }
}   // namespace RenderGL
