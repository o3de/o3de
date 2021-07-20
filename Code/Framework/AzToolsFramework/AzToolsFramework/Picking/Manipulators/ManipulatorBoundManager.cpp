/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ManipulatorBoundManager.h"

namespace AzToolsFramework
{
    namespace Picking
    {
        RegisteredBoundId ManipulatorBoundManager::UpdateOrRegisterBound(const BoundRequestShapeBase& shapeData, RegisteredBoundId boundId)
        {
            if (boundId == InvalidBoundId)
            {
                // make a new bound
                boundId = m_nextBoundId++;
            }

            if (auto result = m_boundIdToShapeMap.find(boundId); result == m_boundIdToShapeMap.end())
            {
                if (AZStd::shared_ptr<BoundShapeInterface> createdShape = CreateShape(shapeData, boundId))
                {
                    m_boundIdToShapeMap[boundId] = createdShape;
                    createdShape->SetValidity(true);
                }
                else
                {
                    boundId = InvalidBoundId;
                }
            }
            else
            {
                result->second->SetShapeData(shapeData);
                result->second->SetValidity(true);
            }

            return boundId;
        }

        void ManipulatorBoundManager::UnregisterBound(const RegisteredBoundId boundId)
        {
            if (const auto findIter = m_boundIdToShapeMap.find(boundId); findIter != m_boundIdToShapeMap.end())
            {
                DeleteShape(findIter->second.get());
                m_boundIdToShapeMap.erase(findIter);
            }
        }

        void ManipulatorBoundManager::SetBoundValidity(const RegisteredBoundId boundId, const bool valid)
        {
            if (auto found = m_boundIdToShapeMap.find(boundId); found != m_boundIdToShapeMap.end())
            {
                found->second->SetValidity(valid);
            }
        }

        AZStd::shared_ptr<BoundShapeInterface> ManipulatorBoundManager::CreateShape(
            const BoundRequestShapeBase& shapeData, const RegisteredBoundId boundId)
        {
            AZ_Assert(boundId != InvalidBoundId, "Invalid Bound Id!");

            AZStd::shared_ptr<BoundShapeInterface> shape = shapeData.MakeShapeInterface(boundId);
            m_bounds.push_back(shape);
            return shape;
        }

        void ManipulatorBoundManager::DeleteShape(const BoundShapeInterface* boundShape)
        {
            const auto boundShapeCompare = [boundShape](const auto& storedBoundShape)
            {
                return boundShape == storedBoundShape.get();
            };

            m_bounds.erase(AZStd::remove_if(m_bounds.begin(), m_bounds.end(), boundShapeCompare), m_bounds.end());
        }

        void ManipulatorBoundManager::RaySelect(RaySelectInfo& rayInfo)
        {
            using BoundIdHitDistance = AZStd::pair<RegisteredBoundId, float>;

            // create a sorted list of manipulators - sorted based on proximity to ray
            AZStd::vector<BoundIdHitDistance> rayHits;
            rayHits.reserve(m_bounds.size());
            for (const AZStd::shared_ptr<BoundShapeInterface>& bound : m_bounds)
            {
                if (bound->IsValid())
                {
                    float t = 0.0f;
                    if (bound->IntersectRay(rayInfo.m_origin, rayInfo.m_direction, t))
                    {
                        const auto hitItr = AZStd::lower_bound(
                            rayHits.begin(), rayHits.end(), BoundIdHitDistance(0, t),
                            [](const BoundIdHitDistance& lhs, const BoundIdHitDistance& rhs)
                            {
                                return lhs.second < rhs.second;
                            });

                        rayHits.insert(hitItr, AZStd::make_pair(bound->GetBoundId(), t));
                    }
                }
            }

            rayInfo.m_boundIdsHit.reserve(rayHits.size());
            AZStd::copy(rayHits.begin(), rayHits.end(), AZStd::back_inserter(rayInfo.m_boundIdsHit));
        }
    } // namespace Picking
} // namespace AzToolsFramework
