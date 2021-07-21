/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Render/Intersector.h>
#include <AzFramework/Scene/Scene.h>
#include <AzFramework/Visibility/BoundsBus.h>

#include <algorithm>

namespace AzFramework
{
    namespace RenderGeometry
    {
        Intersector::Intersector(AzFramework::EntityContextId contextId)
            : m_contextId(contextId)
        {
            IntersectorBus::Handler::BusConnect(m_contextId);
            IntersectionNotificationBus::Handler::BusConnect(m_contextId);

            AZStd::shared_ptr<Scene> scene = EntityContext::FindContainingScene(m_contextId);
            if (scene)
            {
                scene->SetSubsystem(this);
            }
        }

        Intersector::~Intersector()
        {
            IntersectorBus::Handler::BusDisconnect();
            IntersectionNotificationBus::Handler::BusDisconnect();

            AZStd::shared_ptr<Scene> scene = EntityContext::FindContainingScene(m_contextId);
            if (scene)
            {
                [[maybe_unused]] bool result = scene->UnsetSubsystem(this);
                AZ_Assert(result, "Failed to unregister Intersector with scene");
            }
        }

        RayResult Intersector::RayIntersect(const RayRequest& ray)
        {
            UpdateDirtyEntities();
            return CalculateRayAgainstEntityDataList(ray, m_registeredEntities);
        }

        void Intersector::OnEntityConnected(AZ::EntityId entityId)
        {
            m_registeredEntities.AddRef(entityId);
            m_dirtyEntities.insert(entityId);
        }

        void Intersector::OnEntityDisconnected(AZ::EntityId entityId)
        {
            int newRef = m_registeredEntities.RemoveRef(entityId);
            if (newRef == 0)
            {
                // no need to update this entity if it was queued
                m_dirtyEntities.erase(entityId);
            }
        }

        void Intersector::OnGeometryChanged(AZ::EntityId entityId)
        {
            m_dirtyEntities.insert(entityId);
        }

        RayResult Intersector::CalculateRayAgainstEntityDataList(const RayRequest& ray, EntityDataList& entityDataList)
        {
            const bool hasEntityFilter = ray.m_entityFilter.HasFilter();
            const AZ::Vector3 rayReciprocal = (ray.m_endWorldPosition - ray.m_startWorldPosition).GetReciprocal();

            for (const auto& entityData : entityDataList)
            {
                if (!hasEntityFilter || ray.m_entityFilter(entityData.m_id))
                {
                    float tStart, tEnd;
                    if (AZ::Intersect::IntersectRayAABB2(ray.m_startWorldPosition, rayReciprocal,
                        entityData.m_aabb, tStart, tEnd) == AZ::Intersect::ISECT_RAY_AABB_ISECT)
                    {
                        m_candidatesSortedByDist.push_back({ &entityData, tStart });
                    }
                }
            }

            AZStd::sort(
                m_candidatesSortedByDist.begin(), m_candidatesSortedByDist.end(),
                [](const auto& lhs, const auto& rhs)
                {
                    return lhs.second < rhs.second;
                });

            RayResult closestResult;
            for (int i = 0; i < m_candidatesSortedByDist.size(); ++i)
            {
                const auto& entityDataDist = m_candidatesSortedByDist[i];
                AZ::EBusReduceResult<RayResult, RayResultClosestAggregator> result;
                IntersectionRequestBus::EventResult(result, { entityDataDist.first->m_id, m_contextId }, &IntersectionRequests::RenderGeometryIntersect, ray);
                if (result.value && result.value.m_distance < closestResult.m_distance)
                {
                    closestResult = result.value;
                    // Only if there are no intersections with farther AABB candidates we can make sure this is actually the closest hit
                    bool hasIntersections = false;
                    for (int j = i + 1; j < m_candidatesSortedByDist.size() && !hasIntersections; ++j)
                    {
                        const auto& nextEntityDataDist = m_candidatesSortedByDist[j];
                        hasIntersections = entityDataDist.first->m_aabb.Overlaps(nextEntityDataDist.first->m_aabb);
                    }
                    if (!hasIntersections)
                    {
                        break;
                    }
                }
            }

            m_candidatesSortedByDist.clear();
            return closestResult;
        }

        void Intersector::UpdateDirtyEntities()
        {
            for (AZ::EntityId entityId : m_dirtyEntities)
            {
                AZ_Assert(
                    AzFramework::BoundsRequestBus::HasHandlers(entityId),
                    "Implementers of IntersectionRequestBus must also implement BoundsRequestBus to ensure valid "
                    "bounds are returned");

                AZ::Entity* entity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(entityId);
                m_registeredEntities.Update({ entityId, CalculateEntityWorldBoundsUnion(entity) });
            }

            m_dirtyEntities.clear();
        }

        //////////////////////////////////////////////////////////////////////////

        int Intersector::EntityDataList::AddRef(AZ::EntityId entityId)
        {
            auto itEntityIndex = m_entityIdIndexLookup.find(entityId);
            if (itEntityIndex != m_entityIdIndexLookup.end())
            {
                int newRef = ++m_entitiesData[itEntityIndex->second].m_ref;
                return newRef;
            }
            else
            {
                m_entitiesData.emplace_back(entityId, AZ::Aabb::CreateNull());
                m_entityIdIndexLookup[entityId] = static_cast<int>(m_entitiesData.size() - 1);
                return 1;
            }
        }

        int Intersector::EntityDataList::RemoveRef(AZ::EntityId entityId)
        {
            auto itEntityIndex = m_entityIdIndexLookup.find(entityId);
            if (itEntityIndex != m_entityIdIndexLookup.end())
            {
                int index = itEntityIndex->second;
                EntityData* removeLocation = &m_entitiesData[index];
                int newRef = --removeLocation->m_ref;
                if (newRef == 0)
                {
                    // Remove by moving the last element into it and reducing the vector
                    *removeLocation = std::move(m_entitiesData.back());

                    // Update the lookup of the moved element
                    m_entityIdIndexLookup[removeLocation->m_id] = index;

                    m_entitiesData.pop_back();
                    m_entityIdIndexLookup.erase(entityId);
                }
                return newRef;
            }
            else
            {
                AZ_Assert(false, "Entity %s to remove not found", entityId.ToString().c_str());
                return 0;
            }
        }

        void Intersector::EntityDataList::Update(const EntityData& newData)
        {
            auto itIndex = m_entityIdIndexLookup.find(newData.m_id);
            if (itIndex != m_entityIdIndexLookup.end())
            {
                int index = itIndex->second;
                m_entitiesData[index].m_aabb = newData.m_aabb;
            }
            else
            {
                AZ_Assert(false, "Tried to update unknown entity id %s", newData.m_id.ToString().c_str());
            }
        }

        Intersector::EntityData* Intersector::EntityDataList::FindEntityData(AZ::EntityId entityId)
        {
            auto itIndex = m_entityIdIndexLookup.find(entityId);
            if (itIndex != m_entityIdIndexLookup.end())
            {
                return &m_entitiesData[itIndex->second];
            }
            return nullptr;
        }
    }
}
