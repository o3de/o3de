/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

namespace AZ::RHI
{
    class ReflectContext;

    //! System Component in charge of registering the GraphicsProfilerBus into the BehaviorContext
    //! so it can be accessed from the scripting side.
    class GraphicsProfilerSystemComponent
        : public AZ::Component
    {
    public:
        GraphicsProfilerSystemComponent() = default;
        virtual ~GraphicsProfilerSystemComponent() = default;
        AZ_COMPONENT(GraphicsProfilerSystemComponent, "{75DEEB83-411F-41DF-9429-74AC2DEC8B9C}");

        static void Reflect(AZ::ReflectContext* context);

    private:
        void Activate() override {}
        void Deactivate() override {}
    };
}
