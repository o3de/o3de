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

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Render/GeometryIntersectionStructures.h>
#include <AzFramework/Render/GeometryIntersectionBus.h>

namespace AzFramework
{
    namespace RenderGeometry
    {
        //! Interface for geometry Intersector Bus, an intersector is responsible of retrieving a performant
        //! render geometry intersection against components that implement the IntersectionRequests interface.
        //! There can be only one intersector per entity context
        //! Usage:
        //! Raycast against editor entity geometry:
        //!    IntersectorBus::EventResult(result, editorContextId, &IntersectorInterface::RayIntersect, ray);
        //!
        //! Raycast against all entities
        //!    RayResultAggregator rayResult;
        //!    IntersectorBus::BroadCastResult(rayResult, &IntersectorInterface::RayIntersect, ray);
        //
        class IntersectorInterface
            : public IntersectionNotifications
        {
        public:
            AZ_RTTI(IntersectorInterface, "{2DE6434E-7BD3-4364-A53E-EB31BC42FE4F}");

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = EntityContextId;

            virtual ~IntersectorInterface() = default;

            // Retrieves the closest render geometry intersection against the given ray
            virtual RayResult RayIntersect(const RayRequest& ray) = 0;
        };
        using IntersectorBus = AZ::EBus<IntersectorInterface>;
    }
}
