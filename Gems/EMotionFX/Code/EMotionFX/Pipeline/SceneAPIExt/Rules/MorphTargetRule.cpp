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
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/FilterIterator.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#include <SceneAPIExt/Rules/MorphTargetRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            AZ_CLASS_ALLOCATOR_IMPL(MorphTargetRule, AZ::SystemAllocator)

            MorphTargetRule::MorphTargetRule()
                :m_readOnly(false)
            {
            }

            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& MorphTargetRule::GetSceneNodeSelectionList()
            {
                return m_morphTargets;
            }

            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& MorphTargetRule::GetSceneNodeSelectionList() const
            {
                return m_morphTargets;
            }

            void MorphTargetRule::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MorphTargetRule, AZ::SceneAPI::DataTypes::IBlendShapeRule>()->Version(1)
                    ->Field("morphTargets", &MorphTargetRule::m_morphTargets);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MorphTargetRule>("Morph Targets", "Select morph targets for actor.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->DataElement(AZ_CRC_CE("ManifestName"), &MorphTargetRule::m_morphTargets, "Select morph targets",
                            "Select 1 or more meshes to include in the actor as morph targets.")
                            ->Attribute("FilterName", "morph targets")
                            ->Attribute("FilterType", AZ::SceneAPI::DataTypes::IBlendShapeData::TYPEINFO_Uuid())
                            ->Attribute("ReadOnly", &MorphTargetRule::m_readOnly)
                            ->Attribute("NarrowSelection", true);
                }
            }

            size_t MorphTargetRule::SelectMorphTargets(const AZ::SceneAPI::Containers::Scene & scene, AZ::SceneAPI::DataTypes::ISceneNodeSelectionList & selection)
            {
                AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(scene.GetGraph(), selection);
                size_t morphTargetCount = 0;

                const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();

                auto keyValueView = AZ::SceneAPI::Containers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = AZ::SceneAPI::Containers::Views::MakeFilterView(keyValueView, AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IBlendShapeData>());
                for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                {
                    AZ::SceneAPI::Events::GraphMetaInfo::VirtualTypesSet types;
                    auto keyValueIterator = it.GetBaseIterator();
                    AZ::SceneAPI::Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                    AZ::SceneAPI::Events::GraphMetaInfoBus::Broadcast(
                        &AZ::SceneAPI::Events::GraphMetaInfoBus::Events::GetVirtualTypes, types, scene, index);
                    if (types.find(AZ::SceneAPI::Events::GraphMetaInfo::GetIgnoreVirtualType()) == types.end())
                    {
                        selection.AddSelectedNode(it->first.GetPath());
                        morphTargetCount++;
                    }
                }
                return morphTargetCount;
            }

            void MorphTargetRule::OnUserAdded()
            {
            }

            void MorphTargetRule::OnUserRemoved() const
            {
            }


            //MorphTargetRuleReadOnly

            AZ_CLASS_ALLOCATOR_IMPL(MorphTargetRuleReadOnly, AZ::SystemAllocator)
            
            MorphTargetRuleReadOnly::MorphTargetRuleReadOnly()
                :m_descriptionText("All morph targets motions imported")
            {
            }

            MorphTargetRuleReadOnly::MorphTargetRuleReadOnly(size_t morphAnimationCount)
                : m_morphAnimationCount(morphAnimationCount)
            {
                m_descriptionText = AZStd::string::format("%zu morph target motions imported", morphAnimationCount);
            }

            void MorphTargetRuleReadOnly::SetMorphAnimationCount(size_t morphAnimationCount)
            {
                m_morphAnimationCount = morphAnimationCount;
                m_descriptionText = AZStd::string::format("%zu morph target motions imported", morphAnimationCount);
            }

            size_t MorphTargetRuleReadOnly::GetMorphAnimationCount() const
            {
                return m_morphAnimationCount;
            }

            void MorphTargetRuleReadOnly::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MorphTargetRuleReadOnly, IRule>()->Version(1)->
                    Field("morphCount", &MorphTargetRuleReadOnly::m_morphAnimationCount)->
                    Field("staticDescription", &MorphTargetRuleReadOnly::m_descriptionText);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MorphTargetRuleReadOnly>("Morph Targets", "This should be hidden!")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
                        ->DataElement(AZ_CRC_CE("ManifestName"), &MorphTargetRuleReadOnly::m_descriptionText, "Morph target motions",
                            "Morph targets involved in motion.")
                            ->Attribute("ReadOnly", true);
                }
            }

            size_t MorphTargetRuleReadOnly::DetectMorphTargetAnimations(const AZ::SceneAPI::Containers::Scene& scene)
            {
                size_t morphTargetAnimations = 0;
                const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
                auto contentStorage = graph.GetContentStorage();
                auto nameStorage = graph.GetNameStorage();

                auto keyValueView = AZ::SceneAPI::Containers::Views::MakePairView(nameStorage, contentStorage);
                auto filteredView = AZ::SceneAPI::Containers::Views::MakeFilterView(keyValueView, AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IBlendShapeData>());
                for (auto it = filteredView.begin(); it != filteredView.end(); ++it)
                {
                    AZStd::unordered_set<AZ::Crc32> types;
                    auto keyValueIterator = it.GetBaseIterator();
                    AZ::SceneAPI::Containers::SceneGraph::NodeIndex index = graph.ConvertToNodeIndex(keyValueIterator.GetFirstIterator());
                    AZ::SceneAPI::Events::GraphMetaInfoBus::Broadcast(
                        &AZ::SceneAPI::Events::GraphMetaInfoBus::Events::GetVirtualTypes, types, scene, index);
                    if (types.find(AZ::SceneAPI::Events::GraphMetaInfo::GetIgnoreVirtualType()) == types.end())
                    {
                        morphTargetAnimations++;
                    }
                }
                return morphTargetAnimations;
            }

        } // namespace Rule
    } // namespace Pipeline
} // namespace EMotionFX
