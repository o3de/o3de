/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Aabb;
}

namespace AzFramework
{
    //! Provides an interface to retrieve and update the union of all Aabbs on a single Entity.
    //! @note This will be the combination/union of all individual Component Aabbs.
    class IEntityBoundsUnion
    {
    public:
        AZ_RTTI(IEntityBoundsUnion, "{106968DD-43C0-478E-8045-523E0BF5D0F5}");

        //! Requests the cached union of component Aabbs to be recalculated as one may have changed.
        //! @note This is used to drive event driven updates to the visibility system.
        virtual void RefreshEntityLocalBoundsUnion(AZ::EntityId entityId) = 0;

        //! Returns the cached union of all component Aabbs in local entity space.
        virtual AZ::Aabb GetEntityLocalBoundsUnion(AZ::EntityId entityId) const = 0;

        //! Returns the cached union of all component Aabbs in world space.
        virtual AZ::Aabb GetEntityWorldBoundsUnion(AZ::EntityId entityId) const = 0;

        //! Writes the current changes made to all entities (transforms and bounds) to the visibility system.
        //! @note During normal operation this is called every frame in OnTick but can
        //! also be called explicitly (e.g. For testing purposes).
        virtual void ProcessEntityBoundsUnionRequests() = 0;

        //! Notifies the EntityBoundsUnion system that an entities transform has been modified.
        //! @param entity the entity whose transform has been modified.
        virtual void OnTransformUpdated(AZ::Entity* entity) = 0;

    protected:
        ~IEntityBoundsUnion() = default;
    };

    // EBus wrapper for ScriptCanvas
    class IEntityBoundsUnionTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using IEntityBoundsUnionRequestBus = AZ::EBus<IEntityBoundsUnion, IEntityBoundsUnionTraits>;
} // namespace AzFramework
