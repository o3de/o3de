/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneData/Groups/MeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestVectorHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneData
        {
            MeshGroup::MeshGroup()
                : m_id(Uuid::CreateRandom())
            {
            }

            const AZStd::string& MeshGroup::GetName() const
            {
                return m_name;
            }

            void MeshGroup::SetName(const AZStd::string& name)
            {
                m_name = name;
            }

            void MeshGroup::SetName(AZStd::string&& name)
            {
                m_name = AZStd::move(name);
            }

            const Uuid& MeshGroup::GetId() const
            {
                return m_id;
            }

            void MeshGroup::OverrideId(const Uuid& id)
            {
                m_id = id;
            }

            Containers::RuleContainer& MeshGroup::GetRuleContainer()
            {
                return m_rules;
            }

            const Containers::RuleContainer& MeshGroup::GetRuleContainerConst() const
            {
                return m_rules;
            }
            
            DataTypes::ISceneNodeSelectionList& MeshGroup::GetSceneNodeSelectionList()
            {
                return m_nodeSelectionList;
            }

            const DataTypes::ISceneNodeSelectionList& MeshGroup::GetSceneNodeSelectionList() const
            {
                return m_nodeSelectionList;
            }

            void MeshGroup::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<MeshGroup, DataTypes::IMeshGroup>()
                    ->Version(3, VersionConverter)
                    ->Field("name", &MeshGroup::m_name)
                    ->Field("nodeSelectionList", &MeshGroup::m_nodeSelectionList)
                    ->Field("rules", &MeshGroup::m_rules)
                    ->Field("id", &MeshGroup::m_id);

                EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<MeshGroup>("Mesh group", "Name and configure 1 or more meshes from your source file.")
                        ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute("AutoExpand", true)
                            ->Attribute(Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::CategoryStyle, "display divider")
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://www.o3de.org/docs/user-guide/assets/scene-settings/meshes-tab/")
                        ->DataElement(AZ_CRC_CE("ManifestName"), &MeshGroup::m_name, "Name mesh",
                            "Name the mesh as you want it to appear in the Open 3D Engine Asset Browser.")
                            ->Attribute("FilterType", DataTypes::IMeshGroup::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &MeshGroup::m_nodeSelectionList, "Select meshes", "Select 1 or more meshes to add to this asset in the Open 3D Engine Asset Browser.")
                            ->Attribute("FilterName", "meshes")
                            ->Attribute("FilterType", DataTypes::IMeshData::TYPEINFO_Uuid())
                        ->DataElement(Edit::UIHandlers::Default, &MeshGroup::m_rules, "", "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"));
                }
            }


            bool MeshGroup::VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement)
            {
                const unsigned int version = classElement.GetVersion();

                // Replaced vector<IRule> with RuleContainer.
                bool result = true;
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
