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
