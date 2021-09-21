/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>

class CEntityObject;

namespace AzFramework
{
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    /// Bus for customizing Entity selection logic from within the EditorComponents.
    /// Used to provide with custom implementation for Ray intersection tests, specifying AABB, etc.
    class EditorComponentSelectionRequests
        : public AZ::ComponentBus
    {
    public:
        /// @brief Returns an AABB that encompasses the object.
        /// @return AABB that encompasses the object.
        /// @note ViewportInfo may be necessary if the all or part of the object
        /// stays at a constant size regardless of camera position.
        virtual AZ::Aabb GetEditorSelectionBoundsViewport(
            const AzFramework::ViewportInfo& /*viewportInfo*/)
        {
            AZ_Assert(!SupportsEditorRayIntersect(),
                "Component claims to support ray intersection but GetEditorSelectionBoundsViewport "
                "has not been implemented in the derived class");

            return AZ::Aabb::CreateNull();
        }

        /// @brief Returns true if editor selection ray intersects with the handler.
        /// @return True if the editor selection ray intersects the handler.
        /// @note ViewportInfo may be necessary if the all or part of the object
        /// stays at a constant size regardless of camera position.
        virtual bool EditorSelectionIntersectRayViewport(
            const AzFramework::ViewportInfo& /*viewportInfo*/,
            const AZ::Vector3& /*src*/, const AZ::Vector3& /*dir*/, float& /*distance*/)
        {
            AZ_Assert(!SupportsEditorRayIntersect(),
                "Component claims to support ray intersection but EditorSelectionIntersectRayViewport "
                "has not been implemented in the derived class");

            return false;
        }

        /// @brief Returns true if the component overrides EditorSelectionIntersectRay method,
        /// otherwise selection will be based only on AABB test.
        /// @return True if EditorSelectionIntersectRay method is implemented.
        virtual bool SupportsEditorRayIntersect() { return false; }

    protected:
        ~EditorComponentSelectionRequests() = default;
    };

    /// Type to inherit to implement EditorComponentSelectionRequests.
    using EditorComponentSelectionRequestsBus = AZ::EBus<EditorComponentSelectionRequests>;

    /// Bus that provides notifications about selection events of the parent Entity.
    class EditorComponentSelectionNotifications
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        /// @brief Notifies listeners about in-editor selection events (mouse hover, selected, etc.)
        virtual void OnAccentTypeChanged(EntityAccentType /*accent*/) {}

    protected:
        ~EditorComponentSelectionNotifications() = default;
    };

    /// Type to inherit to implement EditorComponentSelectionNotifications.
    using EditorComponentSelectionNotificationsBus = AZ::EBus<EditorComponentSelectionNotifications>;

    /// Returns the union of all editor selection bounds on a given Entity.
    /// @note The returned Aabb is in world space.
    inline AZ::Aabb CalculateEditorEntitySelectionBounds(
        const AZ::EntityId entityId, const AzFramework::ViewportInfo& viewportInfo)
    {
        AZ::EBusReduceResult<AZ::Aabb, AzFramework::AabbUnionAggregator> aabbResult(AZ::Aabb::CreateNull());
        EditorComponentSelectionRequestsBus::EventResult(
            aabbResult, entityId, &EditorComponentSelectionRequestsBus::Events::GetEditorSelectionBoundsViewport, viewportInfo);

        return aabbResult.value;
    }
} // namespace AzToolsFramework
