/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(
        OcclusionState,
        uint32_t,
        Unknown, //! The object state cannot be determined.
        Hidden, //! The object is completely occluded or hidden.
        Visible //! The object is completely or partially visible.
    );

    //! @brief OcclusionRequests provides an interface for integrating with occlusion culling systems. Use the interface to create
    //! occlusion views that configure the camera state and context for making occlusion queries. Creating multiple occlusion views
    //! also allows queries to be made across multiple threads.
    //!
    //! The bus must be connected to an entity context ID for an entity context containing objects that contribute to or interacting
    //! with the occlusion scene.
    class OcclusionRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AzFramework::EntityContextId;
        using MutexType = AZStd::recursive_mutex;

        static void Reflect(AZ::ReflectContext* context);

        //! @brief Clear previously collected debug rendering lines and statistics
        virtual void ClearOcclusionViewDebugInfo() = 0;

        //! @brief Check if an occlusion view exists with the specified name
        //! @param viewName Unique name of the view to check
        //! @return true if a view with the given name exists, otherwise false.
        virtual bool IsOcclusionViewValid(const AZ::Name& viewName) const = 0;

        //! @brief Create an occlusion view using the specified name. After the view is created, the name can be used to look up the
        //! view and make updates or queries against it. More than one occlusion view can be created at a time and used across threads.
        //! @param viewName Unique name of the view to create
        //! @return true if a view with the given name was created and registered with the system, otherwise false.
        virtual bool CreateOcclusionView(const AZ::Name& viewName) = 0;

        //! @brief Destroy a previously created occlusion view.
        //! @param viewName Unique name of the view to destroy
        //! @return true if a view with the given name was destroyed and removed from the system, otherwise false.
        virtual bool DestroyOcclusionView(const AZ::Name& viewName) = 0;

        //! @brief Update the occlusion view from the given camera perspective.
        //! @param viewName Unique name of the view to update
        //! @param cameraWorldPos World position of the camera view
        //! @param cameraWorldToClip Matrix transforming vertices from world space to clip space for occlusion tests
        //! @return true if a view with the given name was updated, otherwise false.
        virtual bool UpdateOcclusionView(
            const AZ::Name& viewName, const AZ::Vector3& cameraWorldPos, const AZ::Matrix4x4& cameraWorldToClip) = 0;

        //! @brief Determine if an entity is visible within the specified occlusion view.
        //! @param viewName Unique name of the view to query for entity visibility
        //! @param entityId ID of the entity to test for visibility
        //! @return OcclusionState value classifying the entity as unknown, hidden, or visible.
        virtual OcclusionState GetOcclusionViewEntityVisibility(const AZ::Name& viewName, const AZ::EntityId& entityId) const = 0;

        //! @brief Determine if an AABB is visible within the specified occlusion view.
        //! @param viewName Unique name of the view to query for AABB visibility
        //! @param bounds World AABB to test for visibility
        //! @return OcclusionState value classifying the AABB as unknown, hidden, or visible.
        virtual OcclusionState GetOcclusionViewAabbVisibility(const AZ::Name& viewName, const AZ::Aabb& bounds) const = 0;

        //! @brief Test if target bounding volumes are visible from the perspective of a source bounding volume, within a volume extruded
        //! between them.
        //! @param viewName Unique name of the view to query
        //! @param sourceAabb Source bounding volume defining the frame of reference for visibility tests.
        //! @param targetAabbs Vector of target bounding volumes that will be tested for visibility.
        //! @return A vector of OcclusionState values classifying the input target bounds as unknown, hidden, or visible. The size of the
        //! returned container should match the size of the input target container unless an error occurred, in which case an empty vector
        //! is returned.
        virtual AZStd::vector<OcclusionState> GetOcclusionViewAabbToAabbVisibility(
            const AZ::Name& viewName, const AZ::Aabb& sourceAabb, const AZStd::vector<AZ::Aabb>& targetAabbs) const = 0;

        //! @brief Test if target bounding volumes are visible from the perspective of a source bounding volume, within a volume extruded
        //! between them.
        //! @param viewName Unique name of the view to query
        //! @param sourceSphere Source bounding volume for the frame of reference for visibility tests.
        //! @param targetSpheres Vector of target bounding volumes that will be tested for visibility.
        //! @return A vector of OcclusionState values classifying the input target bounds as unknown, hidden, or visible. The size of the
        //! returned container should match the size of the input target container unless an error occurred, in which case an empty vector
        //! is returned.
        virtual AZStd::vector<OcclusionState> GetOcclusionViewSphereToSphereVisibility(
            const AZ::Name& viewName, const AZ::Sphere& sourceSphere, const AZStd::vector<AZ::Sphere>& targetSpheres) const = 0;

        //! @brief Test if target entity AABBs are visible from the perspective of a source entity AABB, within a volume extruded between
        //! them.
        //! @param viewName Unique name of the view to query
        //! @param sourceEntityId Id for the source entity defining the frame of reference for visibility tests.
        //! @param targetEntityIds Vector of target entity Ids that will be tested for visibility.
        //! @return A vector of OcclusionState values classifying the input target entities as unknown, hidden, or visible. The size of the
        //! returned container should match the size of the input target container unless an error occurred, in which case an empty vector
        //! is returned.
        virtual AZStd::vector<OcclusionState> GetOcclusionViewEntityToEntityVisibility(
            const AZ::Name& viewName, const AZ::EntityId& sourceEntityId, const AZStd::vector<AZ::EntityId>& targetEntityIds) const = 0;

    protected:
        ~OcclusionRequests() = default;
    };

    using OcclusionRequestBus = AZ::EBus<OcclusionRequests>;

    AZ_TYPE_INFO_SPECIALIZE(OcclusionState, "{1F6EE4E1-FD5C-4D80-AC72-E2C6C5E4A53F}");
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::OcclusionRequests);
