/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzToolsFramework/ToolsComponents/EditorSelectionAccentSystemComponent.h>

namespace AzFramework
{
    struct ViewportInfo;
}

namespace AzToolsFramework
{
    //! Bus for customizing Entity selection logic from within the EditorComponents.
    //! Used to provide with custom implementation for Ray intersection tests, specifying AABB, etc.
    class EditorComponentSelectionRequests : public AZ::ComponentBus
    {
    public:
        //! @brief Returns an AABB that encompasses the object.
        //! @return AABB that encompasses the object.
        //! @note ViewportInfo may be necessary if the all or part of the object
        //! stays at a constant size regardless of camera position.
        virtual AZ::Aabb GetEditorSelectionBoundsViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            AZ_Assert(
                !SupportsEditorRayIntersect(),
                "Component claims to support ray intersection but GetEditorSelectionBoundsViewport "
                "has not been implemented in the derived class");

            return AZ::Aabb::CreateNull();
        }

        //! @brief Returns true if editor selection ray intersects with the handler.
        //! @return True if the editor selection ray intersects the handler.
        //! @note ViewportInfo may be necessary if the all or part of the object
        //! stays at a constant size regardless of camera position.
        virtual bool EditorSelectionIntersectRayViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
            [[maybe_unused]] const AZ::Vector3& src,
            [[maybe_unused]] const AZ::Vector3& dir,
            [[maybe_unused]] float& distance)
        {
            AZ_Assert(
                !SupportsEditorRayIntersect(),
                "Component claims to support ray intersection but EditorSelectionIntersectRayViewport "
                "has not been implemented in the derived class");

            return false;
        }

        //! @brief Returns if the component overrides EditorSelectionIntersectRay(Viewport) interface,
        //! otherwise selection will be based only on an AABB test.
        virtual bool SupportsEditorRayIntersect()
        {
            return false;
        }

        //! @brief Returns if the component overrides EditorSelectionIntersectRay(Viewport) interface,
        //! otherwise selection will be based only on an AABB test.
        //! @note Overload of SupportsEditorRayIntersect which accepts a ViewportInfo containing the ViewportId, this can be used to
        //! lookup the intersection setting per viewport.
        virtual bool SupportsEditorRayIntersectViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return SupportsEditorRayIntersect();
        }

    protected:
        ~EditorComponentSelectionRequests() = default;
    };

    //! Type to inherit to implement EditorComponentSelectionRequests.
    using EditorComponentSelectionRequestsBus = AZ::EBus<EditorComponentSelectionRequests>;

    //! Bus that provides notifications about selection events of the parent Entity.
    class EditorComponentSelectionNotifications : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;

        //! @brief Notifies listeners about in-editor selection events (mouse hover, selected, etc.)
        virtual void OnAccentTypeChanged([[maybe_unused]] EntityAccentType accent)
        {
        }

    protected:
        ~EditorComponentSelectionNotifications() = default;
    };

    //! Type to inherit to implement EditorComponentSelectionNotifications.
    using EditorComponentSelectionNotificationsBus = AZ::EBus<EditorComponentSelectionNotifications>;

    //! Returns the union of all editor selection bounds on a given Entity.
    //! @note The returned Aabb is in world space.
    AZ::Aabb CalculateEditorEntitySelectionBounds(const AZ::EntityId entityId, const AzFramework::ViewportInfo& viewportInfo);
} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN(AzToolsFramework::EditorComponentSelectionRequests);
