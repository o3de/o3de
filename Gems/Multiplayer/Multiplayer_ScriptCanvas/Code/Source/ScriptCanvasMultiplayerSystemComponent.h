/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace ScriptCanvasMultiplayer
{
    class ScriptCanvasMultiplayerSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ScriptCanvasMultiplayerSystemComponent, "{e332c2b8-df22-4dae-b105-09a97016aec5}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        
        // AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
    };
}
