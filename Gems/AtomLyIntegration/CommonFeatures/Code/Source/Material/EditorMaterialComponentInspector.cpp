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
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailContext.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentInspector
        {
            MaterialPropertyInspector::MaterialPropertyInspector(
                const AZStd::string& slotName, const AZ::Data::AssetId& assetId, PropertyChangedCallback propertyChangedCallback,
                QWidget* parent)
                : AtomToolsFramework::InspectorWidget(parent)
                , m_slotName(slotName)
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
                if (!EditorMaterialComponentUtil::LoadMaterialEditDataFromAssetId(m_materialAssetId, m_editData))
                {
                    AZ_Warning("AZ::Render::EditorMaterialComponentInspector", false, "Failed to load material data.");
                    return false;
                }

                // The material instance is still needed for functor execution
                m_materialInstance = AZ::RPI::Material::Create(m_editData.m_materialAsset);
                if (!m_materialInstance)
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentInspector", false, "Material instance could not be created.");
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

                auto propertyGroupContainer = new QWidget(this);
                propertyGroupContainer->setLayout(new QHBoxLayout());

                AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey =
                    MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, m_editData.m_materialAssetId);
                auto thumbnailWidget = new AzToolsFramework::Thumbnailer::ThumbnailWidget(this);
                thumbnailWidget->setFixedSize(QSize(120, 120));
                thumbnailWidget->setVisible(true);
                thumbnailWidget->SetThumbnailKey(thumbnailKey, AzToolsFramework::Thumbnailer::ThumbnailContext::DefaultContext);
                propertyGroupContainer->layout()->addWidget(thumbnailWidget);

                auto materialInfoWidget = new QLabel(this);
                QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Preferred);
                sizePolicy1.setHorizontalStretch(0);
                sizePolicy1.setVerticalStretch(0);
                sizePolicy1.setHeightForWidth(materialInfoWidget->sizePolicy().hasHeightForWidth());
                materialInfoWidget->setSizePolicy(sizePolicy1);
                materialInfoWidget->setMinimumSize(QSize(0, 0));
                materialInfoWidget->setMaximumSize(QSize(16777215, 16777215));
                materialInfoWidget->setTextFormat(Qt::AutoText);
                materialInfoWidget->setScaledContents(false);
                materialInfoWidget->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);
                materialInfoWidget->setWordWrap(true);

                QFileInfo materialFileInfo(AZ::RPI::AssetUtils::GetProductPathByAssetId(m_editData.m_materialAsset.GetId()).c_str());
                QFileInfo materialSourceFileInfo(m_editData.m_materialSourcePath.c_str());
                QFileInfo materialTypeSourceFileInfo(m_editData.m_materialTypeSourcePath.c_str());
                QFileInfo materialParentSourceFileInfo(AZ::RPI::AssetUtils::GetSourcePathByAssetId(m_editData.m_materialParentAsset.GetId()).c_str());

                QString materialInfo;
                materialInfo += tr("<table>");
                materialInfo += tr("<tr><td><b>Material Slot&emsp;</b></td><td>%1</td></tr>").arg(m_slotName.c_str());
                if (!materialFileInfo.fileName().isEmpty())
                {
                    materialInfo += tr("<tr><td><b>Material&emsp;</b></td><td>%1</td></tr>").arg(materialFileInfo.fileName());
                }
                if (!materialTypeSourceFileInfo.fileName().isEmpty())
                {
                    materialInfo += tr("<tr><td><b>Material Type&emsp;</b></td><td>%1</td></tr>").arg(materialTypeSourceFileInfo.fileName());
                }
                if (!materialSourceFileInfo.fileName().isEmpty())
                {
                    materialInfo += tr("<tr><td><b>Material Source&emsp;</b></td><td>%1</td></tr>").arg(materialSourceFileInfo.fileName());
                }
                if (!materialParentSourceFileInfo.fileName().isEmpty())
                {
                    materialInfo += tr("<tr><td><b>Material Parent&emsp;</b></td><td>%1</td></tr>").arg(materialParentSourceFileInfo.fileName());
                }
                materialInfo += tr("</table>");
                materialInfoWidget->setText(materialInfo);

                propertyGroupContainer->layout()->addWidget(materialInfoWidget);

                AddGroup(groupNameId, groupDisplayName, groupDescription, propertyGroupContainer);
            }

            void MaterialPropertyInspector::AddUvNamesGroup()
            {
                const AZStd::string groupNameId = AZ::RPI::UvGroupName;
                const AZStd::string groupDisplayName = "UV Sets";
                const AZStd::string groupDescription = "UV set names in this material, which can be renamed to match those in the model.";
                auto& group = m_groups[groupNameId];

                const RPI::MaterialUvNameMap& uvNameMap = m_editData.m_materialAsset->GetMaterialTypeAsset()->GetUvNameMap();
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
                    propertyConfig.m_groupName = groupDisplayName;
                    propertyConfig.m_description = shaderInputStr;
                    propertyConfig.m_defaultValue = uvName;
                    propertyConfig.m_originalValue = uvName;
                    propertyConfig.m_parentValue = uvName;
                    propertyConfig.m_readOnly = true;
                    group.m_properties.emplace_back(propertyConfig);
                }

                // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
                const AZ::Crc32 saveStateKey(AZStd::string::format(
                    "MaterialPropertyInspector::PropertyGroup::%s::%s", m_materialAssetId.ToString<AZStd::string>().c_str(),
                    groupNameId.c_str()));
                auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                    &group, &group, group.TYPEINFO_Uuid(), this, this, saveStateKey,
                    [this](const AzToolsFramework::InstanceDataNode* source, const AzToolsFramework::InstanceDataNode* target) {
                        AZ_UNUSED(source);
                        const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(target);
                        return property && AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue);
                    });
                AddGroup(groupNameId, groupDisplayName, groupDescription, propertyGroupWidget);
            }

            void MaterialPropertyInspector::Populate()
            {
                AddGroupsBegin();

                AddDetailsGroup();
                AddUvNamesGroup();

                // Copy all of the properties from the material asset to the source data that will be exported
                for (const auto& groupDefinition : m_editData.m_materialTypeSourceData.GetGroupDefinitionsInDisplayOrder())
                {
                    const AZStd::string& groupNameId = groupDefinition.m_nameId;
                    const AZStd::string& groupDisplayName = !groupDefinition.m_displayName.empty() ? groupDefinition.m_displayName : groupNameId;
                    const AZStd::string& groupDescription = !groupDefinition.m_description.empty() ? groupDefinition.m_description : groupDisplayName;
                    auto& group = m_groups[groupNameId];

                    const auto& propertyLayout = m_editData.m_materialTypeSourceData.m_propertyLayout;
                    const auto& propertyListItr = propertyLayout.m_properties.find(groupNameId);
                    if (propertyListItr != propertyLayout.m_properties.end())
                    {
                        group.m_properties.reserve(propertyListItr->second.size());
                        for (const auto& propertyDefinition : propertyListItr->second)
                        {
                            AtomToolsFramework::DynamicPropertyConfig propertyConfig;

                            // Assign id before conversion so it can be used in dynamic description
                            propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupNameId, propertyDefinition.m_nameId).GetFullName();

                            AtomToolsFramework::ConvertToPropertyConfig(propertyConfig, propertyDefinition);

                            propertyConfig.m_groupName = groupDisplayName;
                            const auto& propertyIndex = m_editData.m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(propertyConfig.m_id);
                            propertyConfig.m_showThumbnail = true;
                            propertyConfig.m_defaultValue = AtomToolsFramework::ConvertToEditableType(m_editData.m_materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()]);
                            propertyConfig.m_parentValue = AtomToolsFramework::ConvertToEditableType(m_editData.m_materialTypeAsset->GetDefaultPropertyValues()[propertyIndex.GetIndex()]);
                            propertyConfig.m_originalValue = AtomToolsFramework::ConvertToEditableType(m_editData.m_materialAsset->GetPropertyValues()[propertyIndex.GetIndex()]);
                            group.m_properties.emplace_back(propertyConfig);
                        }
                    }

                    // Passing in same group as main and comparison instance to enable custom value comparison for highlighting modified properties
                    const AZ::Crc32 saveStateKey(AZStd::string::format(
                        "MaterialPropertyInspector::PropertyGroup::%s::%s", m_materialAssetId.ToString<AZStd::string>().c_str(),
                        groupNameId.c_str()));
                    auto propertyGroupWidget = new AtomToolsFramework::InspectorPropertyGroupWidget(
                        &group, &group, group.TYPEINFO_Uuid(), this, this, saveStateKey,
                        [this](const AzToolsFramework::InstanceDataNode* source, const AzToolsFramework::InstanceDataNode* target) {
                            AZ_UNUSED(source);
                            const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(target);
                            return property && AtomToolsFramework::ArePropertyValuesEqual(property->GetValue(), property->GetConfig().m_parentValue);
                        });
                    AddGroup(groupNameId, groupDisplayName, groupDescription, propertyGroupWidget);
                }

                AddGroupsEnd();

                m_dirtyPropertyFlags.set();
                RunEditorMaterialFunctors();
            }

            void MaterialPropertyInspector::RunPropertyChangedCallback()
            {
                if (m_propertyChangedCallback)
                {
                    m_propertyChangedCallback(m_editData.m_materialPropertyOverrideMap);
                }
            }

            void MaterialPropertyInspector::RunEditorMaterialFunctors()
            {
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
                    AZ::Name groupName{groupPair.first};

                    if (changedPropertyGroupNames.find(groupName) != changedPropertyGroupNames.end())
                    {
                        SetGroupVisible(groupPair.first, propertyGroupDynamicMetadata[groupName].m_visibility == AZ::RPI::MaterialPropertyGroupVisibility::Enabled);
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
                m_editData.m_materialPropertyOverrideMap = propertyOverrideMap;

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
                RunPropertyChangedCallback();
                RunEditorMaterialFunctors();
                RebuildAll();
            }

            bool MaterialPropertyInspector::SaveMaterial() const
            {
                const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                    QString(AZ::IO::FileIOBase::GetInstance()->GetAlias("@devassets@")) +
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
                const QString saveFilePath = AtomToolsFramework::GetSaveFileInfo(m_editData.m_materialSourcePath.c_str()).absoluteFilePath();
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
                return !m_editData.m_materialSourcePath.empty() &&
                    AZ::StringFunc::Path::IsExtension(m_editData.m_materialSourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
            }

            bool MaterialPropertyInspector::HasMaterialParentSource() const
            {
                return !m_editData.m_materialParentSourcePath.empty() &&
                    AZ::StringFunc::Path::IsExtension(
                        m_editData.m_materialParentSourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
            }

            void MaterialPropertyInspector::OpenMaterialSourceInEditor() const
            {
                if (HasMaterialSource())
                {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::OpenInMaterialEditor, m_editData.m_materialSourcePath);
                }
            }

            void MaterialPropertyInspector::OpenMaterialParentSourceInEditor() const
            {
                if (HasMaterialParentSource())
                {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::OpenInMaterialEditor, m_editData.m_materialParentSourcePath);
                }
            }

            const EditorMaterialComponentUtil::MaterialEditData& MaterialPropertyInspector::GetEditData() const
            {
                return m_editData;
            }

            void MaterialPropertyInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                // For some reason the reflected property editor notifications are not symmetrical
                // This function is called continuously anytime a property changes until the edit has completed
                // Because of that, we have to track whether or not we are continuing to edit the same property to know when editing has started and ended
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
                        RunPropertyChangedCallback();
                    }
                }
            }

            void MaterialPropertyInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
            {
                // As above, there are symmetrical functions on the notification interface for when editing begins and ends and has been completed but they are not being called following that pattern.
                // when this function executes the changes to the property are ready to be committed or reverted
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
                if (property)
                {
                    if (m_activeProperty == property)
                    {
                        m_editData.m_materialPropertyOverrideMap[m_activeProperty->GetId()] = m_activeProperty->GetValue();
                        UpdateMaterialInstanceProperty(*m_activeProperty);
                        RunPropertyChangedCallback();
                        RunEditorMaterialFunctors();
                        m_activeProperty = nullptr;
                    }
                }
            }

            bool OpenInspectorDialog(
                const AZStd::string& slotName, const AZ::Data::AssetId& assetId, MaterialPropertyOverrideMap propertyOverrideMap,
                PropertyChangedCallback propertyChangedCallback)
            {
                QWidget* activeWindow = nullptr;
                AzToolsFramework::EditorWindowRequestBus::BroadcastResult(activeWindow, &AzToolsFramework::EditorWindowRequests::GetAppMainWindow);

                // Constructing a dialog with a table to display all configurable material export items
                QDialog dialog(activeWindow);
                dialog.setWindowTitle("Material Inspector");

                MaterialPropertyInspector* inspector = new MaterialPropertyInspector(slotName, assetId, propertyChangedCallback, &dialog);
                if (!inspector->LoadMaterial())
                {
                    return false;
                }

                inspector->Populate();
                inspector->SetOverrides(propertyOverrideMap);

                // Create the menu button
                QToolButton* menuButton = new QToolButton(&dialog);
                menuButton->setAutoRaise(true);
                menuButton->setIcon(QIcon(":/Cards/img/UI20/Cards/menu_ico.svg"));
                menuButton->setVisible(true);
                QObject::connect(menuButton, &QToolButton::clicked, &dialog, [&]() {
                    QAction* action = nullptr;

                    QMenu menu(&dialog);
                    action = menu.addAction("Clear Overrides", [&] { inspector->SetOverrides(MaterialPropertyOverrideMap()); });
                    action = menu.addAction("Revert Changes", [&] { inspector->SetOverrides(propertyOverrideMap); });

                    menu.addSeparator();
                    action = menu.addAction("Save Material", [&] { inspector->SaveMaterial(); });
                    action = menu.addAction("Save Material To Source", [&] { inspector->SaveMaterialToSource(); });
                    action->setEnabled(inspector->HasMaterialSource());

                    menu.addSeparator();
                    action = menu.addAction("Open Source Material In Editor", [&] { inspector->OpenMaterialSourceInEditor(); });
                    action->setEnabled(inspector->HasMaterialSource());
                    action = menu.addAction("Open Parent Material In Editor", [&] { inspector->OpenMaterialParentSourceInEditor(); });
                    action->setEnabled(inspector->HasMaterialParentSource());
                    menu.exec(QCursor::pos());
                });

                QDialogButtonBox* buttonBox = new QDialogButtonBox(&dialog);
                buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                QObject::connect(&dialog, &QDialog::rejected, &dialog, [&] { inspector->SetOverrides(propertyOverrideMap); });

                QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);
                dialogLayout->addWidget(menuButton);
                dialogLayout->addWidget(inspector);
                dialogLayout->addWidget(buttonBox);
                dialog.setLayout(dialogLayout);
                dialog.setModal(true);

                // Forcing the initial dialog size to accomodate typical content.
                // Temporarily settng fixed size because dialog.show/exec invokes WindowDecorationWrapper::showEvent.
                // This forces the dialog to be centered and sized based on the layout of content.
                // Resizing the dialog after show will not be centered and moving the dialog programatically doesn't m0ve the custmk frame. 
                dialog.setFixedSize(500, 800);
                dialog.show();

                // Removing fixed size to allow drag resizing
                dialog.setMinimumSize(0, 0);
                dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

                // Return true if the user press the export button
                return dialog.exec() == QDialog::Accepted;
            }
        } // namespace EditorMaterialComponentInspector
    } // namespace Render
} // namespace AZ

//#include <AtomLyIntegration/CommonFeatures/moc_EditorMaterialComponentInspector.cpp>
