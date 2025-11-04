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

namespace LmbrCentral
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
                    "Shape Reference", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &ReferenceShapeConfig::m_shapeEntityId, "Shape Entity Id", "Entity with shape component to reference.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ShapeService"))
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
        services.push_back(AZ_CRC_CE("ShapeService"));
        services.push_back(AZ_CRC_CE("ReferenceShapeService"));
    }

    void ReferenceShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("ShapeService"));
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

        if (m_configuration.m_shapeEntityId.IsValid() && m_configuration.m_shapeEntityId != GetEntityId())
        {
            AZ::EntityBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
            AZ::TransformNotificationBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeEntityId);
        }
    }

    void ReferenceShapeComponent::Activate()
    {
        SetupDependencies();

        // Only connect to these after we've finished initializing everything else.
        ReferenceShapeRequestBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());

        // Finally, after everything is set up, broadcast out a "ShapeChanged" event. This is needed because the Editor version
        // of ReferenceShapeComponent will internally deactivate/activate a runtime version of this component on ShapeEntityId
        // changes instead of going through SetShapeEntityId. Other components may rely on knowing about shape changes, so the
        // activation needs to send out this event.
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void ReferenceShapeComponent::Deactivate()
    {
        // Disconnect from these first so that the component stops accepting new requests.
        LmbrCentral::ShapeComponentRequestsBus::Handler::BusDisconnect();
        ReferenceShapeRequestBus::Handler::BusDisconnect();

        AZ::EntityBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
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
        if (AllowNotification())
        {
            LmbrCentral::ShapeComponentNotificationsBus::Event(
                GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
                LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
    }

    void ReferenceShapeComponent::OnEntityDeactivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        if (AllowNotification())
        {
            LmbrCentral::ShapeComponentNotificationsBus::Event(
                GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
                LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
    }

    void ReferenceShapeComponent::OnTransformChanged([[maybe_unused]] const AZ::Transform& local, [[maybe_unused]] const AZ::Transform& world)
    {
        if (AllowNotification())
        {
            LmbrCentral::ShapeComponentNotificationsBus::Event(
                GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
                LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
        }
    }

    void ReferenceShapeComponent::OnShapeChanged([[maybe_unused]] LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        if (AllowNotification())
        {
            LmbrCentral::ShapeComponentNotificationsBus::Event(
                GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
                LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
    }

    AZ::Crc32 ReferenceShapeComponent::GetShapeType() const
    {
        AZ::Crc32 result = {};

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetShapeType);
        }

        return result;
    }

    AZ::Aabb ReferenceShapeComponent::GetEncompassingAabb() const
    {
        AZ::Aabb result = AZ::Aabb::CreateNull();

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        }

        return result;
    }

    void ReferenceShapeComponent::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const
    {
        transform = AZ::Transform::CreateIdentity();
        bounds = AZ::Aabb::CreateNull();

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::Event(m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds, transform, bounds);
        }
    }

    bool ReferenceShapeComponent::IsPointInside(const AZ::Vector3& point) const
    {
        bool result = false;

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point);
        }

        return result;
    }

    float ReferenceShapeComponent::DistanceFromPoint(const AZ::Vector3& point) const
    {
        float result = FLT_MAX;

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceFromPoint, point);
        }

        return result;
    }

    float ReferenceShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point) const
    {
        float result = FLT_MAX;

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint, point);
        }

        return result;
    }

    AZ::Vector3 ReferenceShapeComponent::GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) const
    {
        AZ::Vector3 result = AZ::Vector3::CreateZero();

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, randomDistribution);
        }

        return result;
    }

    bool ReferenceShapeComponent::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const
    {
        bool result = false;

        AZStd::shared_lock lock(m_mutex);
        if (AllowRequest())
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(result, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::IntersectRay, src, dir, distance);
        }

        return result;
    }

    bool ReferenceShapeComponent::AllowRequest() const
    {
        AZ_ErrorOnce(
            "Shape", !LmbrCentral::ShapeComponentRequestsBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with shape entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        return !LmbrCentral::ShapeComponentRequestsBus::HasReentrantEBusUseThisThread() && m_configuration.m_shapeEntityId.IsValid() &&
            m_configuration.m_shapeEntityId != GetEntityId();
    }

    bool ReferenceShapeComponent::AllowNotification() const
    {
        AZ_ErrorOnce(
            "Shape", !LmbrCentral::ShapeComponentNotificationsBus::HasReentrantEBusUseThisThread(),
            "Detected cyclic dependencies with shape entity references on entity '%s' (%s)", GetEntity()->GetName().c_str(),
            GetEntityId().ToString().c_str());

        return !LmbrCentral::ShapeComponentNotificationsBus::HasReentrantEBusUseThisThread() &&
            m_allowNotifications &&
            m_configuration.m_shapeEntityId.IsValid() &&
            m_configuration.m_shapeEntityId != GetEntityId();
    }

    AZ::EntityId ReferenceShapeComponent::GetShapeEntityId() const
    {
        AZStd::shared_lock lock(m_mutex);
        return m_configuration.m_shapeEntityId;
    }

    void ReferenceShapeComponent::SetShapeEntityId(AZ::EntityId entityId)
    {
        {
            AZStd::unique_lock lock(m_mutex);
            if (m_configuration.m_shapeEntityId != entityId)
            {
                m_configuration.m_shapeEntityId = entityId;
            }

            // Temporarily disable notifications so that we don't get an "Entity Activated" notification when setting up
            // the dependencies. The notification would both cause a redundant OnShapeChanged call and would be potentially
            // problematic because it would occur while still holding the unique_lock above. Instead we can just skip that
            // notification and send a single OnShapeChanged notification below.
            m_allowNotifications = false;
            SetupDependencies();
            m_allowNotifications = true;
        }

        // Broadcast out a "ShapeChanged" event.  In some cases, this might be excessive, but in the specific
        // case that the entity ID gets cleared out of this component in the Editor, there are no other events
        // that fire to notify upstream shape consumers that something has changed about the shape.
        LmbrCentral::ShapeComponentNotificationsBus::Event(
            GetEntityId(), &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }
}
