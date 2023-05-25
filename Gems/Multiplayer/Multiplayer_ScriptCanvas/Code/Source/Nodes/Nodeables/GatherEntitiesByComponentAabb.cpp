/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Nodes/Nodeables/GatherEntitiesByComponentAabb.h>
#include <AzFramework/Visibility/IVisibilitySystem.h>
#include <AzCore/Math/ShapeIntersection.h>

namespace ScriptCanvasMultiplayer
{
    AZStd::vector<AZ::EntityId> GatherEntitiesByComponentAabb::In(AZ::Uuid componentGuid, AZ::Vector3 minimum, AZ::Vector3 maximum)
    {
        const AZ::Aabb bound = AZ::Aabb::CreateFromMinMax(minimum, maximum);

        AZStd::vector<AZ::EntityId> gatheredEntities;
        AZ::Interface<AzFramework::IVisibilitySystem>::Get()->GetDefaultVisibilityScene()->Enumerate(bound,
            [bound, componentGuid, &gatheredEntities](const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            gatheredEntities.reserve(gatheredEntities.size() + nodeData.m_entries.size());
            for (AzFramework::VisibilityEntry* visEntry : nodeData.m_entries)
            {
                if (AZ::ShapeIntersection::Overlaps(visEntry->m_boundingVolume, bound))
                {
                    if (visEntry->m_typeFlags & AzFramework::VisibilityEntry::TypeFlags::TYPE_Entity)
                    {
                        AZ::Entity* entity = static_cast<AZ::Entity*>(visEntry->m_userData);
                        if (entity->FindComponent(componentGuid) != nullptr)
                        {
                            gatheredEntities.push_back(entity->GetId());
                        }
                    }
                }
            }
        });

        return gatheredEntities;
    }
}
