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
#include <AzCore/Component/TransformBus.h>

#include "OccluderAreaComponentBus.h"

namespace Visibility
{
    class OccluderAreaConfiguration
    {
    public:
        AZ_TYPE_INFO(OccluderAreaConfiguration, "{F024EC7E-717F-576C-8C22-09CAFEFEAF29}");
        AZ_CLASS_ALLOCATOR(OccluderAreaConfiguration, AZ::SystemAllocator, 0);

        virtual ~OccluderAreaConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        bool m_displayFilled = true;
        float m_cullDistRatio = 100.0f;
        bool m_useInIndoors = false;
        bool m_doubleSide = true;

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

    class OccluderAreaComponent
        : public AZ::Component
        , private OccluderAreaRequestBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(OccluderAreaComponent, "{B3C90C5F-0F9B-5D4F-ABAE-6D16CB45CB5A}", AZ::Component);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provides);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& requires);
        static void Reflect(AZ::ReflectContext* context);

        OccluderAreaComponent() = default;
        explicit OccluderAreaComponent(const OccluderAreaConfiguration& params);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // OccluderAreaRequestBus
        bool GetDisplayFilled() override;
        float GetCullDistRatio() override;
        bool GetUseInIndoors() override;
        bool GetDoubleSide() override;

    protected:
        // Reflected Data
        OccluderAreaConfiguration m_config;
    };
} // namespace Visibility
