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

#include <LyShine/IDraw2d.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Interface used by UI components to render to the canvas
//
//! The IUiRenderer provides helper functions for UI rendering and also manages state that
//! persists between UI elements when rendering a UI canvas.
//! For example one UI component can turn on stencil test and that affects all UI rendering
//! until it is turned off.
//!
//! This is a singleton class that is accessed via IUiRenderer::Get() which is a shortcut for
//!     gEnv->pLyShine()->GetUiRenderer();
class IUiRenderer
{
public: // types


public: // member functions

    //! Implement virtual destructor for safety.
    virtual ~IUiRenderer() {}

    //! Start the rendering of a UI canvas
    virtual void BeginCanvasRender(AZ::Vector2 viewportSize) = 0;

    //! End the rendering of a UI canvas
    virtual void EndCanvasRender() = 0;

    //! Get the current base state
    virtual int GetBaseState() = 0;

    //! Set the base state
    virtual void SetBaseState(int state) = 0;

    //! Get the current stencil test reference value
    virtual uint32 GetStencilRef() = 0;

    //! Set the stencil test reference value
    virtual void SetStencilRef(uint32) = 0;

    //! Increment the current stencil reference value
    virtual void IncrementStencilRef() = 0;

    //! Decrement the current stencil reference value
    virtual void DecrementStencilRef() = 0;

    //! Get flag that indicates we are rendering into a mask. Used to avoid masks on child mask elements.
    virtual bool IsRenderingToMask() = 0;

    //! Set flag that we are rendering into a mask. Used to avoid masks on child mask elements.
    virtual void SetIsRenderingToMask(bool isRenderingToMask) = 0;

    //! Push an alpha fade, this is multiplied with any existing alpha fade from parents
    virtual void PushAlphaFade(float alphaFadeValue) = 0;

    //! Pop an alpha fade off the stack
    virtual void PopAlphaFade() = 0;

    //! Get the current alpha fade value
    virtual float GetAlphaFade() const = 0;

public: // static member functions

    //! Helper function to get the singleton UiRenderer
    static IUiRenderer* Get() { return gEnv->pLyShine->GetUiRenderer(); }
};
