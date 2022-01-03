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
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Components/Widgets/Text.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConfig.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
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
                AZ::TickBus::Handler::BusConnect();
                AZ::EntitySystemBus::Handler::BusConnect();
                EditorMaterialSystemComponentNotificationBus::Handler::BusConnect();
            }

            MaterialPropertyInspector::~MaterialPropertyInspector()
            {
                AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
                AZ::EntitySystemBus::Handler::BusDisconnect();
                EditorMaterialSystemComponentNotificationBus::Handler::BusDisconnect();
                MaterialComponentNotificationBus::Handler::BusDisconnect();
            }

            bool MaterialPropertyInspector::LoadMaterial(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId)
            {
                UnloadMaterial();

                m_entityId = entityId;
                m_materialAssignmentId = materialAssignmentId;
                MaterialComponentNotificationBus::Handler::BusDisconnect();
                MaterialComponentNotificationBus::Handler::BusConnect(m_entityId);

                AZ::Data::AssetId materialAssetId = {};
                MaterialComponentRequestBus::EventResult(
                    materialAssetId, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialOverride, m_materialAssignmentId);
                if (!materialAssetId.IsValid())
                {
                    MaterialComponentRequestBus::EventResult(
                        materialAssetId, m_entityId, &MaterialComponentRequestBus::Events::GetDefaultMaterialAssetId,
                        m_materialAssignmentId);
                }

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

                // Get a list of all the editor functors to be used for property editor states
                auto propertyLayout = m_editData.m_materialAsset->GetMaterialPropertiesLayout();
                const AZ::RPI::MaterialFunctorSourceData::EditorContext editorContext =
                    AZ::RPI::MaterialFunctorSourceData::EditorContext(m_editData.m_materialTypeSourcePath, propertyLayout);
                for (AZ::RPI::Ptr<AZ::RPI::MaterialFunctorSourceDataHolder> functorData : m_editData.m_materialTypeSourceData.m_materialFunctorSourceData)
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
                        AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Material functors were not created: '%s'.", m_editData.m_materialTypeSourcePath.c_str());
                    }
                }

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
                return m_entityId.IsValid() && m_materialInstance && m_editData.m_materialAsset.IsReady();
            }

            void MaterialPropertyInspector::Reset()
            {
                m_activeProperty = {};
                m_groups = {};
                m_dirtyPropertyFlags.set();
                m_internalEditNotification = {};

                AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
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
                    m_overviewText->setText(tr("Material not available"));
                    m_overviewText->setAlignment(Qt::AlignCenter);
                    m_overviewImage->setVisible(false);
                    return;
                }

                QFileInfo materialFileInfo(AZ::RPI::AssetUtils::GetProductPathByAssetId(m_editData.m_materialAsset.GetId()).c_str());
                QFileInfo materialSourceFileInfo(m_editData.m_materialSourcePath.c_str());
                QFileInfo materialTypeSourceFileInfo(m_editData.m_materialTypeSourcePath.c_str());
                QFileInfo materialParentSourceFileInfo(
                    AZ::RPI::AssetUtils::GetSourcePathByAssetId(m_editData.m_materialParentAsset.GetId()).c_str());

                AZStd::string entityName;
                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, m_entityId);

                AZStd::string slotName;
                MaterialComponentRequestBus::EventResult(
                    slotName, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialSlotLabel, m_materialAssignmentId);

                QString materialInfo;
                materialInfo += tr("<table>");
                materialInfo += tr("<tr><td><b>Entity&emsp;</b></td><td>%1</td></tr>").arg(entityName.c_str());
                materialInfo += tr("<tr><td><b>Material Slot&emsp;</b></td><td>%1</td></tr>").arg(slotName.c_str());
                if (!materialFileInfo.fileName().isEmpty())
                {
                    materialInfo += tr("<tr><td><b>Material&emsp;</b></td><td>%1</td></tr>").arg(materialFileInfo.fileName());
                }
                if (!materialTypeSourceFileInfo.fileName().isEmpty())
                {
                    materialInfo +=
                        tr("<tr><td><b>Material Type&emsp;</b></td><td>%1</td></tr>").arg(materialTypeSourceFileInfo.fileName());
                }
                if (!materialSourceFileInfo.fileName().isEmpty())
                {
                    materialInfo += tr("<tr><td><b>Material Source&emsp;</b></td><td>%1</td></tr>").arg(materialSourceFileInfo.fileName());
                }
                if (!materialParentSourceFileInfo.fileName().isEmpty())
                {
                    materialInfo +=
                        tr("<tr><td><b>Material Parent&emsp;</b></td><td>%1</td></tr>").arg(materialParentSourceFileInfo.fileName());
                }
                materialInfo += tr("</table>");

                m_overviewText->setText(materialInfo);
                m_overviewText->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);

                QPixmap pixmap;
                EditorMaterialSystemComponentRequestBus::BroadcastResult(
                    pixmap, &EditorMaterialSystemComponentRequestBus::Events::GetRenderedMaterialPreview, m_entityId,
                    m_materialAssignmentId);
                m_overviewImage->setPixmap(pixmap);
                m_overviewImage->setVisible(true);
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
                    propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::String;
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
                // Copy all of the properties from the material asset to the source data that will be exported
                for (const auto& groupDefinition : m_editData.m_materialTypeSourceData.GetGroupDefinitionsInDisplayOrder())
                {
                    const AZStd::string& groupName = groupDefinition.m_name;
                    const AZStd::string& groupDisplayName = !groupDefinition.m_displayName.empty() ? groupDefinition.m_displayName : groupName;
                    const AZStd::string& groupDescription = !groupDefinition.m_description.empty() ? groupDefinition.m_description : groupDisplayName;
                    auto& group = m_groups[groupName];

                    const auto& propertyLayout = m_editData.m_materialTypeSourceData.m_propertyLayout;
                    const auto& propertyListItr = propertyLayout.m_properties.find(groupName);
                    if (propertyListItr != propertyLayout.m_properties.end())
                    {
                        group.m_properties.reserve(propertyListItr->second.size());
                        for (const auto& propertyDefinition : propertyListItr->second)
                        {
                            AtomToolsFramework::DynamicPropertyConfig propertyConfig;

                            // Assign id before conversion so it can be used in dynamic description
                            propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupName, propertyDefinition.m_name).GetFullName();

                            AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDefinition);

                            const auto& propertyIndex =
                                m_editData.m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);

                            propertyConfig.m_groupName = groupDisplayName;
                            propertyConfig.m_showThumbnail = true;

                            propertyConfig.m_defaultValue = AtomToolsFramework::ConvertToEditableType(
                                m_editData.m_materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()]);

                            // There is no explicit parent material here. Material instance property overrides replace the values from the
                            // assigned material asset. Its values should be treated as parent, for comparison, in this case.
                            propertyConfig.m_parentValue = AtomToolsFramework::ConvertToEditableType(
                                m_editData.m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                            propertyConfig.m_originalValue = AtomToolsFramework::ConvertToEditableType(
                                m_editData.m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                            group.m_properties.emplace_back(propertyConfig);
                        }
                    }

                    // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
                    auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                        &group, &group, group.TYPEINFO_Uuid(), this, this, GetGroupSaveStateKey(groupName), {},
                        [this](const auto node) { return GetInstanceNodePropertyIndicator(node); }, 0);
                    AddGroup(groupName, groupDisplayName, groupDescription, propertyGroupWidget);
                }
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
                    m_editData.m_materialPropertyOverrideMap, m_entityId, &MaterialComponentRequestBus::Events::GetPropertyOverrides,
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
                RunEditorMaterialFunctors();
                RebuildAll();
                UpdateHeading();
            }

            void MaterialPropertyInspector::SaveOverridesToEntity(bool commitChanges)
            {
                if (!IsLoaded())
                {
                    return;
                }

                MaterialComponentRequestBus::Event(
                    m_entityId, &MaterialComponentRequestBus::Events::SetPropertyOverrides, m_materialAssignmentId,
                    m_editData.m_materialPropertyOverrideMap);

                if (commitChanges)
                {
                    AzToolsFramework::ScopedUndoBatch undoBatch("Material slot changed.");
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, m_entityId);

                    m_internalEditNotification = true;
                    MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                    m_internalEditNotification = false;
                }

                // m_updatePreview should be set to true here for continuous preview updates as slider/color properties change but needs
                // throttling
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
                    // of the material property inspector.
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
                        AZ::RPI::MaterialFunctor::EditorContext context = AZ::RPI::MaterialFunctor::EditorContext(
                            m_materialInstance->GetPropertyValues(),
                            m_materialInstance->GetMaterialPropertiesLayout(),
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

            AZ::Crc32 MaterialPropertyInspector::GetGroupSaveStateKey(const AZStd::string& groupName) const
            {
                return AZ::Crc32(AZStd::string::format(
                    "MaterialPropertyInspector::PropertyGroup::%s::%s", m_editData.m_materialAssetId.ToString<AZStd::string>().c_str(),
                    groupName.c_str()));
            }

            bool MaterialPropertyInspector::IsInstanceNodePropertyModifed(const AzToolsFramework::InstanceDataNode* node) const
            {
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(node);
                return property && !AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue);
            }

            const char* MaterialPropertyInspector::GetInstanceNodePropertyIndicator(const AzToolsFramework::InstanceDataNode* node) const
            {
                if (IsInstanceNodePropertyModifed(node))
                {
                    return ":/Icons/changed_property.svg";
                }
                return ":/Icons/blank.png";
            }

            bool MaterialPropertyInspector::SaveMaterial() const
            {
                const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                    QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@projectroot@")) +
                    AZ_CORRECT_FILESYSTEM_SEPARATOR + "Materials" +
                    AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                    AZ::RPI::MaterialSourceData::Extension).absoluteFilePath();

                const QString saveFilePath = AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath();
                if (saveFilePath.isEmpty())
                {
                    return false;
                }

                if (!EditorMaterialComponentUtil::SaveSourceMaterialFromEditData(saveFilePath.toUtf8().constData(), m_editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentInspector", false, "Failed to save material data.");
                    return false;
                }

                return true;
            }

            bool MaterialPropertyInspector::SaveMaterialToSource() const
            {
                const QString saveFilePath =
                    AtomToolsFramework::GetSaveFileInfo(m_editData.m_materialSourcePath.c_str()).absoluteFilePath();
                if (saveFilePath.isEmpty())
                {
                    return false;
                }

                if (!EditorMaterialComponentUtil::SaveSourceMaterialFromEditData(saveFilePath.toUtf8().constData(), m_editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentInspector", false, "Failed to save material data.");
                    return false;
                }

                return true;
            }

            bool MaterialPropertyInspector::HasMaterialSource() const
            {
                return IsLoaded() && !m_editData.m_materialSourcePath.empty() &&
                    AZ::StringFunc::Path::IsExtension(m_editData.m_materialSourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
            }

            bool MaterialPropertyInspector::HasMaterialParentSource() const
            {
                return IsLoaded() && !m_editData.m_materialParentSourcePath.empty() &&
                    AZ::StringFunc::Path::IsExtension(
                        m_editData.m_materialParentSourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
            }

            void MaterialPropertyInspector::OpenMaterialSourceInEditor() const
            {
                if (HasMaterialSource())
                {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor, m_editData.m_materialSourcePath);
                }
            }

            void MaterialPropertyInspector::OpenMaterialParentSourceInEditor() const
            {
                if (HasMaterialParentSource())
                {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor, m_editData.m_materialParentSourcePath);
                }
            }

            void MaterialPropertyInspector::OpenMenu()
            {
                QAction* action = nullptr;

                QMenu menu(this);
                action = menu.addAction("Clear Overrides", [this] {
                    MaterialComponentRequestBus::Event(
                        m_entityId, &MaterialComponentRequestBus::Events::SetPropertyOverrides, m_materialAssignmentId,
                        MaterialPropertyOverrideMap());
                    m_updateUI = true;
                    m_updatePreview = true;
                });
                action->setEnabled(IsLoaded());

                menu.addSeparator();

                action = menu.addAction("Save Material", [this] { SaveMaterial(); });
                action->setEnabled(IsLoaded());

                action = menu.addAction("Save Material To Source", [this] { SaveMaterialToSource(); });
                action->setEnabled(HasMaterialSource());

                menu.addSeparator();

                action = menu.addAction("Open Source Material In Editor", [this] { OpenMaterialSourceInEditor(); });
                action->setEnabled(HasMaterialSource());

                action = menu.addAction("Open Parent Material In Editor", [this] { OpenMaterialParentSourceInEditor(); });
                action->setEnabled(HasMaterialParentSource());

                menu.exec(QCursor::pos());
            }

            const EditorMaterialComponentUtil::MaterialEditData& MaterialPropertyInspector::GetEditData() const
            {
                return m_editData;
            }

            void MaterialPropertyInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                // For some reason the reflected property editor notifications are not symmetrical
                // This function is called continuously anytime a property changes until the edit has completed
                // Because of that, we have to track whether or not we are continuing to edit the same property to know when editing has
                // started and ended
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
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
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
                if (property)
                {
                    if (m_activeProperty == property)
                    {
                        m_editData.m_materialPropertyOverrideMap[m_activeProperty->GetId()] = m_activeProperty->GetValue();
                        UpdateMaterialInstanceProperty(*m_activeProperty);
                        SaveOverridesToEntity(false);
                    }
                }
            }

            void MaterialPropertyInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
            {
                // As above, there are symmetrical functions on the notification interface for when editing begins and ends and has been
                // completed but they are not being called following that pattern. when this function executes the changes to the property
                // are ready to be committed or reverted
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
                if (property)
                {
                    if (m_activeProperty == property)
                    {
                        m_editData.m_materialPropertyOverrideMap[m_activeProperty->GetId()] = m_activeProperty->GetValue();
                        UpdateMaterialInstanceProperty(*m_activeProperty);
                        SaveOverridesToEntity(true);
                        RunEditorMaterialFunctors();
                        m_activeProperty = nullptr;
                    }
                }
            }

            void MaterialPropertyInspector::OnEntityInitialized(const AZ::EntityId& entityId)
            {
                if (m_entityId == entityId)
                {
                    UnloadMaterial();
                }
            }

            void MaterialPropertyInspector::OnEntityDestroyed(const AZ::EntityId& entityId)
            {
                if (m_entityId == entityId)
                {
                    UnloadMaterial();
                }
            }

            void MaterialPropertyInspector::OnEntityActivated(const AZ::EntityId& entityId)
            {
                m_updateUI |= (m_entityId == entityId);
            }

            void MaterialPropertyInspector::OnEntityDeactivated(const AZ::EntityId& entityId)
            {
                if (m_entityId == entityId)
                {
                    UnloadMaterial();
                }
            }

            void MaterialPropertyInspector::OnEntityNameChanged(const AZ::EntityId& entityId, const AZStd::string& name)
            {
                AZ_UNUSED(name);
                m_updateUI |= (m_entityId == entityId);
            }

            void MaterialPropertyInspector::OnTick(float deltaTime, ScriptTimePoint time)
            {
                AZ_UNUSED(time);
                AZ_UNUSED(deltaTime);
                if (m_updateUI)
                {
                    m_updateUI = false;
                    UpdateUI();
                }

                if (m_updatePreview)
                {
                    m_updatePreview = false;
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::RenderMaterialPreview, m_entityId, m_materialAssignmentId);
                }
            }

            void MaterialPropertyInspector::OnMaterialsEdited()
            {
                m_updateUI |= !m_internalEditNotification;
                m_updatePreview = true;
            }

            void MaterialPropertyInspector::OnRenderMaterialPreviewComplete(
                const AZ::EntityId& entityId, const AZ::Render::MaterialAssignmentId& materialAssignmentId, const QPixmap& pixmap)
            {
                if (m_overviewImage && m_entityId == entityId && m_materialAssignmentId == materialAssignmentId)
                {
                    m_overviewImage->setPixmap(pixmap);
                }
            }

            void MaterialPropertyInspector::UpdateUI()
            {
                AZ::Data::AssetId materialAssetId = {};
                MaterialComponentRequestBus::EventResult(
                    materialAssetId, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialOverride, m_materialAssignmentId);
                if (!materialAssetId.IsValid())
                {
                    MaterialComponentRequestBus::EventResult(
                        materialAssetId, m_entityId, &MaterialComponentRequestBus::Events::GetDefaultMaterialAssetId, m_materialAssignmentId);
                }

                if (IsLoaded() && m_editData.m_materialAssetId == materialAssetId)
                {
                    LoadOverridesFromEntity();
                }
                else
                {
                    LoadMaterial(m_entityId, m_materialAssignmentId);
                }
            }
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ
