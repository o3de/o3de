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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/Rules/LodRule.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            const size_t LodRule::m_maxLods;


            SceneNodeSelectionList& LodRule::GetNodeSelectionList(size_t index)
            {
                if (index < m_maxLods)
                {
                    return m_nodeSelectionLists[index];
                }
                return *m_nodeSelectionLists.end();
            }


            DataTypes::ISceneNodeSelectionList& LodRule::GetSceneNodeSelectionList(size_t index)
            {
                if (index < m_maxLods)
                {
                    return m_nodeSelectionLists[index];
                }
                return *m_nodeSelectionLists.end();
            }

            const DataTypes::ISceneNodeSelectionList& LodRule::GetSceneNodeSelectionList(size_t index) const
            {
                if (index < m_maxLods)
                {
                    return m_nodeSelectionLists[index];
                }
                return *m_nodeSelectionLists.end();
            }


            size_t LodRule::GetLodCount() const
            {
                return m_nodeSelectionLists.size();
            }

            void LodRule::AddLod()
            {
                if (m_nodeSelectionLists.size() < m_nodeSelectionLists.capacity())
                {
                    m_nodeSelectionLists.push_back(SceneNodeSelectionList());
                }
            }

            void LodRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<LodRule, DataTypes::ILodRule>()->Version(1)
                    ->Field("nodeSelectionList", &LodRule::m_nodeSelectionLists);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<LodRule>("Level of Detail", "Set up the level of detail for the meshes in this group.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ_CRC("AutoExpand", 0x306ff5c0), true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_nodeSelectionLists, "Lod Meshes", "Select the meshes to assign to each level of detail.")
                            ->ElementAttribute(AZ_CRC("FilterName", 0xf49ce62e), "Lod meshes")
                            ->ElementAttribute(AZ_CRC("FilterType", 0x2661cf01), DataTypes::IMeshData::TYPEINFO_Uuid());
                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
