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

            void LodRule::AutoLodGenerationSettings::Reflect(ReflectContext* context)
            {
                if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<LodRule::AutoLodGenerationSettings>()
                        ->Version(1)
                        ->Field("PreserveTopology", &LodRule::AutoLodGenerationSettings::m_preserveTopology)
                        ->Field("LimitError", &LodRule::AutoLodGenerationSettings::m_limitError)
                        ->Field("LockBorder", &LodRule::AutoLodGenerationSettings::m_lockBorder)
                        ->Field("Sparse", &LodRule::AutoLodGenerationSettings::m_sparse)
                        ->Field("Prune", &LodRule::AutoLodGenerationSettings::m_prune)
                        ->Field("TargetError", &LodRule::AutoLodGenerationSettings::m_targetError)
                        ->Field("IndexThreshold", &LodRule::AutoLodGenerationSettings::m_indexThreshold);
                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<LodRule::AutoLodGenerationSettings>("Auto Lod Generation Settings", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LodRule::AutoLodGenerationSettings::m_preserveTopology, "Preserve Topology", "Preserve the topology of the mesh (slower).")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LodRule::AutoLodGenerationSettings::m_lockBorder, "Lock Border", "Restrict from collapsing edges that are on the border of the mesh.")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::AutoLodGenerationSettings::m_preserveTopology)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LodRule::AutoLodGenerationSettings::m_sparse, "Sparse", "Improve simplification performance assuming input indices are a sparse subset of the mesh.")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::AutoLodGenerationSettings::m_preserveTopology)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LodRule::AutoLodGenerationSettings::m_prune, "Prune", "Allow the simplifier to remove isolated components regardless of the topological restrictions inside the component.")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::AutoLodGenerationSettings::m_preserveTopology)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &LodRule::AutoLodGenerationSettings::m_limitError, "Limit Error", "Enable the error limit for the mesh.")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &LodRule::AutoLodGenerationSettings::m_targetError, "Target Error", "The target error limit for the mesh.")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                                ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                                ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::AutoLodGenerationSettings::m_limitError)
                            ->DataElement(Edit::UIHandlers::Slider, &LodRule::AutoLodGenerationSettings::m_indexThreshold, "Index Threshold", "The index buffer reduction threshold for the new lod.")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                                ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                                ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                                ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                            ;
                    }
                }
            }

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

            bool LodRule::IsAutoLodGenerationEnabled() const
            {
                return m_isAutoLodGenerationEnabled;
            }

            const LodRule::AutoLodGenerationSettings& LodRule::GetAutoLodGenerationSettings() const
            {
                return m_autoLodGenerationSettings;
            }

            void LodRule::Reflect(ReflectContext* context)
            {
                AutoLodGenerationSettings::Reflect(context);
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<LodRule, DataTypes::ILodRule>()->Version(2)
                    ->Field("nodeSelectionList", &LodRule::m_nodeSelectionLists)
                    ->Field("isAutoLodGenerationEnabled", &LodRule::m_isAutoLodGenerationEnabled)
                    ->Field("autoLodGenerationSettings", &LodRule::m_autoLodGenerationSettings);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<LodRule>("Level of Detail", "Set up the level of detail for the meshes in this group.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ_CRC_CE("AutoExpand"), true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_nodeSelectionLists, "Lod Meshes", "Select the meshes to assign to each level of detail.")
                            ->ElementAttribute(AZ_CRC_CE("FilterName"), "Lod meshes")
                            ->ElementAttribute(AZ_CRC_CE("FilterType"), DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_isAutoLodGenerationEnabled, "Enable Auto Lod Generation", "Automatically generate the missing levels of details")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_autoLodGenerationSettings, "Auto Lod Generation Settings", "Auto Lod Generation Settings")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::m_isAutoLodGenerationEnabled)
                        ;
                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
