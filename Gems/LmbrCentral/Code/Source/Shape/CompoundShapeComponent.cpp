/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CompoundShapeComponent.h"
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    void CompoundShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CompoundShapeConfiguration>()
                ->Version(1)
                ->Field("Child Shape Entities", &CompoundShapeConfiguration::m_childEntities);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CompoundShapeConfiguration>("Configuration", "Compound shape configuration parameters")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CompoundShapeConfiguration::m_childEntities,
                        "Child Shape Entities", "A list of entities that have shapes on them which when combined, act as the compound shape")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->ElementAttribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ShapeService"));
            }
        }
    }

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

    AZ::Aabb CompoundShapeComponent::GetEncompassingAabb() const
    {
        AZ::Aabb finalAabb = AZ::Aabb::CreateNull();

        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            AZ::Aabb childAabb = AZ::Aabb::CreateNull();
            ShapeComponentRequestsBus::EventResult(childAabb, childEntity, &ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (childAabb.IsValid())
            {
                finalAabb.AddAabb(childAabb);
            }
        }
        return finalAabb;
    }

    void CompoundShapeComponent::GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const
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

    bool CompoundShapeComponent::IsPointInside(const AZ::Vector3& point) const
    {
        bool result = false;
        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            ShapeComponentRequestsBus::EventResult(result, childEntity, &ShapeComponentRequestsBus::Events::IsPointInside, point);
            if (result)
            {
                break;
            }
        }
        return result;
    }

    float CompoundShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point) const
    {
        float smallestDistanceSquared = FLT_MAX;
        for (AZ::EntityId childEntity : m_configuration.GetChildEntities())
        {
            float currentDistanceSquared = FLT_MAX;
            ShapeComponentRequestsBus::EventResult(
                currentDistanceSquared, childEntity, &ShapeComponentRequestsBus::Events::DistanceSquaredFromPoint, point);
            if (currentDistanceSquared < smallestDistanceSquared)
            {
                smallestDistanceSquared = currentDistanceSquared;
            }
        }
        return smallestDistanceSquared;
    }

    bool CompoundShapeComponent::IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const
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
            ShapeComponentNotificationsBus::Event(
                GetEntityId(),
                &ShapeComponentNotificationsBus::Events::OnShapeChanged,
                ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
    }

    void CompoundShapeComponent::OnEntityDeactivated(const AZ::EntityId& id)
    {
        m_currentlyActiveChildren--;
        ShapeComponentNotificationsBus::MultiHandler::BusDisconnect(id);
        ShapeComponentNotificationsBus::Event(
            GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    void CompoundShapeComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            ShapeComponentNotificationsBus::Event(
                GetEntityId(),
                &ShapeComponentNotificationsBus::Events::OnShapeChanged,
                ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
        }
        else if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::TransformChanged)
        {
            // If there are multiple shapes in a compound shape, then moving one of them changes the overall compound shape, otherwise the
            // transform change is bubbled up directly
            ShapeComponentNotificationsBus::Event(
                GetEntityId(),
                &ShapeComponentNotificationsBus::Events::OnShapeChanged,
                (m_currentlyActiveChildren > 1) ? ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged
                                                : ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
        }
    }

    //////////////////////////////////////////////////////////////////////////
} // namespace LmbrCentral
