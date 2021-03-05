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

#include <Material/EditorMaterialComponentInspector.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentInspector
        {
            MaterialPropertyInspector::MaterialPropertyInspector(const AZ::Data::AssetId& assetId, PropertyChangedCallback propertyChangedCallback, QWidget* parent)
                : AtomToolsFramework::InspectorWidget(parent)
                , m_materialAssetId(assetId)
                , m_propertyChangedCallback(propertyChangedCallback)
            {
            }

            MaterialPropertyInspector::~MaterialPropertyInspector()
            {
                AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
            }

            bool MaterialPropertyInspector::LoadMaterial()
            {
                if (!m_materialAssetId.IsValid())
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentInspector", false, "Attempted to load material data for invalid asset id.");
                    return false;
                }

                // Load the originating product asset from which the new source has set will be generated
                auto materialAssetOutcome = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(m_materialAssetId);
                if (!materialAssetOutcome)
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Failed to load material asset: %s", m_materialAssetId.ToString<AZStd::string>().c_str());
                    return false;
                }

                m_materialAsset = materialAssetOutcome.GetValue();
                m_materialTypeAsset = m_materialAsset->GetMaterialTypeAsset();
                m_parentMaterialAsset = {};

                // The material instance is still needed for functor execution
                m_materialInstance = AZ::RPI::Material::Create(m_materialAsset);
                if (!m_materialInstance)
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Material instance could not be created.");
                    return false;
                }

                // We need a valid path to the material type source data because it's required for to get the property layout and assign to the new material
                const AZStd::string& materialTypePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(m_materialTypeAsset.GetId());
                if (materialTypePath.empty())
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Failed to locate source material type asset: %s", m_materialAssetId.ToString<AZStd::string>().c_str());
                    return false;
                }

                // Getting the source info for the material type file to make sure that it exists
                // We also need to watch folder to generate a relative asset path for the material type
                bool result = false;
                AZ::Data::AssetInfo info;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, materialTypePath.c_str(), info, watchFolder);
                if (!result)
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Failed to get source file info and asset path: %s", materialTypePath.c_str());
                    return false;
                }

                // At this point, we should be ready to attempt to load the material type data
                auto materialTypeOutcome = RPI::MaterialUtils::LoadMaterialTypeSourceData(materialTypePath);
                if (!materialTypeOutcome.IsSuccess())
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Failed to get load material type source data: %s", materialTypePath.c_str());
                    return false;
                }
                else
                {
                    m_materialTypeSourceData = materialTypeOutcome.GetValue();
                }

                // Get a list of all the editor functors to be used for property editor states
                auto propertyLayout = m_materialAsset->GetMaterialPropertiesLayout();
                const AZ::RPI::MaterialFunctorSourceData::EditorContext editorContext = AZ::RPI::MaterialFunctorSourceData::EditorContext(materialTypePath, propertyLayout);
                for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder> functorData : m_materialTypeSourceData.m_materialFunctorSourceData)
                {
                    AZ::RPI::MaterialFunctorSourceData::FunctorResult createResult = functorData->CreateFunctor(editorContext);

                    if (createResult.IsSuccess())
                    {
                        AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor = createResult.GetValue();
                        if (functor != nullptr)
                        {
                            m_editorFunctors.push_back(functor);
                        }
                    }
                    else
                    {
                        AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Material functors were not created: '%s'.", materialTypePath.c_str());
                    }
                }

                return true;
            }

            void MaterialPropertyInspector::Reset()
            {
                m_activeProperty = {};
                m_groups = {};
                m_dirtyPropertyFlags.set();

                AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
                AtomToolsFramework::InspectorWidget::Reset();
            }

            void MaterialPropertyInspector::AddDetailsGroup()
            {
                const AZStd::string& groupNameId = "Details";
                const AZStd::string& groupDisplayName = "Details";
                const AZStd::string& groupDescription = "";
                auto& group = m_groups[groupNameId];

                AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Asset;
                propertyConfig.m_id = "details.materialType";
                propertyConfig.m_nameId = "materialType";
                propertyConfig.m_displayName = "Material Type";
                propertyConfig.m_description = propertyConfig.m_displayName;
                propertyConfig.m_defaultValue = AZStd::any(m_materialAsset->GetMaterialTypeAsset());
                propertyConfig.m_originalValue = propertyConfig.m_defaultValue;
                propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
                propertyConfig.m_readOnly = true;
                group.m_properties.emplace_back(propertyConfig);

                propertyConfig = {};
                propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Asset;
                propertyConfig.m_id = "details.parentMaterial";
                propertyConfig.m_nameId = "parentMaterial";
                propertyConfig.m_displayName = "Parent Material";
                propertyConfig.m_description = propertyConfig.m_displayName;
                propertyConfig.m_defaultValue = AZStd::any(m_parentMaterialAsset);
                propertyConfig.m_originalValue = propertyConfig.m_defaultValue;
                propertyConfig.m_parentValue = propertyConfig.m_defaultValue;
                propertyConfig.m_readOnly = true;
                group.m_properties.emplace_back(propertyConfig);

                AddGroup(groupNameId, groupDisplayName, groupDescription,
                    new AtomToolsFramework::InspectorPropertyGroupWidget(&group, group.TYPEINFO_Uuid(), this));
            }

            void MaterialPropertyInspector::AddUvNamesGroup()
            {
                const AZStd::string groupNameId = AZ::RPI::UvGroupName;
                const AZStd::string groupDisplayName = "UV Names";
                const AZStd::string groupDescription = "UV set names in this material, which can be renamed to match those in the model.";
                auto& group = m_groups[groupNameId];

                const RPI::MaterialUvNameMap& uvNameMap = m_materialAsset->GetMaterialTypeAsset()->GetUvNameMap();
                group.m_properties.reserve(uvNameMap.size());

                for (const RPI::UvNamePair& uvNamePair : uvNameMap)
                {
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;

                    const AZStd::string shaderInputStr = uvNamePair.m_shaderInput.ToString();
                    const AZStd::string uvName = uvNamePair.m_uvName.GetStringView();

                    propertyConfig = {};
                    propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::String;
                    propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupNameId, shaderInputStr).GetCStr();
                    propertyConfig.m_nameId = shaderInputStr;
                    propertyConfig.m_displayName = shaderInputStr;
                    propertyConfig.m_description = shaderInputStr;
                    propertyConfig.m_defaultValue = uvName;
                    propertyConfig.m_originalValue = uvName;
                    propertyConfig.m_parentValue = uvName;
                    propertyConfig.m_readOnly = true;
                    group.m_properties.emplace_back(propertyConfig);
                }

                AddGroup(groupNameId, groupDisplayName, groupDescription,
                    new AtomToolsFramework::InspectorPropertyGroupWidget(&group, group.TYPEINFO_Uuid(), this));
            }

            void MaterialPropertyInspector::Populate()
            {
                AddGroupsBegin();

                AddDetailsGroup();
                AddUvNamesGroup();

                // Copy all of the properties from the material asset to the source data that will be exported
                for (const auto& groupDefinition : m_materialTypeSourceData.GetGroupDefinitionsInDisplayOrder())
                {
                    const AZStd::string& groupNameId = groupDefinition.m_nameId;
                    const AZStd::string& groupDisplayName = !groupDefinition.m_displayName.empty() ? groupDefinition.m_displayName : groupNameId;
                    const AZStd::string& groupDescription = !groupDefinition.m_description.empty() ? groupDefinition.m_description : groupDisplayName;
                    auto& group = m_groups[groupNameId];

                    const auto& propertyLayout = m_materialTypeSourceData.m_propertyLayout;
                    const auto& propertyListItr = propertyLayout.m_properties.find(groupNameId);
                    if (propertyListItr != propertyLayout.m_properties.end())
                    {
                        group.m_properties.reserve(propertyListItr->second.size());
                        for (const auto& propertyDefinition : propertyListItr->second)
                        {
                            AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                            AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDefinition);

                            propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupNameId, propertyDefinition.m_nameId).GetFullName();
                            const auto& propertyIndex = m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
                            propertyConfig.m_parentValue = AtomToolsFramework::ConvertToEditableType(m_materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()]);
                            propertyConfig.m_originalValue = AtomToolsFramework::ConvertToEditableType(m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                            group.m_properties.emplace_back(propertyConfig);
                        }
                    }

                    AddGroup(groupNameId, groupDisplayName, groupDescription,
                        new AtomToolsFramework::InspectorPropertyGroupWidget(&group, group.TYPEINFO_Uuid(), this));
                }

                AddGroupsEnd();

                m_dirtyPropertyFlags.set();
                RunEditorMaterialFunctors();
            }

            void MaterialPropertyInspector::RunPropertyChangedCallback()
            {
                if (m_propertyChangedCallback)
                {
                    m_propertyChangedCallback(m_materialPropertyOverrideMap);
                }
            }

            void MaterialPropertyInspector::RunEditorMaterialFunctors()
            {
                AZStd::unordered_set<AZ::Name> changedPropertyNames;

                // Convert editor property configuration data into material property meta data so that it can be used to execute functors
                AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
                for (auto& group : m_groups)
                {
                    for (auto& property : group.second.m_properties)
                    {
                        AtomToolsFramework::ConvertToPropertyMetaData(propertyDynamicMetadata[property.GetId()], property.GetConfig());
                    }
                }

                for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor : m_editorFunctors)
                {
                    const AZ::RPI::MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();
                    // None also covers case that the client code doesn't register material properties to dependencies,
                    // which will later get caught in Process() when trying to access a property.
                    if (materialPropertyDependencies.none() || functor->NeedsProcess(m_dirtyPropertyFlags))
                    {
                        AZ::RPI::MaterialFunctor::EditorContext context = AZ::RPI::MaterialFunctor::EditorContext(
                            m_materialInstance->GetPropertyValues(),
                            m_materialInstance->GetMaterialPropertiesLayout(),
                            propertyDynamicMetadata,
                            changedPropertyNames,
                            &materialPropertyDependencies
                        );
                        functor->Process(context);
                    }
                }
                m_dirtyPropertyFlags.reset();

                // Apply any changes to material property meta data back to the editor property configurations
                for (auto& group : m_groups)
                {
                    for (auto& property : group.second.m_properties)
                    {
                        AtomToolsFramework::DynamicPropertyConfig propertyConfig = property.GetConfig();

                        const auto oldVisible = propertyConfig.m_visible;
                        const auto oldReadOnly = propertyConfig.m_readOnly;
                        AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDynamicMetadata[property.GetId()]);
                        property.SetConfig(propertyConfig);

                        if (oldReadOnly != propertyConfig.m_readOnly)
                        {
                            RefreshAll();
                        }

                        if (oldVisible != propertyConfig.m_visible)
                        {
                            RebuildAll();
                        }
                    }
                }
            }

            void MaterialPropertyInspector::UpdateMaterialInstanceProperty(const AtomToolsFramework::DynamicProperty& property)
            {
                if (m_materialInstance)
                {
                    const auto propertyIndex = m_materialInstance->FindPropertyIndex(property.GetId());
                    if (!propertyIndex.IsNull())
                    {
                        m_dirtyPropertyFlags.set(propertyIndex.GetIndex());

                        const auto runtimeValue = AtomToolsFramework::ConvertToRuntimeType(property.GetValue());
                        if (runtimeValue.IsValid())
                        {
                            m_materialInstance->SetPropertyValue(propertyIndex, runtimeValue);
                        }
                    }
                }
            }

            void MaterialPropertyInspector::SetOverrides(const MaterialPropertyOverrideMap& propertyOverrideMap)
            {
                m_materialPropertyOverrideMap = propertyOverrideMap;

                for (auto& group : m_groups)
                {
                    for (auto& property : group.second.m_properties)
                    {
                        const AtomToolsFramework::DynamicPropertyConfig& propertyConfig = property.GetConfig();
                        const auto overrideItr = m_materialPropertyOverrideMap.find(propertyConfig.m_id);
                        const auto& editValue = overrideItr != m_materialPropertyOverrideMap.end() ? overrideItr->second : propertyConfig.m_originalValue;

                        // This first converts to an acceptable runtime type in case the value came from script
                        const auto propertyIndex = m_materialInstance->FindPropertyIndex(property.GetId());
                        if (!propertyIndex.IsNull())
                        {
                            const auto runtimeValue = AtomToolsFramework::ConvertToRuntimeType(editValue);
                            if (runtimeValue.IsValid())
                            {
                                property.SetValue(AtomToolsFramework::ConvertToEditableType(runtimeValue));
                            }
                        }
                        else
                        {
                            property.SetValue(editValue);
                        }

                        UpdateMaterialInstanceProperty(property);
                    }
                }

                m_dirtyPropertyFlags.set();
                RunPropertyChangedCallback();
                RunEditorMaterialFunctors();
                RebuildAll();
            }

            void MaterialPropertyInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                // For some reason the reflected property editor notifications are not symmetrical
                // This function is called continuously anytime a property changes until the edit has completed
                // Because of that, we have to track whether or not we are continuing to edit the same property to know when editing has started and ended
                const AtomToolsFramework::DynamicProperty* property = FindPropertyForNode(pNode);
                if (property)
                {
                    if (m_activeProperty != property)
                    {
                        m_activeProperty = property;
                    }
                }
            }

            void MaterialPropertyInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                const AtomToolsFramework::DynamicProperty* property = FindPropertyForNode(pNode);
                if (property)
                {
                    if (m_activeProperty == property)
                    {
                        m_materialPropertyOverrideMap[m_activeProperty->GetId()] = m_activeProperty->GetValue();
                        UpdateMaterialInstanceProperty(*m_activeProperty);
                        RunPropertyChangedCallback();
                    }
                }
            }

            void MaterialPropertyInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
            {
                // As above, there are symmetrical functions on the notification interface for when editing begins and ends and has been completed but they are not being called following that pattern.
                // when this function executes the changes to the property are ready to be committed or reverted
                const AtomToolsFramework::DynamicProperty* property = FindPropertyForNode(pNode);
                if (property)
                {
                    if (m_activeProperty == property)
                    {
                        m_materialPropertyOverrideMap[m_activeProperty->GetId()] = m_activeProperty->GetValue();
                        UpdateMaterialInstanceProperty(*m_activeProperty);
                        RunPropertyChangedCallback();
                        RunEditorMaterialFunctors();
                        m_activeProperty = nullptr;
                    }
                }
            }

            const AtomToolsFramework::DynamicProperty* MaterialPropertyInspector::FindPropertyForNode(AzToolsFramework::InstanceDataNode* pNode) const
            {
                // Traverse up the hierarchy from the input node to search for an instance corresponding to material inspector property
                const AZ::SerializeContext::ClassElement* elementData = pNode->GetElementMetadata();
                for (const AzToolsFramework::InstanceDataNode* currentNode = pNode; currentNode; currentNode = currentNode->GetParent())
                {
                    const AZ::SerializeContext* context = currentNode->GetSerializeContext();
                    const AZ::SerializeContext::ClassData* classData = currentNode->GetClassMetadata();
                    if (context && classData)
                    {
                        if (context->CanDowncast(classData->m_typeId, azrtti_typeid<AtomToolsFramework::DynamicProperty>(), classData->m_azRtti, nullptr))
                        {
                            return static_cast<const AtomToolsFramework::DynamicProperty*>(currentNode->FirstInstance());
                        }
                    }
                }

                return nullptr;
            }

            bool OpenInspectorDialog(const AZ::Data::AssetId& assetId, MaterialPropertyOverrideMap propertyOverrideMap, PropertyChangedCallback propertyChangedCallback)
            {
                // Constructing a dialog with a table to display all configurable material export items
                QDialog dialog(QApplication::activeWindow());
                dialog.setWindowTitle("Material Inspector");
                dialog.setFixedSize(300, 600);

                MaterialPropertyInspector* inspector = new MaterialPropertyInspector(assetId, propertyChangedCallback, &dialog);
                if (!inspector->LoadMaterial())
                {
                    return false;
                }

                inspector->Populate();
                inspector->SetOverrides(propertyOverrideMap);

                // Create the bottom row of the dialog with action buttons for exporting or canceling the operation
                QWidget* buttonRow = new QWidget(&dialog);
                buttonRow->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

                QPushButton* revertButton = new QPushButton("Revert", buttonRow);
                QObject::connect(revertButton, &QPushButton::clicked, revertButton, [inspector, propertyOverrideMap] {
                    inspector->SetOverrides(propertyOverrideMap);
                    });

                QPushButton* clearButton = new QPushButton("Clear", buttonRow);
                QObject::connect(clearButton, &QPushButton::clicked, clearButton, [inspector] {
                    inspector->SetOverrides(MaterialPropertyOverrideMap());
                    });

                QPushButton* confirmButton = new QPushButton("Confirm", buttonRow);
                QObject::connect(confirmButton, &QPushButton::clicked, confirmButton, [&dialog] {
                    dialog.done(1);
                    });

                QPushButton* cancelButton = new QPushButton("Cancel", buttonRow);
                QObject::connect(cancelButton, &QPushButton::clicked, cancelButton, [inspector, propertyOverrideMap, &dialog] {
                    inspector->SetOverrides(propertyOverrideMap);
                    dialog.done(0);
                    });

                QHBoxLayout* buttonLayout = new QHBoxLayout(buttonRow);
                buttonLayout->addStretch();
                buttonLayout->addWidget(revertButton);
                buttonLayout->addWidget(clearButton);
                buttonLayout->addWidget(confirmButton);
                buttonLayout->addWidget(cancelButton);

                QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);
                dialogLayout->addWidget(inspector);
                dialogLayout->addWidget(buttonRow);
                dialog.setLayout(dialogLayout);

                // Return true if the user press the export button
                return dialog.exec() == 1;
            }
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ

//#include <AtomLyIntegration/CommonFeatures/moc_EditorMaterialComponentInspector.cpp>
