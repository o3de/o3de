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

#pragma once

///
/// Abstract all rendering for any connected HMD. This renderer takes care of managing the rendering-specific portions
/// of an HMD including: render target creation and destruction, social screen rendering, and frame submission.
///

#include <HMDBus.h>
#include <IStereoRenderer.h>

class CD3D9Renderer;
class CD3DStereoRenderer;
class CTexture;

class D3DHMDRenderer
{
    public:

        D3DHMDRenderer();
        ~D3DHMDRenderer();

        ///
        /// Initialize the renderer for use. This includes creating any internal render targets.
        ///
        /// @param hmdDevice The HMD device to use when creating render targets and submitting frames. The HMD device should remain valid
        ///                  the entire time that this renderer is in use.
        /// @param renderer The platform-specific renderer (e.g. DX11) to use for render target creation/deletion.
        /// @param stereoRenderer The platform-specific stereo renderer controlling this object. see IStereoRenderer.h for more info.
        ///
        /// @returns If true, the renderer successfully initialized and is ready to go.
        ///
        bool Initialize(CD3D9Renderer* renderer, CD3DStereoRenderer* stereoRenderer);

        ///
        /// Shutdown the renderer and free any associated data from it.
        ///
        void Shutdown();

        ///
        /// Calculate the backbuffer resolution necessary to display both eyes simultaneously to a device such as a PC monitor.
        ///
        /// @param eyeWidth The width (in pixels) of one eye.
        /// @param eyeHeight The height (in pixels) of one eye.
        /// @param backbufferWidth Calculated backbuffer width based on the eye width/height.
        /// @param backbufferHeight Calculated backbuffer height based on the eye width/height.
        ///
        void CalculateBackbufferResolution(int eyeWidth, int eyeHeight, int& backbufferWidth, int& backbufferHeight);

        ///
        /// Change the underlying buffers to match the new resolution after the renderer has resized.
        ///
        void OnResolutionChanged();

        ///
        /// Render the social screen to a connected display (e.g. a PC monitor).
        ///
        void RenderSocialScreen();

        ///
        /// Prepare the current frame for submission. This must be called before any call to SubmitFrame().
        ///
        void PrepareFrame();

        ///
        /// Submit the most recently rendered frame to the connected HMD device.
        ///
        void SubmitFrame();

    private:

        ///
        /// Resize the internal render targets based on the texture descriptor passed in.
        ///
        bool ResizeRenderTargets(const AZ::VR::HMDDeviceBus::TextureDesc& textureDesc);

        ///
        /// Free the internal render targets (including device targets).
        ///
        void FreeRenderTargets();

        ///
        /// Wrap a device render target into a CTexture object for easy access throughout the renderer.
        ///
        CTexture* WrapD3DRenderTarget(ID3D11Texture2D* d3dTexture, uint32 width, uint32 height, ETEX_Format format, const char* name, bool shaderResourceView);

        uint32 m_eyeWidth;      ///< Current width of an eye (in pixels).
        uint32 m_eyeHeight;     ///< Current height of an eye (in pixels).

        CD3D9Renderer* m_renderer;              ///< Platform-specific rendering device.
        CD3DStereoRenderer* m_stereoRenderer;   ///< Platform-specific stereo rendering device controlling this object.

        ///
        /// Per-eye render target info.
        ///
        struct EyeRenderTarget
        {
            AZ::VR::HMDRenderTarget deviceRenderTarget;     ///< Device-controlled swap chain for rendering to.
            TArray<CTexture*> textureChain;                 ///< Texture references to the internal swap chain.

            Vec2i viewportPosition;     ///< Position of the per-eye viewport (in pixels).
            Vec2i viewportSize;         ///< Size of the per-eye viewport (in pixels).
        };

        EyeRenderTarget m_eyes[STEREO_EYE_COUNT];     ///< Device render targets to be rendered to and submitted to the HMD for display.

        bool m_framePrepared;       ///< If true, PrepareFrame() and SubmitFrame() were called in the proper ordering (just for debugging purproses).
};