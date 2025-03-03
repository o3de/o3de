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

            bool LodRule::GetAutoGenerateLods() const
            {
                return m_autoGenerateLods;
            }

            bool LodRule::GetSimplifyPreserveTopology() const
            {
                return m_simplifyPreserveTopology;
            }

            bool LodRule::GetSimplifyLimitError() const
            {
                return m_simplifyLimitError;
            }

            bool LodRule::GetSimplifyLockBorder() const
            {
                return m_simplifyLockBorder;
            }

            bool LodRule::GetSimplifySparse() const
            {
                return m_simplifySparse;
            }

            bool LodRule::GetSimplifyPrune() const
            {
                return m_simplifyPrune;
            }

            float LodRule::GetSimplifyTargetError() const
            {
                return m_simplifyTargetError;
            }

            float LodRule::GetSimplifyIndexThreshold() const
            {
                return m_simplifyIndexThreshold;
            }

            AZ::Crc32 LodRule::GetAutoLodsVisibility() const
            {
                return m_autoGenerateLods ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
            }

            AZ::Crc32 LodRule::GetErrorLimitVisibility() const
            {
                return (m_autoGenerateLods && m_simplifyLimitError) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
            }

            AZ::Crc32 LodRule::GetTopologySettingsVisibility() const
            {
                return (m_autoGenerateLods && m_simplifyPreserveTopology) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
            }

            void LodRule::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<LodRule, DataTypes::ILodRule>()->Version(2)
                    ->Field("autoGenerateLods", &LodRule::m_autoGenerateLods)
                    ->Field("PreserveTopology", &LodRule::m_simplifyPreserveTopology)
                    ->Field("LimitError", &LodRule::m_simplifyLimitError)
                    ->Field("LockBorder", &LodRule::m_simplifyLockBorder)
                    ->Field("Sparse", &LodRule::m_simplifySparse)
                    ->Field("Prune", &LodRule::m_simplifyPrune)
                    ->Field("TargetError", &LodRule::m_simplifyTargetError)
                    ->Field("Threshold", &LodRule::m_simplifyIndexThreshold)
                    ->Field("nodeSelectionList", &LodRule::m_nodeSelectionLists);

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
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_autoGenerateLods, "Auto Generate Lods", "Automatically generate the missing levels of details")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Auto Lod Settings")
                            ->Attribute(AZ_CRC_CE("AutoExpand"), true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::GetAutoLodsVisibility)
                        ->DataElement(Edit::UIHandlers::Slider, &LodRule::m_simplifyIndexThreshold, "Index Threshold", "The target index reduce threshold for the new lod.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_simplifyLimitError, "Limit Error", "Enable the error limit for the mesh.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->DataElement(Edit::UIHandlers::Slider, &LodRule::m_simplifyTargetError, "Target Error", "The target error limit for the mesh.")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::GetErrorLimitVisibility)
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_simplifyPreserveTopology, "Preserve Topology", "Preserve the topology of the mesh (slower).")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Topology Settings")
                            ->Attribute(AZ_CRC_CE("AutoExpand"), true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &LodRule::GetTopologySettingsVisibility)
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_simplifyLockBorder, "Lock Border", "Restrict the simplifier from collapsing edges that are on the border of the mesh.")
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_simplifySparse, "Sparse", "Improve simplification performance assuming input indices are a sparse subset of the mesh.")
                        ->DataElement(Edit::UIHandlers::Default, &LodRule::m_simplifyPrune, "Prune", "Allow the simplifier to remove isolated components regardless of the topological restrictions inside the component.");
                }
            }
        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
