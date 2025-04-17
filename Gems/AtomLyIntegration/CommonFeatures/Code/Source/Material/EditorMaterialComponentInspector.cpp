/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/EditorMaterialComponentInspector.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialNameContext.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConfig.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QToolTip>
#include <QWidget>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentInspector
        {
            MaterialPropertyInspector::MaterialPropertyInspector(QWidget* parent)
                : AtomToolsFramework::InspectorWidget(parent)
            {
                CreateHeading();
                AZ::SystemTickBus::Handler::BusConnect();
                AZ::EntitySystemBus::Handler::BusConnect();
                EditorMaterialSystemComponentNotificationBus::Handler::BusConnect();
            }

            MaterialPropertyInspector::~MaterialPropertyInspector()
            {
                AZ::SystemTickBus::Handler::BusDisconnect();
                AZ::EntitySystemBus::Handler::BusDisconnect();
                EditorMaterialSystemComponentNotificationBus::Handler::BusDisconnect();
                MaterialComponentNotificationBus::MultiHandler::BusDisconnect();
            }

            bool MaterialPropertyInspector::LoadMaterial(
                const AZ::EntityId& primaryEntityId,
                const AzToolsFramework::EntityIdSet& entityIdsToEdit,
                const AZ::Render::MaterialAssignmentId& materialAssignmentId)
            {
                UnloadMaterial();

                // Only allow the load to succeed if all of the affected entities have matching material types to guarantee that the
                // inspector configuration matches all of the entities
                if (!EditorMaterialComponentUtil::DoEntitiesHaveMatchingMaterialTypes(primaryEntityId, entityIdsToEdit, materialAssignmentId))
                {
                    UnloadMaterial();
                    return false;
                }

                m_primaryEntityId = primaryEntityId;
                m_entityIdsToEdit = entityIdsToEdit;
                m_materialAssignmentId = materialAssignmentId;

                // Connect all of the affected entities to the material component notification bus so that the UI can be updated or
                // invalidated whenever any of their configurations change in a way that may not be compatible with the other entities 
                MaterialComponentNotificationBus::MultiHandler::BusDisconnect();
                MaterialComponentNotificationBus::MultiHandler::BusConnect(m_primaryEntityId);
                for (const AZ::EntityId& entityId : m_entityIdsToEdit)
                {
                    MaterialComponentNotificationBus::MultiHandler::BusConnect(entityId);
                }

                const AZ::Data::AssetId materialAssetId = GetActiveMaterialAssetIdFromEntity();
                if (!materialAssetId.IsValid())
                {
                    UnloadMaterial();
                    return false;
                }

                if (!EditorMaterialComponentUtil::LoadMaterialEditDataFromAssetId(materialAssetId, m_editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentInspector", false, "Failed to load material data.");
                    UnloadMaterial();
                    return false;
                }

                // The material instance is still needed for functor execution
                m_materialInstance = AZ::RPI::Material::Create(m_editData.m_materialAsset);
                if (!m_materialInstance)
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Material instance could not be created.");
                    UnloadMaterial();
                    return false;
                }
                
                // Add material functors that are in the top-level functors list. Other functors are also added per-property-group elsewhere.
                AddEditorMaterialFunctors(m_editData.m_materialTypeSourceData.m_materialFunctorSourceData, AZ::RPI::MaterialNameContext{});

                Populate();
                LoadOverridesFromEntity();
                return true;
            }

            void MaterialPropertyInspector::UnloadMaterial()
            {
                Reset();
                m_editData = EditorMaterialComponentUtil::MaterialEditData();
                m_materialInstance = {};
                m_dirtyPropertyFlags.set();
                m_editorFunctors = {};
                m_internalEditNotification = {};
                m_updateUI = {};
                m_updatePreview = {};
                UpdateHeading();
            }

            bool MaterialPropertyInspector::IsLoaded() const
            {
                // The inspector only has a valid configuration if the entity ID, material assignment ID, and material asset are all valid
                // and match what is on the selected entity. If there is a mismatch, the content must be reloaded.
                const AZ::Data::AssetId materialAssetId = GetActiveMaterialAssetIdFromEntity();
                return m_primaryEntityId.IsValid() && m_materialInstance && m_editData.m_materialAsset.IsReady() &&
                    m_editData.m_materialAsset.GetId() == materialAssetId && m_editData.m_materialAssetId == materialAssetId &&
                    EditorMaterialComponentUtil::DoEntitiesHaveMatchingMaterialTypes(m_primaryEntityId, m_entityIdsToEdit, m_materialAssignmentId);
            }

            void MaterialPropertyInspector::Reset()
            {
                m_groups = {};
                m_dirtyPropertyFlags.set();
                m_internalEditNotification = {};

                AtomToolsFramework::InspectorWidget::Reset();
            }

            void MaterialPropertyInspector::CreateHeading()
            {
                // Create the menu button
                QToolButton* menuButton = new QToolButton(this);
                menuButton->setAutoRaise(true);
                menuButton->setIcon(QIcon(":/Cards/img/UI20/Cards/menu_ico.svg"));
                menuButton->setVisible(true);
                QObject::connect(menuButton, &QToolButton::clicked, this, [this]() { OpenMenu(); });
                AddHeading(menuButton);

                m_overviewImage = new QLabel(this);
                m_overviewImage->setFixedSize(QSize(120, 120));
                m_overviewImage->setScaledContents(true);
                m_overviewImage->setVisible(false);

                m_overviewText = new QLabel(this);
                QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Preferred);
                sizePolicy1.setHorizontalStretch(0);
                sizePolicy1.setVerticalStretch(0);
                sizePolicy1.setHeightForWidth(m_overviewText->sizePolicy().hasHeightForWidth());
                m_overviewText->setSizePolicy(sizePolicy1);
                m_overviewText->setMinimumSize(QSize(0, 0));
                m_overviewText->setMaximumSize(QSize(16777215, 16777215));
                m_overviewText->setTextFormat(Qt::AutoText);
                m_overviewText->setScaledContents(false);
                m_overviewText->setWordWrap(true);
                m_overviewText->setVisible(true);

                auto overviewContainer = new QWidget(this);
                overviewContainer->setLayout(new QHBoxLayout());
                overviewContainer->layout()->addWidget(m_overviewImage);
                overviewContainer->layout()->addWidget(m_overviewText);
                AddHeading(overviewContainer);
            }

            void MaterialPropertyInspector::UpdateHeading()
            {
                if (!IsLoaded())
                {
                    if (m_entityIdsToEdit.size() > 1)
                    {
                        m_overviewText->setText(tr("The selected entities and materials cannot be edited.\n"
                                                   "Multiple entities and materials have been selected for editing.\n"
                                                   "All of the selected entities must be valid, active, and have a material component.\n"
                                                   "Each material component must provide the selected material slot.\n"
                                                   "The active material on each slot must have the same material type."));
                    }
                    else
                    {
                        m_overviewText->setText(tr("The selected entities and materials cannot be edited.\n"
                                                   "The selected entity must be valid, active, and have a material component.\n"
                                                   "The material component must provide the selected material slot."));
                    }

                    m_overviewText->setAlignment(Qt::AlignCenter);
                    m_overviewImage->setVisible(false);
                    return;
                }

                AZStd::string entityName;
                AZ::ComponentApplicationBus::BroadcastResult(
                    entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, m_primaryEntityId);

                AZStd::string slotName;
                MaterialComponentRequestBus::EventResult(
                    slotName, m_primaryEntityId, &MaterialComponentRequestBus::Events::GetMaterialLabel, m_materialAssignmentId);

                QString materialInfo;
                materialInfo += tr("<table>");
                materialInfo += tr("<tr><td><b>Entity Name&emsp;</b></td><td>%1</td></tr>").arg(entityName.c_str());
                materialInfo += tr("<tr><td><b>Entity Count&emsp;</b></td><td>%1</td></tr>").arg(m_entityIdsToEdit.size());
                materialInfo += tr("<tr><td><b>Material Slot Name&emsp;</b></td><td>%1</td></tr>").arg(slotName.c_str());
                materialInfo += m_materialAssignmentId.IsDefault() || m_materialAssignmentId.IsSlotIdOnly()
                    ? tr("<tr><td><b>Material Slot LOD&emsp;</b></td><td>%1</td></tr>").arg(-1)
                    : tr("<tr><td><b>Material Slot LOD&emsp;</b></td><td>%1</td></tr>").arg(m_materialAssignmentId.m_lodIndex);

                if (m_editData.m_materialAsset.GetId().IsValid())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_editData.m_materialAsset.GetId());

                    materialInfo += tr("<tr><td><b>Material Asset&emsp;</b></td><td>%1</td></tr>").arg(assetInfo.m_relativePath.c_str());
                }
                if (!m_editData.m_materialSourcePath.empty())
                {
                    // Inserting links that will be used to open the material/type in the material editor
                    const auto& materialSourceFileName = GetFileName(m_editData.m_materialSourcePath);
                    if (IsSourceMaterial(m_editData.m_materialSourcePath))
                    {
                        materialInfo += tr("<tr><td><b>Material Source&emsp;</b></td><td><a href=\"%1\">%2</a></td></tr>")
                                            .arg(m_editData.m_materialSourcePath.c_str())
                                            .arg(materialSourceFileName.c_str());
                    }
                    else
                    {
                        // Materials that come from other sources like FBX files will not have the link
                        materialInfo += tr("<tr><td><b>Material Source&emsp;</b></td><td>%1</td></tr>")
                                            .arg(materialSourceFileName.c_str());
                    }
                }
                if (IsSourceMaterial(m_editData.m_materialParentSourcePath))
                {
                    // Inserting links that will be used to open the material/type in the material editor
                    const auto& materialParentSourceFileName = GetFileName(m_editData.m_materialParentSourcePath);
                    materialInfo += tr("<tr><td><b>Material Parent&emsp;</b></td><td><a href=\"%1\">%2</a></td></tr>")
                                        .arg(m_editData.m_materialParentSourcePath.c_str())
                                        .arg(materialParentSourceFileName.c_str());
                }
                if (!m_editData.m_originalMaterialTypeSourcePath.empty())
                {
                    // Inserting links that will be used to open the material/type in the material editor
                    const auto& materialTypeSourceFileName = GetFileName(m_editData.m_originalMaterialTypeSourcePath);
                    materialInfo += tr("<tr><td><b>Material Type&emsp;</b></td><td><a href=\"%1\">%2</a></td></tr>")
                                        .arg(m_editData.m_originalMaterialTypeSourcePath.c_str())
                                        .arg(materialTypeSourceFileName.c_str());
                }
                materialInfo += tr("</table>");

                m_overviewText->setText(materialInfo);
                m_overviewText->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);
                m_overviewText->setOpenExternalLinks(false);
                connect(m_overviewText, &QLabel::linkActivated, this, [](const QString& link) {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor, link.toUtf8().constData());
                });
                connect(m_overviewText, &QLabel::linkHovered, this, [](const QString& link) {
                    QToolTip::showText(QCursor::pos(), link);
                });

                // Update the overview image with the last rendered preview of the primary entity's material.
                QPixmap pixmap;
                EditorMaterialSystemComponentRequestBus::BroadcastResult(
                    pixmap, &EditorMaterialSystemComponentRequestBus::Events::GetRenderedMaterialPreview, m_primaryEntityId,
                    m_materialAssignmentId);
                m_overviewImage->setPixmap(pixmap);

                // If more than one entity is selected for editing in this inspector then the image will be hidden.
                // This will eliminate any confusion if editing multiple materials and they do not hold match the primary entities preview.
                m_overviewImage->setVisible(m_entityIdsToEdit.size() == 1);

                // If the image was not found then request that the preview be updated again at a later time
                m_updatePreview |= pixmap.isNull();
            }

            void MaterialPropertyInspector::AddUvNamesGroup()
            {
                const AZStd::string groupName = AZ::RPI::UvGroupName;
                const AZStd::string groupDisplayName = "UV Sets";
                const AZStd::string groupDescription = "UV set names in this material, which can be renamed to match those in the model.";
                auto& group = m_groups[groupName];

                const RPI::MaterialUvNameMap& uvNameMap = m_editData.m_materialAsset->GetMaterialTypeAsset()->GetUvNameMap();
                group.m_properties.reserve(uvNameMap.size());

                for (const RPI::UvNamePair& uvNamePair : uvNameMap)
                {
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;

                    const AZStd::string shaderInputStr = uvNamePair.m_shaderInput.ToString();
                    const AZStd::string uvName = uvNamePair.m_uvName.GetStringView();

                    propertyConfig = {};
                    propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupName, shaderInputStr).GetCStr();
                    propertyConfig.m_name = shaderInputStr;
                    propertyConfig.m_displayName = shaderInputStr;
                    propertyConfig.m_groupName = groupDisplayName;
                    propertyConfig.m_description = shaderInputStr;
                    propertyConfig.m_defaultValue = uvName;
                    propertyConfig.m_originalValue = uvName;
                    propertyConfig.m_parentValue = uvName;
                    propertyConfig.m_readOnly = true;
                    group.m_properties.emplace_back(propertyConfig);
                }

                // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
                auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                    &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName), {},
                    [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);
                AddGroup(groupName, groupDisplayName, groupDescription, propertyGroupWidget);
            }

            void MaterialPropertyInspector::AddPropertiesGroup()
            {
                // Copy all of the properties from the material asset to the populate the inspector
                m_editData.m_materialTypeSourceData.EnumeratePropertyGroups(
                    [this](const AZ::RPI::MaterialTypeSourceData::PropertyGroupStack& propertyGroupStack)
                    {
                        using namespace AZ::RPI;

                        const MaterialTypeSourceData::PropertyGroup* propertyGroupDefinition = propertyGroupStack.back();
                
                        MaterialNameContext groupNameContext = MaterialTypeSourceData::MakeMaterialNameContext(propertyGroupStack);
                        
                        AddEditorMaterialFunctors(propertyGroupDefinition->GetFunctors(), groupNameContext);

                        AZStd::vector<AZStd::string> groupNameVector;
                        AZStd::vector<AZStd::string> groupDisplayNameVector;
                        
                        groupNameVector.reserve(propertyGroupStack.size());
                        groupDisplayNameVector.reserve(propertyGroupStack.size());

                        for (auto& nextGroup : propertyGroupStack)
                        {
                            groupNameVector.push_back(nextGroup->GetName());
                            groupDisplayNameVector.push_back(!nextGroup->GetDisplayName().empty() ? nextGroup->GetDisplayName() : nextGroup->GetName());
                        }

                        AZStd::string groupId;
                        AzFramework::StringFunc::Join(groupId, groupNameVector.begin(), groupNameVector.end(), ".");

                        auto& group = m_groups[groupId];
                        group.m_name = groupId;
                        AzFramework::StringFunc::Join(group.m_displayName, groupDisplayNameVector.begin(), groupDisplayNameVector.end(), " | ");
                        group.m_description = !propertyGroupDefinition->GetDescription().empty() ? propertyGroupDefinition->GetDescription() : group.m_displayName;
                        
                        group.m_properties.reserve(propertyGroupDefinition->GetProperties().size());
                        for (const auto& propertyDefinition : propertyGroupDefinition->GetProperties())
                        {
                            AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                            
                            // Assign id before conversion so it can be used in dynamic description
                            propertyConfig.m_id = propertyDefinition->GetName();
                            groupNameContext.ContextualizeProperty(propertyConfig.m_id);
                            
                            AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, *propertyDefinition);
                            propertyConfig.m_description +=
                                "\n\n<img src=\':/Icons/changed_property.svg\'> An indicator icon will be shown to the left of properties "
                                "with overridden values that are different from the assigned material.\n";

                            const auto& propertyIndex = 
                                m_editData.m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);

                            // (Does DynamicPropertyConfig really even need m_groupName? It doesn't seem to be used anywhere)
                            propertyConfig.m_groupName = group.m_name;
                            propertyConfig.m_groupDisplayName = group.m_displayName;
                            propertyConfig.m_showThumbnail = true;
                            
                            propertyConfig.m_defaultValue = AtomToolsFramework::ConvertToEditableType(
                                m_editData.m_materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()]);
                            
                            // There is no explicit parent material here. Material instance property overrides replace the values from the
                            // assigned material asset. Its values should be treated as parent, for comparison, in this case.
                            propertyConfig.m_parentValue = AtomToolsFramework::ConvertToEditableType(
                                m_editData.m_materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()]);
                            propertyConfig.m_originalValue = AtomToolsFramework::ConvertToEditableType(
                                m_editData.m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                            group.m_properties.emplace_back(propertyConfig);
                        }
                        
                        // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
                        auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                            &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(group.m_name), {},
                            [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);
                        AddGroup(group.m_name, group.m_displayName, group.m_description, propertyGroupWidget);

                        return true;
                    });
            }

            void MaterialPropertyInspector::Populate()
            {
                AddGroupsBegin();
                AddUvNamesGroup();
                AddPropertiesGroup();
                AddGroupsEnd();
            }

            void MaterialPropertyInspector::LoadOverridesFromEntity()
            {
                if (!IsLoaded())
                {
                    return;
                }

                m_editData.m_materialPropertyOverrideMap.clear();
                MaterialComponentRequestBus::EventResult(
                    m_editData.m_materialPropertyOverrideMap, m_primaryEntityId, &MaterialComponentRequestBus::Events::GetPropertyValues,
                    m_materialAssignmentId);

                // Apply any automatic property renames so that the material inspector will be properly initialized with the right values
                // for properties that have new names.
                {
                    AZStd::vector<AZStd::pair<Name, Name>> renamedProperties;
                    for (auto& propertyOverridePair : m_editData.m_materialPropertyOverrideMap)
                    {
                        Name name = propertyOverridePair.first;
                        if (m_materialInstance->GetAsset()->GetMaterialTypeAsset()->ApplyPropertyRenames(name))
                        {
                            renamedProperties.emplace_back(propertyOverridePair.first, name);
                        }
                    }
                    for (const auto& [oldName, newName] : renamedProperties)
                    {
                        m_editData.m_materialPropertyOverrideMap[newName] = m_editData.m_materialPropertyOverrideMap[oldName];
                        m_editData.m_materialPropertyOverrideMap.erase(oldName);
                    }
                }

                for (auto& group : m_groups)
                {
                    for (auto& property : group.second.m_properties)
                    {
                        const AtomToolsFramework::DynamicPropertyConfig& propertyConfig = property.GetConfig();
                        const auto overrideItr = m_editData.m_materialPropertyOverrideMap.find(propertyConfig.m_id);
                        const auto& editValue = overrideItr != m_editData.m_materialPropertyOverrideMap.end() ? overrideItr->second : propertyConfig.m_originalValue;

                        // This first converts to an acceptable runtime type in case the value came from script
                        const auto propertyIndex = m_materialInstance->FindPropertyIndex(property.GetId());
                        if (propertyIndex.IsValid())
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
                RunEditorMaterialFunctors();
                RebuildAll();
                UpdateHeading();
            }

            void MaterialPropertyInspector::SaveOverrideToEntities(const AtomToolsFramework::DynamicProperty& property, bool commitChanges)
            {
                if (!IsLoaded())
                {
                    return;
                }

                // Apply the incoming property override to all pinned entities
                for (const AZ::EntityId& entityId : m_entityIdsToEdit)
                {
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::SetPropertyValue, m_materialAssignmentId,
                        property.GetId().GetStringView(), property.GetValue());
                }

                if (commitChanges)
                {
                    // If editing is complete and these changes are being committed we must mark all of the entities dirty for undo redo
                    AzToolsFramework::ScopedUndoBatch undoBatch("Material slot changed.");

                    m_internalEditNotification = true;
                    for (const AZ::EntityId& entityId : m_entityIdsToEdit)
                    {
                        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);

                        MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                    }
                    m_internalEditNotification = false;
                }

                // m_updatePreview should be set to true here for continuous preview updates as slider/color properties change but needs
                // throttling
            }
            
            bool MaterialPropertyInspector::AddEditorMaterialFunctors(
                const AZStd::vector<AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder>>& functorSourceDataHolders,
                const AZ::RPI::MaterialNameContext& nameContext)
            {
                // Copied from MaterialDocument::AddEditorMaterialFunctors, should be refactored at some point

                const AZ::RPI::MaterialFunctorSourceData::EditorContext editorContext = AZ::RPI::MaterialFunctorSourceData::EditorContext(
                    m_editData.m_materialTypeSourcePath, m_editData.m_materialAsset->GetMaterialPropertiesLayout(), &nameContext);

                for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder> functorData : functorSourceDataHolders)
                {
                    AZ::RPI::MaterialFunctorSourceData::FunctorResult result = functorData->CreateFunctor(editorContext);

                    if (result.IsSuccess())
                    {
                        AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor = result.GetValue();
                        if (functor != nullptr)
                        {
                            m_editorFunctors.push_back(functor);
                        }
                    }
                    else
                    {
                        AZ_Error("MaterialDocument", false, "Material functors were not created: '%s'.", m_editData.m_materialTypeSourcePath.c_str());
                        return false;
                    }
                }

                return true;
            }

            void MaterialPropertyInspector::RunEditorMaterialFunctors()
            {
                if (!IsLoaded())
                {
                    return;
                }

                AZStd::unordered_set<AZ::Name> changedPropertyNames;
                AZStd::unordered_set<AZ::Name> changedPropertyGroupNames;

                // Convert editor property configuration data into material property meta data so that it can be used to execute functors
                AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyDynamicMetadata> propertyDynamicMetadata;
                AZStd::unordered_map<AZ::Name, AZ::RPI::MaterialPropertyGroupDynamicMetadata> propertyGroupDynamicMetadata;
                for (auto& groupPair : m_groups)
                {
                    AZ::RPI::MaterialPropertyGroupDynamicMetadata& metadata = propertyGroupDynamicMetadata[AZ::Name{groupPair.first}];
                    
                    for (auto& property : groupPair.second.m_properties)
                    {
                        AtomToolsFramework::ConvertToPropertyMetaData(propertyDynamicMetadata[property.GetId()], property.GetConfig());
                    }

                    // It's significant that we check IsGroupHidden rather than IsGroupVisisble, because it follows the same rules as QWidget::isHidden().
                    // We don't care whether the widget and all its parents are visible, we only care about whether the group was hidden within the context
                    // of the Material Instance Editor.
                    metadata.m_visibility = IsGroupHidden(groupPair.first) ?
                        AZ::RPI::MaterialPropertyGroupVisibility::Hidden : AZ::RPI::MaterialPropertyGroupVisibility::Enabled;
                }

                for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctor>& functor : m_editorFunctors)
                {
                    const AZ::RPI::MaterialPropertyFlags& materialPropertyDependencies = functor->GetMaterialPropertyDependencies();
                    // None also covers case that the client code doesn't register material properties to dependencies,
                    // which will later get caught in Process() when trying to access a property.
                    if (materialPropertyDependencies.none() || functor->NeedsProcess(m_dirtyPropertyFlags))
                    {
                        AZ::RPI::MaterialFunctorAPI::EditorContext context = AZ::RPI::MaterialFunctorAPI::EditorContext(
                            m_materialInstance->GetPropertyCollection(),
                            propertyDynamicMetadata,
                            propertyGroupDynamicMetadata,
                            changedPropertyNames,
                            changedPropertyGroupNames,
                            &materialPropertyDependencies
                        );
                        functor->Process(context);
                    }
                }
                m_dirtyPropertyFlags.reset();

                // Apply any changes to material property meta data back to the editor property configurations
                for (auto& groupPair : m_groups)
                {
                    AZ::Name groupName{ groupPair.first };

                    if (changedPropertyGroupNames.find(groupName) != changedPropertyGroupNames.end())
                    {
                        SetGroupVisible(
                            groupPair.first,
                            propertyGroupDynamicMetadata[groupName].m_visibility == AZ::RPI::MaterialPropertyGroupVisibility::Enabled);
                    }

                    for (auto& property : groupPair.second.m_properties)
                    {
                        AtomToolsFramework::DynamicPropertyConfig propertyConfig = property.GetConfig();

                        const auto oldVisible = propertyConfig.m_visible;
                        const auto oldReadOnly = propertyConfig.m_readOnly;
                        AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDynamicMetadata[property.GetId()]);
                        property.SetConfig(propertyConfig);

                        if (oldReadOnly != propertyConfig.m_readOnly)
                        {
                            RefreshGroup(groupPair.first);
                        }

                        if (oldVisible != propertyConfig.m_visible)
                        {
                            RebuildGroup(groupPair.first);
                        }
                    }
                }
            }

            void MaterialPropertyInspector::UpdateMaterialInstanceProperty(const AtomToolsFramework::DynamicProperty& property)
            {
                if (!IsLoaded())
                {
                    return;
                }

                const auto propertyIndex = m_materialInstance->FindPropertyIndex(property.GetId());
                if (propertyIndex.IsValid())
                {
                    m_dirtyPropertyFlags.set(propertyIndex.GetIndex());

                    const auto runtimeValue = AtomToolsFramework::ConvertToRuntimeType(property.GetValue());
                    if (runtimeValue.IsValid())
                    {
                        m_materialInstance->SetPropertyValue(propertyIndex, runtimeValue);
                    }
                }
            }

            AZ::Crc32 MaterialPropertyInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
            {
                return AZ::Crc32(AZStd::string::format(
                    "MaterialPropertyInspector::PropertyGroup::%s::%s", m_editData.m_materialAssetId.ToString<AZStd::string>().c_str(),
                    groupName.c_str()));
            }

            bool MaterialPropertyInspector::IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const
            {
                const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(node);
                return property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_originalValue);
            }

            const char* MaterialPropertyInspector::GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const
            {
                if (IsInstanceNodePropertyModifed(node))
                {
                    return ":/Icons/changed_property.svg";
                }
                return ":/Icons/blank.png";
            }

            AZStd::string MaterialPropertyInspector::GetRelativePath(const AZStd::string& path) const
            {
                bool pathFound = false;
                AZStd::string rootFolder;
                AZStd::string relativePath;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    pathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath, path, relativePath,
                    rootFolder);
                return relativePath;
            }

            AZStd::string MaterialPropertyInspector::GetFileName(const AZStd::string& path) const
            {
                AZStd::string fileName;
                AZ::StringFunc::Path::GetFullFileName(path.c_str(), fileName);
                return fileName;
            }

            bool MaterialPropertyInspector::IsSourceMaterial(const AZStd::string& path) const
            {
                return !path.empty() && AZ::StringFunc::Path::IsExtension(path.c_str(), AZ::RPI::MaterialSourceData::Extension);
            }

            bool MaterialPropertyInspector::SaveMaterial(const AZStd::string& path) const
            {
                const AZStd::string saveFilePath = AtomToolsFramework::GetSaveFilePathFromDialog(
                    path, { { "Material", AZ::RPI::MaterialSourceData::Extension } }, "Material");
                if (saveFilePath.empty())
                {
                    return false;
                }

                if (!EditorMaterialComponentUtil::SaveSourceMaterialFromEditData(saveFilePath, m_editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentInspector", false, "Failed to save material data.");
                    return false;
                }

                return true;
            }

            void MaterialPropertyInspector::OpenMenu()
            {
                if (!IsLoaded())
                {
                    return;
                }

                QMenu menu(this);

                menu.addAction(tr("Save As..."), [this] {
                    const auto& defaultPath = AtomToolsFramework::GetUniqueFilePath(AZStd::string::format(
                        "%s/Assets/untitled.%s", AZ::Utils::GetProjectPath().c_str(), AZ::RPI::MaterialSourceData::Extension));
                    SaveMaterial(defaultPath);
                });

                if (IsSourceMaterial(m_editData.m_materialSourcePath))
                {
                    const auto& materialSourceFileName = GetFileName(m_editData.m_materialSourcePath);
                    menu.addAction(tr("Save Over \"%1\"...").arg(materialSourceFileName.c_str()), [this] {
                        SaveMaterial(m_editData.m_materialSourcePath);
                    });
                }

                menu.addSeparator();

                menu.addAction("Clear Overrides", [this] {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Clear material property overrides.");
                    m_editData.m_materialPropertyOverrideMap.clear();
                    for (const AZ::EntityId& entityId : m_entityIdsToEdit)
                    {
                        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                        MaterialComponentRequestBus::Event(
                            entityId, &MaterialComponentRequestBus::Events::SetPropertyValues, m_materialAssignmentId,
                            m_editData.m_materialPropertyOverrideMap);
                        MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                    }
                    m_updateUI = true;
                    m_updatePreview = true;
                });

                menu.exec(QCursor::pos());
            }

            const EditorMaterialComponentUtil::MaterialEditData& MaterialPropertyInspector::GetEditData() const
            {
                return m_editData;
            }

            AZ::Data::AssetId MaterialPropertyInspector::GetActiveMaterialAssetIdFromEntity() const
            {
                AZ::Data::AssetId materialAssetId = {};
                MaterialComponentRequestBus::EventResult(
                    materialAssetId, m_primaryEntityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId, m_materialAssignmentId);
                return materialAssetId;
            }

            void MaterialPropertyInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(pNode);
                if (property)
                {
                    m_editData.m_materialPropertyOverrideMap[property->GetId()] = property->GetValue();
                    UpdateMaterialInstanceProperty(*property);
                    SaveOverrideToEntities(*property, false);
                }
            }

            void MaterialPropertyInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
            {
                // As above, there are symmetrical functions on the notification interface for when editing begins and ends and has been
                // completed but they are not being called following that pattern. when this function executes the changes to the property
                // are ready to be committed or reverted
                const auto property = AtomToolsFramework::FindAncestorInstanceDataNodeByType<AtomToolsFramework::DynamicProperty>(pNode);
                if (property)
                {
                    m_editData.m_materialPropertyOverrideMap[property->GetId()] = property->GetValue();
                    UpdateMaterialInstanceProperty(*property);
                    SaveOverrideToEntities(*property, true);
                    RunEditorMaterialFunctors();
                }
            }

            void MaterialPropertyInspector::OnEntityInitialized(const AZ::EntityId& entityId)
            {
                if (m_entityIdsToEdit.count(entityId) > 0)
                {
                    UnloadMaterial();
                }
            }

            void MaterialPropertyInspector::OnEntityDestroyed(const AZ::EntityId& entityId)
            {
                if (m_entityIdsToEdit.count(entityId) > 0)
                {
                    UnloadMaterial();
                }
            }

            void MaterialPropertyInspector::OnEntityActivated(const AZ::EntityId& entityId)
            {
                m_updateUI |= (m_entityIdsToEdit.count(entityId) > 0);
            }

            void MaterialPropertyInspector::OnEntityDeactivated(const AZ::EntityId& entityId)
            {
                if (m_entityIdsToEdit.count(entityId) > 0)
                {
                    UnloadMaterial();
                }
            }

            void MaterialPropertyInspector::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
            {
                AZ_UNUSED(name);
                m_updateUI |= (m_primaryEntityId == entityId);
            }

            void MaterialPropertyInspector::OnSystemTick()
            {
                if (m_updateUI)
                {
                    m_updateUI = false;
                    UpdateUI();
                }

                if (m_updatePreview)
                {
                    m_updatePreview = false;
                    for (const AZ::EntityId& entityId : m_entityIdsToEdit)
                    {
                        EditorMaterialSystemComponentRequestBus::Broadcast(
                            &EditorMaterialSystemComponentRequestBus::Events::RenderMaterialPreview, entityId, m_materialAssignmentId);
                    }
                }
            }

            void MaterialPropertyInspector::OnMaterialsEdited()
            {
                m_updateUI |= !m_internalEditNotification;
                m_updatePreview = true;
            }

            void MaterialPropertyInspector::OnRenderMaterialPreviewReady(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap)
            {
                if (m_overviewImage && m_primaryEntityId == entityId && m_materialAssignmentId == materialAssignmentId)
                {
                    m_overviewImage->setPixmap(pixmap);
                }
            }

            void MaterialPropertyInspector::UpdateUI()
            {
                if (IsLoaded())
                {
                    LoadOverridesFromEntity();
                }
                else
                {
                    LoadMaterial(m_primaryEntityId, m_entityIdsToEdit, m_materialAssignmentId);
                }
            }
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ
