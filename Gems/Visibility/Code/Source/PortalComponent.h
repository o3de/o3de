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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>

#include "PortalComponentBus.h"

namespace Visibility
{
    class PortalConfiguration
    {
    public:
        AZ_TYPE_INFO(PortalConfiguration, "{B9BDF017-DF8B-5DBE-8A92-5F62B7AECC1C}");
        AZ_CLASS_ALLOCATOR(PortalConfiguration, AZ::SystemAllocator,0);
        
        virtual ~PortalConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);
        static bool VersionConverter(
            AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        float m_height = 1.0f;
        bool m_displayFilled = false;
        bool m_affectedBySun = false;
        float m_viewDistRatio = 100.0f;
        bool m_skyOnly = false;
        bool m_oceanIsVisible = false;
        bool m_useDeepness = true;
        bool m_doubleSide = true;
        bool m_lightBlending = true;
        float m_lightBlendValue = 0.5f;

        AZStd::array<AZ::Vector3, 4> m_vertices =
        AZStd::array<AZ::Vector3, 4> { {
            AZ::Vector3(-1.0f, -1.0f, 0.0f),
            AZ::Vector3( 1.0f, -1.0f, 0.0f),
            AZ::Vector3( 1.0f,  1.0f, 0.0f),
            AZ::Vector3(-1.0f,  1.0f, 0.0f)
        } };

        virtual void OnChange() {}
        virtual void OnVerticesChange() {}
    };

    class PortalComponent
        : public AZ::Component
        , private PortalRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PortalComponent, "{89F1DD88-4445-5A9D-9223-6D4D8D44E6AC}", AZ::Component);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);    

        PortalComponent() = default;
        explicit PortalComponent(const PortalConfiguration& params);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // PortalRequestBus
        float GetHeight() override;
        bool GetDisplayFilled() override;
        bool GetAffectedBySun() override;
        float GetViewDistRatio() override;
        bool GetSkyOnly() override;
        bool GetOceanIsVisible() override;
        bool GetUseDeepness() override;
        bool GetDoubleSide() override;
        bool GetLightBlending() override;
        float GetLightBlendValue() override;

    protected:
        // Reflected Data
        PortalConfiguration m_config;
    };
} // namespace Visibility
