/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/functional.h>

//! Common structures for Render geometry queries
namespace AzFramework
{
    //! Aggregator for usage with EBus BroadcastResult or EventResult
    //! Contains the AABB union of all the handlers
    struct AabbUnionAggregator
    {
        AZ::Aabb operator()(AZ::Aabb lhs, const AZ::Aabb& rhs) const
        {
            lhs.AddAabb(rhs);
            return lhs;
        }
    };

    namespace RenderGeometry
    {
        struct EntityFilter
        {
            bool operator()(AZ::EntityId entityId) const
            {
                bool isIgnored = m_ignoreEntities.find(entityId) != m_ignoreEntities.end();
                if (!isIgnored)
                {
                    bool noExclusivesOrIsExclusive = (m_exclusiveEntities.empty()) || (m_exclusiveEntities.find(entityId) != m_ignoreEntities.end());
                    if (noExclusivesOrIsExclusive)
                    {
                        bool considered = !m_filterFunction || m_filterFunction(entityId);
                        return considered;
                    }
                }
                return false;
            }

            bool HasFilter() const
            {
                return !m_exclusiveEntities.empty() || !m_ignoreEntities.empty() || m_filterFunction;
            }

            AZStd::unordered_set<AZ::EntityId> m_exclusiveEntities; //! If *not empty*, will only raycast against these entities
            AZStd::unordered_set<AZ::EntityId> m_ignoreEntities;    //! Entities to ignore
            AZStd::function<bool(AZ::EntityId)> m_filterFunction;   //! Optional custom filter callback, return true for using argument's entityid for raycast
                                                                    //! return false for not considering this entity
        };

        //! Ray intersection request 
        struct RayRequest
        {
            AZ::Vector3 m_startWorldPosition; //!< Ray start position in world space
            AZ::Vector3 m_endWorldPosition; //!< Ray end position in world space
            bool m_onlyVisible = true; //!< Whether to only consider visible objects
            EntityFilter m_entityFilter; //!< Filter for entities
        };

        //! Result of intersection
        struct RayResult
        {
            AZ::Vector3 m_worldPosition; //!< Hit position in world space
            AZ::Vector3 m_worldNormal; //!< Normal of the surface hit in world space
            AZ::Vector2 m_uv; //!< UV texture coordinates of the intersected geometry
            AZ::EntityComponentIdPair m_entityAndComponent; //!< Entity and component hit, for non-entity systems this can be invalid
            float m_distance = FLT_MAX; //!< Distance from ray start to hit collision, FLT_MAX if there is no hit

            explicit operator bool() const
            {
                return m_distance != FLT_MAX;
            }
        };

        //! Aggregator for usage with EBus BroadcastResult or EventResult
        //! Contains the closet successful hit of all the handlers
        struct RayResultClosestAggregator
        {
            RayResult operator()(RayResult lhs, const RayResult& rhs) const
            {
                if (rhs && rhs.m_distance < lhs.m_distance)
                {
                    lhs = rhs;
                }
                return lhs;
            }
        };
    }
}
