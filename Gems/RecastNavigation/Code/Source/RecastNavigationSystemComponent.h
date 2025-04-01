/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <RecastNavigation/RecastNavigationBus.h>

namespace RecastNavigation
{
    class RecastNavigationSystemComponent
        : public AZ::Component
        , protected RecastNavigationRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(RecastNavigationSystemComponent, "{9d7ae509-b1db-4889-98bb-941a3f672ca3}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        RecastNavigationSystemComponent();
        ~RecastNavigationSystemComponent() override;

    protected:
        //! AZ::Component overrides ...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        //! AZTickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    };

} // namespace RecastNavigation
