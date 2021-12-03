/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LyShine/UiBase.h>
#include <LyShine/UiRenderFormats.h>

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

        //! End rendering to a texture
        virtual void EndRenderToTexture() = 0;

        //! Get a dynamic quad primitive that can be added as an image primitive to the render graph
        //! The graph handles the allocation of this DynUiPrimitive and deletes it when the graph is reset
        //! This can be used if the UI component doesn't want to own the storage of the primitive. Used infrequently,
        //! e.g. for the selection rect on a text component.
        virtual LyShine::UiPrimitive* GetDynamicQuadPrimitive(const AZ::Vector2* positions, uint32 packedColor) = 0;

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
