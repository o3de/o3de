/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
    class EntityBoundsUnionRequests : public AZ::EBusTraits
    {
    public:
        //! Requests the cached union of component Aabbs to be recalculated as one may have changed.
        //! @note This is used to drive event driven updates to the visibility system.
        virtual void RefreshEntityLocalBoundsUnion(AZ::EntityId entityId) = 0;

        //! Returns the cached union of all component Aabbs.
        virtual AZ::Aabb GetEntityLocalBoundsUnion(AZ::EntityId entityId) const = 0;

        //! Writes the current changes made to all entities (transforms and bounds) to the visibility system.
        //! @note During normal operation this is called every frame in OnTick but can
        //! also be called explicitly (e.g. For testing purposes).
        virtual void ProcessEntityBoundsUnionRequests() = 0;

    protected:
        ~EntityBoundsUnionRequests() = default;
    };

    using EntityBoundsUnionRequestBus = AZ::EBus<EntityBoundsUnionRequests>;
} // namespace AzFramework
