/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompoundShapeComponent.h"
#include <AzCore/Math/Transform.h>


namespace LmbrCentral
{
    void CompoundShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        CompoundShapeConfiguration::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CompoundShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &CompoundShapeComponent::m_configuration)
                ;
        }
    }

    void CompoundShapeComponent::Activate()
    {
        for (const AZ::EntityId& childEntity : m_configuration.GetChildEntities())
        {
            AZ::EntityBus::MultiHandler::BusConnect(childEntity);
        }

        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        CompoundShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void CompoundShapeComponent::Deactivate()
    {
        AZ::EntityBus::MultiHandler::BusDisconnect();
        CompoundShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentNotificationsBus::MultiHandler::BusDisconnect();
    }

    //////////////////////////////////////////////////////////////////////////

    AZ::Aabb CompoundShapeComponent::GetEncompassingAabb()
    {
        AZ::Aabb finalAabb = AZ::Aabb::CreateNull();

        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            AZ::Aabb childAabb = AZ::Aabb::CreateNull();
            EBUS_EVENT_ID_RESULT(childAabb, childEntity, ShapeComponentRequestsBus, GetEncompassingAabb);
            if (childAabb.IsValid())
            {
                finalAabb.AddAabb(childAabb);
            }
        }
        return finalAabb;
    }

    void CompoundShapeComponent::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds)
    {
        transform = AZ::Transform::CreateIdentity();
        bounds = AZ::Aabb::CreateNull();

        // Get the transform for the compound shape itself.
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        // Get the inverse transform that we'll use for transforming the childBounds from world space back to the parent-relative space.
        const AZ::Transform inverseTransform = transform.GetInverse();

        // Build up the local bounds by computing it from the local bounds of all the children entities.
        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            AZ::Transform childTransform = AZ::Transform::CreateIdentity();
            AZ::Aabb childBounds = AZ::Aabb::CreateNull();

            ShapeComponentRequestsBus::Event(
                childEntity,
                &ShapeComponentRequestsBus::Events::GetTransformAndLocalBounds,
                childTransform, childBounds);

            if (childBounds.IsValid())
            {
                // Transform the childBounds to world space, then back to the local space of the Compound Shape.
                // The net result is a local bounds that contains all the child local bounds in their relative positions to the Compound
                // Shape's entity position.
                childBounds.ApplyTransform(childTransform);
                childBounds.ApplyTransform(inverseTransform);
                bounds.AddAabb(childBounds);

            }
        }
    }

    bool CompoundShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        bool result = false;
        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            EBUS_EVENT_ID_RESULT(result, childEntity, ShapeComponentRequestsBus, IsPointInside, point);
            if (result)
            {
                break;
            }
        }
        return result;
    }

    float CompoundShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        float smallestDistanceSquared = FLT_MAX;
        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            float currentDistanceSquared = FLT_MAX;
            EBUS_EVENT_ID_RESULT(currentDistanceSquared, childEntity, ShapeComponentRequestsBus, DistanceSquaredFromPoint, point);
            if (currentDistanceSquared < smallestDistanceSquared)
            {
                smallestDistanceSquared = currentDistanceSquared;
            }
        }
        return smallestDistanceSquared;
    }

    bool CompoundShapeComponent::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance)
    {
        bool intersection = false;
        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            ShapeComponentRequestsBus::EventResult(intersection, childEntity, &ShapeComponentRequests::IntersectRay, src, dir, distance);
            if (intersection)
            {
                return true;
            }
        }

        return false;
    }

    void CompoundShapeComponent::OnEntityActivated(const AZ::EntityId& id)
    {
        m_currentlyActiveChildren++;
        ShapeComponentNotificationsBus::MultiHandler::BusConnect(id);

        if (ShapeComponentRequestsBus::Handler::BusIsConnected() && CompoundShapeComponentRequestsBus::Handler::BusIsConnected())
        {
            EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
    }

    void CompoundShapeComponent::OnEntityDeactivated(const AZ::EntityId& id)
    {
        m_currentlyActiveChildren--;
        ShapeComponentNotificationsBus::MultiHandler::BusDisconnect(id);
        EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CompoundShapeComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
        else if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::TransformChanged)
        {
            // If there are multiple shapes in a compound shape, then moving one of them changes the overall compound shape, otherwise the transform change is bubbled up directly
            EBUS_EVENT_ID(GetEntityId(), ShapeComponentNotificationsBus, OnShapeChanged, (m_currentlyActiveChildren > 1) ? ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged
                          : ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
        }
    }

    //////////////////////////////////////////////////////////////////////////
} // namespace LmbrCentral
