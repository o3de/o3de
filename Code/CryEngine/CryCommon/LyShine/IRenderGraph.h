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

#include <IRenderer.h>
#include <LyShine/UiBase.h>

namespace AZ
{
    class Color;
    class Vector2;
}

namespace LyShine
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // UI visual components use this interface to add primitives to the render graph, which is how the
    // UI gets rendered.
    // There is one render graph per UI canvas. The render graph (like a display list) is rebuilt when
    // any visual change occurs on the canvas.
    class IRenderGraph
    {
    public:

        //! Virtual destructor
        virtual ~IRenderGraph() {}

        //---- Functions for creating and adding primitives to the render graph ----

        //! Begin the setup of a mask render node, primitives added between this call and StartChildrenForMask define the mask
        virtual void BeginMask(bool isMaskingEnabled, bool useAlphaTest, bool drawBehind, bool drawInFront) = 0;

        //! Start defining the children (masked primitives) of a mask
        virtual void StartChildrenForMask() = 0;

        //! End the setup of a mask render node, this marks the end of adding child primitives
        virtual void EndMask() = 0;

        //! Begin rendering to a texture
        virtual void BeginRenderToTexture(int renderTargetHandle, SDepthTexture* renderTargetDepthSurface,
            const AZ::Vector2& viewportTopLeft, const AZ::Vector2& viewportSize, const AZ::Color& clearColor) = 0;

        //! End rendering to a texture
        virtual void EndRenderToTexture() = 0;

        //! Add an indexed triangle list primitive to the render graph with given render state
        virtual void AddPrimitive(IRenderer::DynUiPrimitive* primitive, ITexture* texture,
            bool isClampTextureMode, bool isTextureSRGB, bool isTexturePremultipliedAlpha, BlendMode blendMode) = 0;
        
        //! Add an indexed triangle list primitive to the render graph which will use maskTexture as an alpha (gradient) mask
        virtual void AddAlphaMaskPrimitive(IRenderer::DynUiPrimitive* primitive,
            ITexture* texture, ITexture* maskTexture,
            bool isClampTextureMode, bool isTextureSRGB, bool isTexturePremultipliedAlpha, BlendMode blendMode) = 0;
        
        //! Get a dynamic quad primitive that can be added as an image primitive to the render graph
        //! The graph handles the allocation of this DynUiPrimitive and deletes it when the graph is reset
        //! This can be used if the UI component doesn't want to own the storage of the primitive. Used infrequently,
        //! e.g. for the selection rect on a text component.
        virtual IRenderer::DynUiPrimitive* GetDynamicQuadPrimitive(const AZ::Vector2* positions, uint32 packedColor) = 0;

        //---- Functions for supporting masking (used during creation of the graph, not rendering ) ----

        //! Get flag that indicates we are rendering into a mask. Used to avoid masks on child mask elements.
        virtual bool IsRenderingToMask() const = 0;

        //! Set flag that we are rendering into a mask. Used to avoid masks on child mask elements.
        virtual void SetIsRenderingToMask(bool isRenderingToMask) = 0;

        //---- Functions for supporting fading  (used during creation of the graph, not rendering ) ----

        //! Push an alpha fade, this is multiplied with any existing alpha fade from parents
        virtual void PushAlphaFade(float alphaFadeValue) = 0;

        //! Push a new alpha fade value, this replaces any existing alpha fade
        virtual void PushOverrideAlphaFade(float alphaFadeValue) = 0;

        //! Pop an alpha fade off the stack
        virtual void PopAlphaFade() = 0;

        //! Get the current alpha fade value
        virtual float GetAlphaFade() const = 0;
    };
}
