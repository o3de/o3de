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
#include <MCore/Source/LogManager.h>
#include "GLInclude.h"

#include "RenderTexture.h"
#include "GraphicsManager.h"


namespace RenderGL
{
    // constructor
    RenderTexture::RenderTexture()
    {
        mFormat         = 0;
        mWidth          = 0;
        mHeight         = 0;
        mFrameBuffer    = 0;
        mDepthBuffer    = 0;
        mTexture        = 0;
    }


    // destructor
    RenderTexture::~RenderTexture()
    {
        glDeleteTextures(1, &mTexture);
        glDeleteRenderbuffersEXT(1, &mDepthBuffer);
        glDeleteFramebuffersEXT(1, &mFrameBuffer);
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
        mPrevWidth  = (uint32)glDimensions[2];
        mPrevHeight = (uint32)glDimensions[3];

        // bind the render texture and frame buffer
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFrameBuffer);

        // setup the new viewport
        glViewport(0, 0, mWidth, mHeight);
        GetGraphicsManager()->SetRenderTexture(this);
    }


    // clear the render texture
    void RenderTexture::Clear(const MCore::RGBAColor& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }


    // deactivate the render texture
    void RenderTexture::Deactivate()
    {
        // undbind the frame buffer
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        // reset viewport to original dimensions
        glViewport(0, 0, mPrevWidth, mPrevHeight);
        GetGraphicsManager()->SetRenderTexture(nullptr);
    }


    // initialize the render texture
    bool RenderTexture::Init(int32 format, uint32 width, uint32 height, AZ::u32 depthBuffer)
    {
        mFormat      = format;
        mWidth       = width;
        mHeight      = height;
        mDepthBuffer = depthBuffer;

        // check if the graphics hardware is capable of rendering to textures, return false if not
        if (!GLEW_EXT_framebuffer_object)
        {
            return false;
        }

        // create surface IDs
        glGenFramebuffersEXT(1, &mFrameBuffer);
        glGenTextures(1, &mTexture);

        // if the depth buffer was not specified, generate it
        if (mDepthBuffer == 0)
        {
            glGenRenderbuffersEXT(1, &mDepthBuffer);
        }

        // check if initalization of the texture, the frame buffer and the depth buffer worked okay
        if (mFrameBuffer == 0 || mDepthBuffer == 0 || mTexture == 0)
        {
            MCore::LogWarning("[OpenGL] RenderTexture failed to init (5d, %d, %d)", mFrameBuffer, mDepthBuffer, mTexture);
            return false;
        }

        // create the frame buffer object
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mFrameBuffer);

        // setup channels
        GLenum glChannels = GL_RGBA;
        if (mFormat == GL_ALPHA16F_ARB || mFormat == GL_ALPHA32F_ARB)
        {
            glChannels = GL_ALPHA;
        }

        // create render target
        glBindTexture(GL_TEXTURE_2D, mTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, mFormat, mWidth, mHeight, 0, glChannels, GL_FLOAT, nullptr);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, mTexture, 0);

        // create depth buffer
        if (depthBuffer == 0)
        {
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, mDepthBuffer);
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, mWidth, mHeight);
        }

        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepthBuffer);

        if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            MCore::LogWarning("[OpenGL] RenderTexture did not init correctly");
            return false;
        }

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
