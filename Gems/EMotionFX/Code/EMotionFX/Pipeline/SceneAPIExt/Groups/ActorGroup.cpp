/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
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
            
            // The scene node selection list are supposed to store all the nodes that we want to export for this group.
            // If you left any mesh, the sub mtl file that belongs to this mesh won't be generated in the mtl and dccmtl file.
            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& ActorGroup::GetSceneNodeSelectionList()
            {
                return m_allNodeSelectionList;
            }

            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& ActorGroup::GetSceneNodeSelectionList() const
            {
                return m_allNodeSelectionList;
            }

            AZ::SceneAPI::DataTypes::ISceneNodeSelectionList & ActorGroup::GetBaseNodeSelectionList()
            {
                return m_baseNodeSelectionList;
            }

            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList & ActorGroup::GetBaseNodeSelectionList() const
            {
                return m_baseNodeSelectionList;
            }

            const AZStd::string & ActorGroup::GetSelectedRootBone() const
            {
                return m_selectedRootBone;
            }

            void ActorGroup::SetSelectedRootBone(const AZStd::string & selectedRootBone)
            {
                m_selectedRootBone = selectedRootBone;
            }

            void ActorGroup::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    // Check if the context is serialized and has actorGroup class data. 
                    return;
                }

                serializeContext->Class<IActorGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(2, SceneNodeVersionConverter);

                serializeContext->Class<ActorGroup, IActorGroup>()->Version(4, ActorVersionConverter)
                    ->Field("name", &ActorGroup::m_name)
                    ->Field("selectedRootBone", &ActorGroup::m_selectedRootBone)
                    // In version 4, we stores the reflected node list on the base mesh list. 
                    // The all mesh list is populated in the actor group behavior, so we don't reflect it in the serialization context.
                    ->Field("nodeSelectionList", &ActorGroup::m_baseNodeSelectionList)
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
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorGroup::m_baseNodeSelectionList, "Select base meshes", "Select the base meshes to be included in the actor. These meshes belong to LOD 0.")
                            ->Attribute("FilterName", "meshes")
                            ->Attribute("NarrowSelection", true)
                            ->Attribute("FilterType", AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &ActorGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }

            bool ActorGroup::SceneNodeVersionConverter(AZ::SerializeContext & context, AZ::SerializeContext::DataElementNode & classElement)
            {
                const unsigned int version = classElement.GetVersion();
                bool result = true;

                if (version < 2)
                {
                    // In version 1 IActorGroup is directly inherit from IGroup, we need to add ISceneNodeGroup inheritance in between. 
                    AZ::SerializeContext::DataElementNode iGroupNode = classElement.GetSubElement(0);
                    classElement.RemoveElement(0);
                    result = result && classElement.AddElement<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(context, "BaseClass1");
                    AZ::SerializeContext::DataElementNode& iSceneNodeGroupNode = classElement.GetSubElement(0);
                    result = result && iSceneNodeGroupNode.AddElement(iGroupNode);
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

                return result;
            }
        }
    }
}
