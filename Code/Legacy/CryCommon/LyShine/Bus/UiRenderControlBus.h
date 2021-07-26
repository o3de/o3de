/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace LyShine
{
    class IRenderGraph;
}

class UiElementInterface;
class UiRenderInterface;

////////////////////////////////////////////////////////////////////////////////////////////////////
//! The UiRenderControlBus is used for controlling the rendering of elements that affect the rendering
//! of their children.
//! An example use is a mask component that needs to setup stencil write before rendering its
//! components to increment the stencil buffer, switch to stencil test before rendering the child
//! elements and then do a second pass to decrement the stencil buffer.
//! The interface is designed to be flexible and could also be used for setting up scissoring or
//! rendering to a texture.
class UiRenderControlInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRenderControlInterface() {}

    //! This renders this element plus its children. It allows the RenderControl element to control
    //! the order in which the element's component and children are rendered and to change state
    //! at any point while rendering them.
    //! \param renderGraph, the render graph being added to
    //! \param elementInterface, pointer to the element interface for this element (for performance)
    //! \param renderInterface, pointer to the render interface for this element (for performance)
    //! \param numChildren, the number of child elements of this element
    //! \param isInGame, true if element being rendered in game (or preview), false if being render in edit mode
    virtual void Render(
        LyShine::IRenderGraph* renderGraph,
        UiElementInterface* elementInterface,
        UiRenderInterface* renderInterface,
        int numChildren,
        bool isInGame) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiRenderControlInterface> UiRenderControlBus;
