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
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISceneNodeGroup.h>
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <SceneAPIExt/Groups/ActorGroup.h>
#include <SceneAPIExt/Behaviors/ActorGroupBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Group
        {
            AZ_CLASS_ALLOCATOR_IMPL(ActorGroup, AZ::SystemAllocator, 0)

            ActorGroup::ActorGroup()
                : m_id(AZ::Uuid::CreateRandom())
            {
            }

            const AZStd::string& ActorGroup::GetName() const
            {
                return m_name;
            }

            void ActorGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void ActorGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const AZ::Uuid& ActorGroup::GetId() const
            {
                return m_id;
            }

            void ActorGroup::OverrideId(const AZ::Uuid& id)
            {
                m_id = id;
            }

            AZ::SceneAPI::Containers::RuleContainer& ActorGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const AZ::SceneAPI::Containers::RuleContainer& ActorGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }

            const AZStd::string & ActorGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            void ActorGroup::SetSelectedRootBone(const AZStd::string & selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void ActorGroup::SetBestMatchingRootBone(const AZ::SceneAPI::Containers::SceneGraph& sceneGraph)
            {
                auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(sceneGraph.GetNameStorage(), sceneGraph.GetContentStorage());
                auto graphDownwardsView = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<AZ::SceneAPI::Containers::Views::BreadthFirst>(
                    sceneGraph, sceneGraph.GetRoot(), nameContentView.begin(), true);

                for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
                {
                    if (it->second && it->second->RTTI_IsTypeOf(AZ::SceneData::GraphData::RootBoneData::TYPEINFO_Uuid()))
                    {
                        SetSelectedRootBone(it->first.GetPath());
                        return;
                    }
                }
            }

            void ActorGroup::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    // Check if the context is serialized and has actorGroup class data. 
                    return;
                }

                serializeContext->Class<IActorGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(3, IActorGroupVersionConverter);

                serializeContext->Class<ActorGroup, IActorGroup>()->Version(7, ActorVersionConverter)
                    ->Field("name", &ActorGroup::m_name)
                    ->Field("selectedRootBone", &ActorGroup::m_selectedRootBone)
                    ->Field("id", &ActorGroup::m_id)
                    ->Field("rules", &ActorGroup::m_rules);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<ActorGroup>("Actor group", "Configure actor data exporting.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &ActorGroup::m_name, "Name actor",
                            "Name for the group. This name will also be used as the name for the generated file.")
                            ->Attribute("FilterType", IActorGroup::TYPEINFO_Uuid())
                        ->DataElement("NodeListSelection", &ActorGroup::m_selectedRootBone, "Select root bone", "The root bone of the animation that will be exported.")
                            ->Attribute("ClassTypeIdFilter", AZ::SceneAPI::DataTypes::IBoneData::TYPEINFO_Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }

             bool ActorGroup::IActorGroupVersionConverter(
                [[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();
                bool result = true;

                // In version 1 IActorGroup is directly inherit from IGroup, so there is nothing to do.
                // In version 2 IActroGroup is inherit from ISceneNodeGroup, which inherit from IGroup. We need to remove ISceneNodeGroup in-between.
                if (version == 2)
                {
                    AZ::SerializeContext::DataElementNode iSceneNodeGroupNode = classElement.GetSubElement(0);
                    AZ::SerializeContext::DataElementNode iGroupNode = iSceneNodeGroupNode.GetSubElement(0);
                    classElement.RemoveElement(0);
                    result = result && classElement.AddElement(iGroupNode);
                }

                return result;
            }


            bool ActorGroup::ActorVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();

                bool result = true;
                // Added a uuid "id" as the unique identifier to replace the file name.
                // Setting it to null by default and expecting a behavior to patch this when additional information is available.
                if (version < 2)
                {
                    result = result && classElement.AddElementWithData<AZ::Uuid>(context, "id", AZ::Uuid::CreateNull()) != -1;
                    classElement.RemoveElementByName(AZ_CRC("autoCreateTrajectoryNode"));
                }

                if (version < 3)
                {
                    classElement.RemoveElementByName(AZ_CRC("loadMorphTargets", 0xfd19aef8));
                }

                // Coordinate system rule moved to the SceneAPI
                if (version < 5)
                {
                    if (!AZ::SceneAPI::SceneData::CoordinateSystemRule::ConvertLegacyCoordinateSystemRule(context, classElement))
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Cannot convert legacy coordinate system rule.\n");
                        return false;
                    }
                }

                if (version < 6)
                {
                    classElement.RemoveElementByName(AZ_CRC("nodeSelectionList"));
                }

                return result;
            }
        }
    }
}
