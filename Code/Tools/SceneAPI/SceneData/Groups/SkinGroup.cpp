/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/Groups/SkinGroup.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneData/Behaviors/SkinGroup.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            AZ_CLASS_ALLOCATOR_IMPL(SkinGroup, AZ::SystemAllocator)

            SkinGroup::SkinGroup()
                : m_id(Uuid::CreateRandom())
            {
            }

            const AZStd::string& SkinGroup::GetName() const
            {
                return m_name;
            }

            void SkinGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void SkinGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const Uuid& SkinGroup::GetId() const
            {
                return m_id;
            }

            void SkinGroup::OverrideId(const Uuid& id)
            {
                m_id = id;
            }

            Containers::RuleContainer& SkinGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const Containers::RuleContainer& SkinGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }

            DataTypes::ISceneNodeSelectionList& SkinGroup::GetSceneNodeSelectionList()
            {
                return m_nodeSelectionList; 
            }

            const DataTypes::ISceneNodeSelectionList& SkinGroup::GetSceneNodeSelectionList() const 
            {
                return m_nodeSelectionList; 
            }

            void SkinGroup::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<SkinGroup, DataTypes::ISkinGroup>()->Version(3, VersionConverter)
                    ->Field("name", &SkinGroup::m_name)
                    ->Field("nodeSelectionList", &SkinGroup::m_nodeSelectionList)
                    ->Field("rules", &SkinGroup::m_rules)
                    ->Field("id", &SkinGroup::m_id);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<SkinGroup>("Skin group", "Name and configure 1 or more skins from your source file.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                        ->DataElement(AZ_CRC_CE("ManifestName"), &SkinGroup::m_name, "Name skin",
                            "Name the skin as you want it to appear in the Open 3D Engine Asset Browser.")
                            ->Attribute("FilterType", DataTypes::ISkinGroup::TYPEINFO_Uuid())
                        ->DataElement(AZ_CRC_CE("ManifestName"), &SkinGroup::m_nodeSelectionList, "Select skins", "Select 1 or more skins to add to this asset in the Open 3D Engine Asset Browser.")
                            ->Attribute("FilterName", "skins")
                            ->Attribute("FilterVirtualType", Behaviors::SkinGroup::s_skinVirtualType)
                        ->DataElement(Edit::UIHandlers::Default, &SkinGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"));
                }
            }


            bool SkinGroup::VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();

                bool result = true;

                // Replaced vector<IRule> with RuleContainer.
                if (version == 1)
                {
                    result = result && Containers::RuleContainer::VectorToRuleContainerConverter(context, classElement);
                }

                // Added a uuid "id" as the unique identifier to replace the file name.
                // Setting it to null by default and expecting a behavior to patch this when additional information is available.
                if (version <= 2)
                {
                    result = result && classElement.AddElementWithData<AZ::Uuid>(context, "id", AZ::Uuid::CreateNull()) != -1;
                }

                return result;
            }

        } // namespace SceneData
    } // namespace SceneAPI
} // namespace AZ
