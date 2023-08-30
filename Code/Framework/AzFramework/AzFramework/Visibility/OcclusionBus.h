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

        //! Create an occlusion view using the specified name. After the view is created, the name can be used to look up the view and make
        //! updates or queries against it. More than one occlusion view can be created at a time and used across threads.
        virtual bool CreateOcclusionView(const AZ::Name& name) = 0;

        //! Destroy a previously created occlusion view.
        virtual bool DestroyOcclusionView(const AZ::Name& name) = 0;

        //! Updating the occlusion view to capture occlusion data from the specified camera perspective.
        virtual bool UpdateOcclusionView(
            const AZ::Name& name, const AZ::Vector3& cameraWorldPos, const AZ::Matrix4x4& cameraWorldToClip) = 0;

        //! Returns true if the entity is not occluded in the occlusion view. Otherwise, returns false.
        virtual bool IsEntityVisibleInOcclusionView(const AZ::Name& name, const AZ::EntityId& entityId) const = 0;

        //! Returns true if the AABB is not occluded in the occlusion view. Otherwise, returns false.
        virtual bool IsAabbVisibleInOcclusionView(const AZ::Name& name, const AZ::Aabb& bounds) const = 0;

    protected:
        ~OcclusionRequests() = default;
    };

    using OcclusionRequestBus = AZ::EBus<OcclusionRequests>;
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::OcclusionRequests);
