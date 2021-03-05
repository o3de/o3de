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
