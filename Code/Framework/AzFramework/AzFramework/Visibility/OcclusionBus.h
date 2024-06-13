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
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Entity/EntityContextBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    //! OcclusionRequests is an interface for creating occlusion views and queries. The bus must be connected to an entity context ID for an
    //! entity context containing objects that contribute to or interact with the occlusion scene.
    class OcclusionRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AzFramework::EntityContextId;
        using MutexType = AZStd::recursive_mutex;

        static void Reflect(AZ::ReflectContext* context);

        //! Clear previously collected debug rendering lines and statistics
        virtual void ClearOcclusionViewDebugInfo() = 0;

        //! Return true if the occlusion view exists.
        virtual bool IsOcclusionView(const AZ::Name& viewName) const = 0;

        //! Create an occlusion view using the specified viewName. After the view is created, the name can be used to look up the view and
        //! make updates or queries against it. More than one occlusion view can be created at a time and used across threads.
        virtual bool CreateOcclusionView(const AZ::Name& viewName) = 0;

        //! Destroy a previously created occlusion view.
        virtual bool DestroyOcclusionView(const AZ::Name& viewName) = 0;

        //! Updating the occlusion view to capture occlusion data from the specified camera perspective.
        virtual bool UpdateOcclusionView(
            const AZ::Name& viewName, const AZ::Vector3& cameraWorldPos, const AZ::Matrix4x4& cameraWorldToClip) = 0;

        //! Returns true if the entity is not occluded in the occlusion view. Otherwise, returns false.
        virtual bool GetOcclusionViewEntityVisibility(const AZ::Name& viewName, const AZ::EntityId& entityId) const = 0;

        //! Returns true if the AABB is not occluded in the occlusion view. Otherwise, returns false.
        virtual bool GetOcclusionViewAabbVisibility(const AZ::Name& viewName, const AZ::Aabb& bounds) const = 0;

        //! @brief Test if target shapes are visible from the perspective of a source shape, within a volume extruded between them.
        //! @param sourceAabb Bounding volume for the source shape defining the frame of reference for visibility tests.
        //! @param targetAabbs Vector of bounding volumes for target shapes that will be tested for visibility.
        //! @return A vector of flags containing the visibility state for corresponding target shapes. The size of the returned container
        //! should match the size of the input target container unless an ever occurred.
        virtual AZStd::vector<bool> GetOcclusionViewAabbToAabbVisibility(
            const AZ::Name& viewName, const AZ::Aabb& sourceAabb, const AZStd::vector<AZ::Aabb>& targetAabbs) const = 0;

        //! @brief Test if target shapes are visible from the perspective of a source shape, within a volume extruded between them.
        //! @param sourceSphere Bounding volume for the source shape defining the frame of reference for visibility tests.
        //! @param targetSpheres Vector of bounding volumes for target shapes that will be tested for visibility.
        //! @return A vector of flags containing the visibility state for corresponding target shapes. The size of the returned container
        //! should match the size of the input target container unless an ever occurred.
        virtual AZStd::vector<bool> GetOcclusionViewSphereToSphereVisibility(
            const AZ::Name& viewName, const AZ::Sphere& sourceSphere, const AZStd::vector<AZ::Sphere>& targetSpheres) const = 0;

        //! @brief Test if target entities are visible from the perspective of a source entity, within a volume extruded between them.
        //! @param sourceEntityId Id for the source entity defining the frame of reference for visibility tests.
        //! @param targetEntityIds Vector of Ids for target entities that will be tested for visibility.
        //! @return A vector of flags containing the visibility state for corresponding target entities. The size of the returned container
        //! should match the size of the input target container unless an ever occurred.
        virtual AZStd::vector<bool> GetOcclusionViewEntityToEntityVisibility(
            const AZ::Name& viewName, const AZ::EntityId& sourceEntityId, const AZStd::vector<AZ::EntityId>& targetEntityIds) const = 0;

    protected:
        ~OcclusionRequests() = default;
    };

    using OcclusionRequestBus = AZ::EBus<OcclusionRequests>;
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::OcclusionRequests);
