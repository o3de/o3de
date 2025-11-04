/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPIExt/Rules/LodRule.h>
#include <SceneAPIExt/Data/LodNodeSelectionList.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(LodRule, AZ::SystemAllocator)

            SceneData::SceneNodeSelectionList& LodRule::GetSceneNodeSelectionList(size_t index)
            {
                if (index < g_maxLods)
                {
                    return m_nodeSelectionLists[index];
                }
                return *m_nodeSelectionLists.end();
            }

            const SceneDataTypes::ISceneNodeSelectionList& LodRule::GetSceneNodeSelectionList(size_t index) const
            {
                if (index < g_maxLods)
                {
                    return m_nodeSelectionLists[index];
                }
                return *m_nodeSelectionLists.end();
            }

            size_t LodRule::GetLodRuleCount() const
            {
                return m_nodeSelectionLists.size();
            }

            void LodRule::AddLod()
            {
                if (m_nodeSelectionLists.size() < m_nodeSelectionLists.capacity())
                {
                    m_nodeSelectionLists.push_back(Data::LodNodeSelectionList());
                }
            }

            bool LodRule::ContainsNodeByPath(const AZStd::string& nodePath) const
            {
                const size_t lodCount = GetLodRuleCount();
                for (size_t i = 0; i < lodCount; ++i)
                {
                    const auto& list = m_nodeSelectionLists[i];
                    if (list.IsSelectedNode(nodePath))
                    {
                        return true;
                    }
                }
                return false;
            }

            bool LodRule::ContainsNodeByRuleIndex(const AZStd::string& nodeeName, AZ::u32 lodRuleIndex) const
            {
                if (lodRuleIndex >= GetLodRuleCount())
                {
                    return false;
                }
                const Data::LodNodeSelectionList list = m_nodeSelectionLists[lodRuleIndex];
                return list.ContainsNode(nodeeName);
            }

            void LodRule::Reflect(AZ::ReflectContext* context)
            {
                Data::LodNodeSelectionList::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<LodRule, IRule>()->Version(1)
                    ->Field("nodeSelectionList", &LodRule::m_nodeSelectionLists);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<LodRule>("Skeleton LOD", "Set up the level of detail for skeletons in this group.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("AutoExpand", true)
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &LodRule::m_nodeSelectionLists, "Skeleton",
                            "Select the joints to assign to each level of detail.")
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "Additional LOD")
                            ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ_CRC_CE("LODTreeSelection"))
                            ->ElementAttribute(AZ_CRC_CE("FilterType"), azrtti_typeid<SceneDataTypes::IBoneData>());
                }
            }
        }
    }
}
