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

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiRenderInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiRenderInterface() {}

    //! Render the component into the render graph
    virtual void Render(LyShine::IRenderGraph* renderGraph) = 0;

public: // static member data

    //! Multiple components on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
};

typedef AZ::EBus<UiRenderInterface> UiRenderBus;
