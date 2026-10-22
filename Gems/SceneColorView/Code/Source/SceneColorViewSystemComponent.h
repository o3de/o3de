/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <AzCore/Component/Component.h>

namespace SceneColorView
{
    class SceneColorViewSystemComponent final : public AZ::Component
    {
    public:
        AZ_COMPONENT(SceneColorViewSystemComponent, "{C134416E-85C5-449B-A1F3-8F73DB1650D6}");
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        void Activate() override;
        void Deactivate() override;
    };
}
