/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReferenceShapeComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Vegetation
{
    void ReferenceShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ReferenceShapeConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ShapeEntityId", &ReferenceShapeConfig::m_shapeEntityId)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ReferenceShapeConfig>(
                    "Vegetation Reference Shape", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &ReferenceShapeConfig::m_shapeEntityId, "Shape Entity Id", "Entity with shape component to reference.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ShapeService", 0xe86aa5fe))
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ReferenceShapeConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("shapeEntityId", BehaviorValueProperty(&ReferenceShapeConfig::m_shapeEntityId))
                ;
        }
    }

    void ReferenceShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
        services.push_back(AZ_CRC_CE("ReferenceShapeService"));
    }

    void ReferenceShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("ShapeService", 0xe86aa5fe));
    }

    void ReferenceShapeComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void ReferenceShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        ReferenceShapeConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ReferenceShapeComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &ReferenceShapeComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("ReferenceShapeComponentTypeId", BehaviorConstant(ReferenceShapeComponentTypeId));

            behaviorContext->Class<ReferenceShapeComponent>()->RequestBus("ReferenceShapeRequestBus");

            behaviorContext->EBus<ReferenceShapeRequestBus>("ReferenceShapeRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetShapeEntityId", &ReferenceShapeRequestBus::Events::GetShapeEntityId)
                ->Event("SetShapeEntityId", &ReferenceShapeRequestBus::Events::SetShapeEntityId)
                ->VirtualProperty("ShapeEntityId", "GetShapeEntityId", "SetShapeEntityId")
                ;
        }
    }

    ReferenceShapeComponent::ReferenceShapeComponent(const ReferenceShapeConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void ReferenceShapeComponent::SetupDependencies()
    {
        AZ::EntityBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());

        if (m_configuration.m_shapeEntityId.IsValid() && m_configuration.m_shapeEntityId != GetEntityId())
        {
            AZ::EntityBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
            AZ::TransformNotificationBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
        }

        // Broadcast out a "ShapeChanged" event.  In some cases, this might be excessive, but in the specific
        // case that the entity ID gets cleared out of this component in the Editor, there are no other events
        // that fire to notify upstream shape consumers that something has changed about the shape.
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void ReferenceShapeComponent::Activate()
    {
        SetupDependencies();
        ReferenceShapeRequestBus::Handler::BusConnect(GetEntityId());
    }

    void ReferenceShapeComponent::Deactivate()
    {
        AZ::EntityBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        ReferenceShapeRequestBus::Handler::BusDisconnect();
    }

    bool ReferenceShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const ReferenceShapeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool ReferenceShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<ReferenceShapeConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void ReferenceShapeComponent::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void ReferenceShapeComponent::OnEntityDeactivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void ReferenceShapeComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    void ReferenceShapeComponent::OnShapeChanged([[maybe_unused]] LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    AZ::Crc32 ReferenceShapeComponent::GetShapeType()
    {
        AZ::Crc32 result = {};

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);
            m_isRequestInProgress = false;
        }

        return result;
    }

    AZ::Aabb ReferenceShapeComponent::GetEncompassingAabb()
    {
        AZ::Aabb result = AZ::Aabb::CreateNull();

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            m_isRequestInProgress = false;
        }

        return result;
    }

    void ReferenceShapeComponent::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        transform = AZ::Transform::CreateIdentity();
        bounds = AZ::Aabb::CreateNull();

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::Event(m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, transform, bounds);
            m_isRequestInProgress = false;
        }
    }

    bool ReferenceShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        bool result = false;

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point);
            m_isRequestInProgress = false;
        }

        return result;
    }

    float ReferenceShapeComponent::DistanceFromPoint(const AZ::Vector3& point)
    {
        float result = FLT_MAX;

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceFromPoint, point);
            m_isRequestInProgress = false;
        }

        return result;
    }

    float ReferenceShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        float result = FLT_MAX;

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint, point);
            m_isRequestInProgress = false;
        }

        return result;
    }

    AZ::Vector3 ReferenceShapeComponent::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution)
    {
        AZ::Vector3 result = AZ::Vector3::CreateZero();

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, randomDistribution);
            m_isRequestInProgress = false;
        }

        return result;
    }

    bool ReferenceShapeComponent::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        bool result = false;

        AZ_WarningOnce("Vegetation", !m_isRequestInProgress, "Detected cyclic dependences with vegetation entity references");
        if (AllowRequest())
        {
            m_isRequestInProgress = true;
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, src, dir, distance);
            m_isRequestInProgress = false;
        }

        return result;
    }

    bool ReferenceShapeComponent::AllowRequest() const
    {
        return !m_isRequestInProgress && m_configuration.m_shapeEntityId.IsValid() && m_configuration.m_shapeEntityId != GetEntityId();
    }

    AZ::EntityId ReferenceShapeComponent::GetShapeEntityId() const
    {
        return m_configuration.m_shapeEntityId;
    }

    void ReferenceShapeComponent::SetShapeEntityId(AZ::EntityId entityId)
    {
        if(m_configuration.m_shapeEntityId != entityId)
        {
            m_configuration.m_shapeEntityId = entityId;
            SetupDependencies();
        }
    }
}
