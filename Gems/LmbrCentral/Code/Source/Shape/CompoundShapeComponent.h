/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>

namespace LmbrCentral
{
    class CompoundShapeComponent
        : public AZ::Component
        , private ShapeComponentRequestsBus::Handler
        , private ShapeComponentNotificationsBus::MultiHandler
        , private CompoundShapeComponentRequestsBus::Handler
        , public AZ::EntityBus::MultiHandler
    {
    public:
        friend class EditorCompoundShapeComponent;

        AZ_COMPONENT(CompoundShapeComponent, "{C0C817DE-843F-44C8-9FC1-989CDE66B662}");

        CompoundShapeComponent() : 
            m_currentlyActiveChildren(0)
        {}

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        
        // ShapeComponent::Handler implementation
        AZ::Crc32 GetShapeType() override
        {
            return AZ::Crc32("Compound");
        }

        AZ::Aabb GetEncompassingAabb() override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) override;
        bool IsPointInside(const AZ::Vector3& point) override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) override;
        
        // CompoundShapeComponentRequestsBus::Handler implementation
        CompoundShapeConfiguration GetCompoundShapeConfiguration() override
        {
            return m_configuration;
        }

        // EntityEvents
        void OnEntityActivated(const AZ::EntityId&) override;
        void OnEntityDeactivated(const AZ::EntityId&) override;

        // ShapeComponentNotificationsBus::MultiHandler
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons) override;
    
    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            provided.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
            incompatible.push_back(AZ_CRC("CompoundShapeService", 0x4f7c640a));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        CompoundShapeConfiguration m_configuration; ///< Stores configuration for this component.
        int m_currentlyActiveChildren; ///< Number of compound shape children shape entities that are currently active.
    };
} // namespace LmbrCentral
