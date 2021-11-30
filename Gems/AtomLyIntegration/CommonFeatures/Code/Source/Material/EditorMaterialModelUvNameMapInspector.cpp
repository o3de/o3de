/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/EditorMaterialModelUvNameMapInspector.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>

#include <AtomToolsFramework/Inspector/InspectorPropertyGroupWidget.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>

#include <AzToolsFramework/API/EditorWindowRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        namespace EditorMaterialComponentInspector
        {
            MaterialModelUvNameMapInspector::MaterialModelUvNameMapInspector(
                const AZ::Data::AssetId& assetId,
                const RPI::MaterialModelUvOverrideMap& matModUvOverrides,
                const AZStd::unordered_set<AZ::Name>& modelUvNames,
                MaterialModelUvOverrideMapChangedCallBack matModUvOverrideMapChangedCallBack,
                QWidget* parent
            )
                : AtomToolsFramework::InspectorWidget(parent)
                , m_matModUvOverrideMapChangedCallBack(matModUvOverrideMapChangedCallBack)
                , m_matModUvOverrides(matModUvOverrides)
            {
                // Load the originating product asset from which the new source has set will be generated
                auto materialAssetOutcome = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(assetId);
                AZ_Error("AZ::Render::EditorMaterialComponentInspector", materialAssetOutcome, "Failed to load material asset: %s", assetId.ToString<AZStd::string>().c_str());

                auto materialAsset = materialAssetOutcome.GetValue();

                // Get material UV names
                m_materialUvNames = materialAsset->GetMaterialTypeAsset()->GetUvNameMap();

                SetModelUvNames(modelUvNames);
                ResetModelUvNameIndices();
            }

            MaterialModelUvNameMapInspector::~MaterialModelUvNameMapInspector()
            {
                AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
            }

            void MaterialModelUvNameMapInspector::Reset()
            {
                AtomToolsFramework::InspectorRequestBus::Handler::BusDisconnect();
                AtomToolsFramework::InspectorWidget::Reset();
            }

            void MaterialModelUvNameMapInspector::Populate()
            {
                AddGroupsBegin();

                const AZStd::string groupName = "ModelUvMap";
                const AZStd::string groupDisplayName = "Material to Model UV Map";
                const AZStd::string groupDescription = "Custom map that maps a UV name from the material to one from the model.";

                const size_t uvSize = m_materialUvNames.size();

                m_group.m_properties.reserve(uvSize);

                for (size_t i = 0; i < uvSize; ++i)
                {
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                    const AZStd::string shaderInput = m_materialUvNames[i].m_shaderInput.ToString();
                    const AZStd::string materialUvName = m_materialUvNames[i].m_uvName.GetStringView();

                    propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Enum;
                    propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupName, shaderInput).GetFullName();
                    propertyConfig.m_name = shaderInput;
                    propertyConfig.m_displayName = materialUvName;
                    propertyConfig.m_description = shaderInput;
                    propertyConfig.m_defaultValue = 0u;
                    propertyConfig.m_originalValue = 0u;
                    propertyConfig.m_parentValue = 0u;
                    propertyConfig.m_enumValues = m_modelUvNames;
                    m_group.m_properties.emplace_back(propertyConfig);
                    m_group.m_properties.back().SetValue(AZStd::any(m_modelUvNameIndices[i]));
                }

                AddGroup(groupName, groupDisplayName, groupDescription,
                    new AtomToolsFramework::InspectorPropertyGroupWidget(&m_group, nullptr, m_group.TYPEINFO_Uuid(), this));

                AddGroupsEnd();
            }

            void MaterialModelUvNameMapInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                // For some reason the reflected property editor notifications are not symmetrical
                // This function is called continuously anytime a property changes until the edit has completed
                // Because of that, we have to track whether or not we are continuing to edit the same property to know when editing has started and ended
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
                if (property && m_activeProperty != property)
                {
                    m_activeProperty = property;
                }
            }

            void MaterialModelUvNameMapInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
            {
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
                if (property && m_activeProperty == property)
                {
                    uint32_t index = 0;
                    while (&(m_group.m_properties[index]) != property)
                    {
                        ++index;
                    }
                    AZ_Assert(index < m_group.m_properties.size(), "The property doesn't exist in the group.");

                    uint32_t modelUvIndex = AZStd::any_cast<uint32_t>(m_activeProperty->GetValue());
                    if (modelUvIndex == 0u)
                    {
                        m_matModUvOverrides[m_materialUvNames[index].m_shaderInput] = Name();
                    }
                    else
                    {
                        m_matModUvOverrides[m_materialUvNames[index].m_shaderInput] = Name(m_modelUvNames[modelUvIndex]);
                    }

                    if (m_matModUvOverrideMapChangedCallBack)
                    {
                        m_matModUvOverrideMapChangedCallBack(m_matModUvOverrides);
                    }
                }
            }

            void MaterialModelUvNameMapInspector::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
            {
                // As above, there are symmetrical functions on the notification interface for when editing begins and ends and has been completed but they are not being called following that pattern.
                // when this function executes the changes to the property are ready to be committed or reverted
                const AtomToolsFramework::DynamicProperty* property = AtomToolsFramework::FindDynamicPropertyForInstanceDataNode(pNode);
                if (property && m_activeProperty == property)
                {
                    uint32_t index = 0;
                    while (&(m_group.m_properties[index]) != property)
                    {
                        ++index;
                    }
                    AZ_Assert(index < m_group.m_properties.size(), "The property doesn't exist in the group.");

                    uint32_t modelUvIndex = AZStd::any_cast<uint32_t>(m_activeProperty->GetValue());
                    if (modelUvIndex == 0u)
                    {
                        m_matModUvOverrides[m_materialUvNames[index].m_shaderInput] = Name();
                    }
                    else
                    {
                        m_matModUvOverrides[m_materialUvNames[index].m_shaderInput] = Name(m_modelUvNames[modelUvIndex]);
                    }

                    if (m_matModUvOverrideMapChangedCallBack)
                    {
                        m_matModUvOverrideMapChangedCallBack(m_matModUvOverrides);
                    }
                    m_activeProperty = nullptr;
                }
            }

            void MaterialModelUvNameMapInspector::ResetModelUvNameIndices()
            {
                m_modelUvNameIndices.clear();
                const uint32_t uvSize = aznumeric_cast<uint32_t>(m_materialUvNames.size());
                m_modelUvNameIndices.resize(uvSize, 0u);

                AZStd::unordered_map<AZ::Name, uint32_t> tempModelUvIndexLookup;
                uint32_t index = 0u;

                for (const AZStd::string& modelUvName : m_modelUvNames)
                {
                    tempModelUvIndexLookup[AZ::Name(modelUvName)] = index++;
                }

                index = 0u;
                for (const auto& materialUvNamePair : m_materialUvNames)
                {
                    const auto overrideIter = m_matModUvOverrides.find(materialUvNamePair.m_shaderInput);
                    if (overrideIter != m_matModUvOverrides.end())
                    {
                        const auto modelUvIndexIter = tempModelUvIndexLookup.find(overrideIter->second);
                        if (modelUvIndexIter != tempModelUvIndexLookup.end())
                        {
                            m_modelUvNameIndices[index++] = modelUvIndexIter->second;
                        }
                    }
                }
            }

            void MaterialModelUvNameMapInspector::SetModelUvNames(const AZStd::unordered_set<AZ::Name>& modelUvNames)
            {
                static constexpr const char DefaultModelUvName[] = "[Same as in the material]";

                m_modelUvNames.clear();
                // Plus the default name
                m_modelUvNames.reserve(modelUvNames.size() + 1u);

                m_modelUvNames.push_back(DefaultModelUvName);
                for (const AZ::Name& modelUvName : modelUvNames)
                {
                    m_modelUvNames.push_back(modelUvName.GetStringView());
                }

            }

            void MaterialModelUvNameMapInspector::SetUvNameMap(const RPI::MaterialModelUvOverrideMap& matModUvOverrides)
            {
                m_matModUvOverrides = matModUvOverrides;

                ResetModelUvNameIndices();

                const AZStd::string groupName = "ModelUvMap";

                size_t uvSize = m_materialUvNames.size();
                for (size_t i = 0u; i < uvSize; ++i)
                {
                    AtomToolsFramework::DynamicPropertyConfig propertyConfig;
                    const AZStd::string shaderInput = m_materialUvNames[i].m_shaderInput.ToString();
                    const AZStd::string materialUvName = m_materialUvNames[i].m_uvName.GetStringView();

                    propertyConfig.m_dataType = AtomToolsFramework::DynamicPropertyType::Enum;
                    propertyConfig.m_id = AZ::RPI::MaterialPropertyId(groupName, shaderInput).GetFullName();
                    propertyConfig.m_name = shaderInput;
                    propertyConfig.m_displayName = materialUvName;
                    propertyConfig.m_description = shaderInput;
                    propertyConfig.m_defaultValue = 0u;
                    propertyConfig.m_originalValue = 0u;
                    propertyConfig.m_parentValue = 0u;
                    propertyConfig.m_enumValues = m_modelUvNames;

                    m_group.m_properties[i].SetConfig(propertyConfig);
                    m_group.m_properties[i].SetValue(AZStd::any(m_modelUvNameIndices[i]));
                }

                if (m_matModUvOverrideMapChangedCallBack)
                {
                    m_matModUvOverrideMapChangedCallBack(matModUvOverrides);
                }

                RebuildAll();
            }

            bool OpenInspectorDialog(
                const AZ::Data::AssetId& assetId,
                const RPI::MaterialModelUvOverrideMap& matModUvOverrides,
                const AZStd::unordered_set<AZ::Name>& modelUvNames,
                MaterialModelUvOverrideMapChangedCallBack matModUvOverrideMapChangedCallBack)
            {
                QWidget* activeWindow = nullptr;
                AzToolsFramework::EditorWindowRequestBus::BroadcastResult(activeWindow, &AzToolsFramework::EditorWindowRequests::GetAppMainWindow);

                // Constructing a dialog with a table to display all configurable material export items
                QDialog dialog(activeWindow);
                dialog.setWindowTitle("Material Inspector");

                MaterialModelUvNameMapInspector* inspector = new MaterialModelUvNameMapInspector(assetId, matModUvOverrides, modelUvNames, matModUvOverrideMapChangedCallBack, &dialog);
                inspector->Populate();

                // Create the menu button
                QToolButton* menuButton = new QToolButton(&dialog);
                menuButton->setAutoRaise(true);
                menuButton->setIcon(QIcon(":/Cards/img/UI20/Cards/menu_ico.svg"));
                menuButton->setVisible(true);
                QObject::connect(
                    menuButton, &QToolButton::clicked, &dialog, [&]()
                    {
                        QMenu menu(&dialog);
                        menu.addAction(
                            "Clear", [&]
                            {
                                inspector->SetUvNameMap(RPI::MaterialModelUvOverrideMap());
                            });
                        menu.addAction(
                            "Revert", [&]
                            {
                                inspector->SetUvNameMap(matModUvOverrides);
                            });
                        menu.exec(QCursor::pos());
                    });

                QDialogButtonBox* buttonBox = new QDialogButtonBox(&dialog);
                buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

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
                dialog.setFixedSize(300, 300);
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
