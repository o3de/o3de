/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/ActorInfo.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeGroupInfo.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorInfo, EMStudio::UIAllocator)

    ActorInfo::ActorInfo(const EMotionFX::ActorInstance* actorInstance)
    {
        EMotionFX::Actor* actor = actorInstance->GetActor();

        m_name = actor->GetNameString();

        // unit type
        m_unitType = MCore::Distance::UnitTypeToString(actor->GetFileUnitType());

        // nodes
        m_nodeCount = actor->GetNumNodes();

        // node groups
        const size_t numNodeGroups = actor->GetNumNodeGroups();
        m_nodeGroups.reserve(numNodeGroups);
        for (size_t i = 0; i < numNodeGroups; ++i)
        {
            m_nodeGroups.emplace_back(actor, actor->GetNodeGroup(i));
        }

        // global mesh information
        const size_t lodLevel = actorInstance->GetLODLevel();
        uint32 numPolygons;
        actor->CalcMeshTotals(lodLevel, &numPolygons, &m_totalVertices, &m_totalIndices);
    }

    void ActorInfo::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ActorInfo>()
            ->Version(1)
            ->Field("name", &ActorInfo::m_name)
            ->Field("unitType", &ActorInfo::m_unitType)
            ->Field("nodeCount", &ActorInfo::m_nodeCount)
            ->Field("nodeGroups", &ActorInfo::m_nodeGroups)
            ->Field("totalVertices", &ActorInfo::m_totalVertices)
            ->Field("totalIndices", &ActorInfo::m_totalIndices)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ActorInfo>("Actor info", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ActorInfo::m_name, "Name", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ActorInfo::m_unitType, "File unit type", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ActorInfo::m_nodeCount, "Nodes", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ActorInfo::m_nodeGroups, "Node groups", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ActorInfo::m_totalVertices, "Total vertices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ActorInfo::m_totalIndices, "Total indices", "")
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
        ;
    }
} // namespace EMStudio
