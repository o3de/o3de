/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LmbrCentral/Ai/NavigationSystemBus.h>
#include <AzCore/Component/Component.h>
#include <CrySystemBus.h>

namespace LmbrCentral
{
    /*!
    * System component that allows access to the navigation system
    */
    class NavigationSystemComponent
        : public AZ::Component
        , public NavigationSystemRequestBus::Handler
        , public CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(NavigationSystemComponent, "{3D27484B-00C4-4F3F-9605-2BF3E5C317FF}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("NavigationSystemService", 0x48446078));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("NavigationSystemService", 0x48446078));
        }

        ~NavigationSystemComponent() override {}

    private:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // NavigationSystemRequestBus
        NavRayCastResult RayCast(const AZ::Vector3& begin, const AZ::Vector3& direction, float maxDistance) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        void OnCrySystemShutdown(ISystem&) override;
        ////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral
