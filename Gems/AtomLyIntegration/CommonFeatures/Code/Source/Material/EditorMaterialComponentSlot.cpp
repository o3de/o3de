/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/EditorMaterialComponentSlot.h>
#include <Material/EditorMaterialComponentExporter.h>
#include <Material/EditorMaterialComponentInspector.h>
#include <Material/EditorMaterialModelUvNameMapInspector.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QMenu>
#include <QAction>
#include <QCursor>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        // Update serialized data to the new format and data types
        bool EditorMaterialComponentSlot::ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                constexpr AZ::u32 materialIdDataCrc = AZ_CRC("id", 0xbf396750);

                AZStd::pair<MaterialAssignmentLodIndex, AZ::Data::AssetId> oldId;
                if (!classElement.GetChildData(materialIdDataCrc, oldId))
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentSlot::ConvertVersion", false, "Failed to get id element");
                    return false;
                }

                if (!classElement.RemoveElementByName(materialIdDataCrc))
                {
                    AZ_Error("AZ::Render::EditorMaterialComponentSlot::ConvertVersion", false, "Failed to remove id element");
                    return false;
                }

                const MaterialAssignmentId newId(oldId.first, oldId.second);
                classElement.AddElementWithData(context, "id", newId);
            }

            if (classElement.GetVersion() < 4)
            {
                constexpr AZ::u32 matModUvOverridesCrc = AZ_CRC_CE("matModUvOverrides");
                AZStd::unordered_map<uint32_t, AZ::Name> oldMatModUvOverrides;
                if (classElement.GetChildData(matModUvOverridesCrc, oldMatModUvOverrides))
                {
                    // This feature is very new, so we assume that any existing matModUvOverrides data is empty.
                    AZ_Error("AZ::Render::EditorMaterialComponentSlot::ConvertVersion", oldMatModUvOverrides.empty(), "Conversion from old matModUvOverrides is not supported. The new overrides map will be empty.");

                    // We just consume the old element to avoid reporting errors about an unknown class ID when serialization continues.
                    classElement.RemoveElementByName(matModUvOverridesCrc);
                }
            }

            if (classElement.GetVersion() < 5)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("matModUvOverrides"));
                classElement.RemoveElementByName(AZ_CRC_CE("propertyOverrides"));
            }

            return true;
        }

        void EditorMaterialComponentSlot::Reflect(ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorMaterialComponentSlot>()
                    ->Version(5, &EditorMaterialComponentSlot::ConvertVersion)
                    ->Field("id", &EditorMaterialComponentSlot::m_id)
                    ->Field("materialAsset", &EditorMaterialComponentSlot::m_materialAsset)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMaterialComponentSlot>(
                        "EditorMaterialComponentSlot", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponentSlot::m_materialAsset, "Material Asset", "")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponentSlot::OnMaterialChanged)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::DefaultAsset, &EditorMaterialComponentSlot::GetDefaultAssetId)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &EditorMaterialComponentSlot::GetLabel)
                            ->Attribute(AZ::Edit::Attributes::ShowProductAssetFileName, true)
                            ->Attribute("ThumbnailCallback", &EditorMaterialComponentSlot::OpenPopupMenu)
                        ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorMaterialComponentSlot>("EditorMaterialComponentSlot")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "editor")
                    ->Constructor()
                    ->Constructor<const EditorMaterialComponentSlot&>()
                    ->Property("id", BehaviorValueProperty(&EditorMaterialComponentSlot::m_id))
                    ->Property("materialAsset", BehaviorValueProperty(&EditorMaterialComponentSlot::m_materialAsset))
                    ->Property("propertyOverrides", BehaviorValueProperty(&EditorMaterialComponentSlot::m_propertyOverrides))
                    ->Property("matModUvOverrides", BehaviorValueProperty(&EditorMaterialComponentSlot::m_matModUvOverrides))
                    ;
            }
        };

        AZ::Data::AssetId EditorMaterialComponentSlot::GetDefaultAssetId() const
        {
            return m_id.m_materialAssetId;
        }

        AZStd::string EditorMaterialComponentSlot::GetLabel() const
        {
            // Generate the label for the material slot based on the assignment ID
            // If this is the default material assignment ID then it represents the default slot which is not contained in any other group
            if (m_id == DefaultMaterialAssignmentId)
            {
                return "Default Material";
            }

            // Otherwise the label can be generated by parsing the source file name associated with the asset ID
            const AZStd::string& label = EditorMaterialComponentExporter::GetLabelByAssetId(m_id.m_materialAssetId);
            return !label.empty() ? label : "<unknown>";
        }

        bool EditorMaterialComponentSlot::HasSourceData() const
        {
            // The slot only has valid source data if the source path is valid and the file has the correct extension
            const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(m_materialAsset.GetId());
            return !sourcePath.empty() && AZ::StringFunc::Path::IsExtension(sourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
        }

        void EditorMaterialComponentSlot::OnMaterialChanged() const
        {
            if (m_materialChangedCallback)
            {
                m_materialChangedCallback();
            }
        }

        void EditorMaterialComponentSlot::OnPropertyChanged() const
        {
            if (m_propertyChangedCallback)
            {
                m_propertyChangedCallback();
            }
        }

        void EditorMaterialComponentSlot::OpenMaterialEditor() const
        {
            const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(m_materialAsset.GetId());
            if (!sourcePath.empty() && AZ::StringFunc::Path::IsExtension(sourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
            {
                EditorMaterialSystemComponentRequestBus::Broadcast(&EditorMaterialSystemComponentRequestBus::Events::OpenInMaterialEditor, sourcePath);
            }
        }

        void EditorMaterialComponentSlot::Clear()
        {
            m_materialAsset = {};
            ClearOverrides();
        }

        void EditorMaterialComponentSlot::ClearOverrides()
        {
            m_propertyOverrides = {};
            m_matModUvOverrides = {};
            OnMaterialChanged();
        }

        void EditorMaterialComponentSlot::SetDefaultAsset()
        {
            m_materialAsset = {};
            m_propertyOverrides = {};
            m_matModUvOverrides = {};
            if (m_id.m_materialAssetId.IsValid())
            {
                // If no material is assigned to this slot, assign the default material from the slot id to edit its properties
                m_materialAsset.Create(m_id.m_materialAssetId);
            }
            OnMaterialChanged();
        }

        void EditorMaterialComponentSlot::OpenMaterialExporter()
        {
            // Because we are generating a source material from this specific slot there is only one entry
            // But we still need to allow the user to reconfigure it using the dialogue
            EditorMaterialComponentExporter::ExportItemsContainer exportItems;
            {
                EditorMaterialComponentExporter::ExportItem exportItem;
                exportItem.m_assetId = m_id.m_materialAssetId;
                exportItems.push_back(exportItem);
            }

            bool changed = false;
            if (EditorMaterialComponentExporter::OpenExportDialog(exportItems))
            {
                for (const EditorMaterialComponentExporter::ExportItem& exportItem : exportItems)
                {
                    if (!EditorMaterialComponentExporter::ExportMaterialSourceData(exportItem))
                    {
                        continue;
                    }

                    // Generate a new asset ID utilizing the export file path so that we can update this material slot to reference the new asset
                    const auto& assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(exportItem.m_exportPath, 0);
                    if (assetIdOutcome)
                    {
                        m_materialAsset.Create(assetIdOutcome.GetValue());
                        changed = true;
                    }
                }
            }

            if (changed)
            {
                OnMaterialChanged();
            }
        }

        void EditorMaterialComponentSlot::OpenMaterialInspector()
        {
            MaterialPropertyOverrideMap initialPropertyOverrides = m_propertyOverrides;
            auto applyPropertyChangedCallback = [this](const MaterialPropertyOverrideMap& propertyOverrides) {
                m_propertyOverrides = propertyOverrides;
                OnPropertyChanged();
            };

            if (m_materialAsset.GetId().IsValid())
            {
                if (EditorMaterialComponentInspector::OpenInspectorDialog(GetLabel(), m_materialAsset.GetId(), m_propertyOverrides, applyPropertyChangedCallback))
                {
                    OnMaterialChanged();
                }
            }
        }

        void EditorMaterialComponentSlot::OpenUvNameMapInspector()
        {
            RPI::MaterialModelUvOverrideMap initialUvOverrides = m_matModUvOverrides;
            auto applyMatModUvOverrideChangedCallback = [this](const RPI::MaterialModelUvOverrideMap& matModUvOverrides) {
                m_matModUvOverrides = matModUvOverrides;
                // Treated as a special property. It will be updated together with properties.
                OnPropertyChanged();
            };

            if (m_materialAsset.GetId().IsValid())
            {
                if (EditorMaterialComponentInspector::OpenInspectorDialog(m_materialAsset.GetId(), m_matModUvOverrides, m_modelUvNames, applyMatModUvOverrideChangedCallback))
                {
                    OnMaterialChanged();
                }
            }
        }

        void EditorMaterialComponentSlot::OpenPopupMenu([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetType& assetType)
        {
            QMenu menu;

            QAction* action = nullptr;

            action = menu.addAction("Generate/Manage Source Material...", [this]() { OpenMaterialExporter(); });
            action->setEnabled(m_id.m_materialAssetId.IsValid());

            menu.addSeparator();

            action = menu.addAction("Edit Source Material...", [this]() { OpenMaterialEditor(); });
            action->setEnabled(HasSourceData());

            action = menu.addAction("Edit Material Instance...", [this]() { OpenMaterialInspector(); });
            action->setEnabled(m_materialAsset.GetId().IsValid());

            action = menu.addAction("Edit Material Instance UV Map...", [this]() { OpenUvNameMapInspector(); });
            action->setEnabled(m_materialAsset.GetId().IsValid());

            menu.addSeparator();

            action = menu.addAction("Clear Material Instance Overrides", [this]() { ClearOverrides(); });
            action->setEnabled(!m_propertyOverrides.empty() || !m_matModUvOverrides.empty());

            menu.exec(QCursor::pos());
        }
    } // namespace Render
} // namespace AZ
