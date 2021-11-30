/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_GBUFFER_H
#define __RENDERGL_GBUFFER_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Color.h>
#include "RenderGLConfig.h"

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>

namespace RenderGL
{
    // forward declarations
    class GLSLShader;
    class RenderTexture;

    /**
     *
     *
     */
    class RENDERGL_API GBuffer
        : private QOpenGLExtraFunctions
    {
    public:
        enum EComponent
        {
            COMPONENT_SHADED    = 0,
            COMPONENT_GLOW      = 1,
            NUM_COMPONENTS
        };

        GBuffer();
        ~GBuffer();

        bool Init(uint32 width, uint32 height);
        void Release();

        void Activate();
        void Deactivate();
        void Clear(const MCore::RGBAColor& color);

        void Render();

        MCORE_INLINE uint32 GetTextureID(EComponent component) const    { return m_components[component]; }

        bool Resize(uint32 width, uint32 height);
        bool ResizeTextures(uint32 screenWidth, uint32 screenHeight);

        MCORE_INLINE RenderTexture* GetRenderTargetA()                  { return m_renderTargetA; }
        MCORE_INLINE RenderTexture* GetRenderTargetB()                  { return m_renderTargetB; }
        MCORE_INLINE RenderTexture* GetRenderTargetC()                  { return m_renderTargetC; }
        MCORE_INLINE RenderTexture* GetRenderTargetD()                  { return m_renderTargetD; }
        MCORE_INLINE RenderTexture* GetRenderTargetE()                  { return m_renderTargetE; }


    private:
        uint32  m_fbo;
        uint32  m_components[NUM_COMPONENTS];
        uint32  m_depthBufferId;
        uint32  m_width;
        uint32  m_height;

        RenderTexture*      m_renderTargetA; /**< A temp render target. */
        RenderTexture*      m_renderTargetB; /**< A temp render target. */
        RenderTexture*      m_renderTargetC; /**< A temp render target. */
        RenderTexture*      m_renderTargetD; /**< Render target with width and height divided by four. */
        RenderTexture*      m_renderTargetE; /**< Render target with width and height divided by four. */
    };
}   // namespace RenderGL


#endif
