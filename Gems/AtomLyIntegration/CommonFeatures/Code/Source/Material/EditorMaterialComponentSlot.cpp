/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <AtomLyIntegration/CommonFeatures/Material/EditorMaterialSystemComponentRequestBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Material/EditorMaterialComponentExporter.h>
#include <Material/EditorMaterialComponentInspector.h>
#include <Material/EditorMaterialComponentSlot.h>
#include <Material/EditorMaterialModelUvNameMapInspector.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAction>
#include <QByteArray>
#include <QCursor>
#include <QDataStream>
#include <QMenu>
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

                const MaterialAssignmentId newId(oldId.first, oldId.second.m_subId);
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
                    ->Version(7, &EditorMaterialComponentSlot::ConvertVersion)
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
                            ->Attribute("ThumbnailIcon", &EditorMaterialComponentSlot::GetPreviewPixmapData)
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
                    ;
            }
        };

        AZStd::vector<char> EditorMaterialComponentSlot::GetPreviewPixmapData() const
        {
            if (!GetActiveAssetId().IsValid())
            {
                return {};
            }

            QPixmap pixmap;
            EditorMaterialSystemComponentRequestBus::BroadcastResult(
                pixmap, &EditorMaterialSystemComponentRequestBus::Events::GetRenderedMaterialPreview, m_entityId, m_id);
            if (pixmap.isNull())
            {
                if (m_updatePreview)
                {
                    EditorMaterialSystemComponentRequestBus::Broadcast(
                        &EditorMaterialSystemComponentRequestBus::Events::RenderMaterialPreview, m_entityId, m_id);
                    m_updatePreview = false;
                }
                return {};
            }

            QByteArray pixmapBytes;
            QDataStream stream(&pixmapBytes, QIODevice::WriteOnly);
            stream << pixmap;
            return AZStd::vector<char>(pixmapBytes.begin(), pixmapBytes.end());
        }

        AZ::Data::AssetId EditorMaterialComponentSlot::GetActiveAssetId() const
        {
            return m_materialAsset.GetId().IsValid() ? m_materialAsset.GetId() : GetDefaultAssetId();
        }

        AZ::Data::AssetId EditorMaterialComponentSlot::GetDefaultAssetId() const
        {
            AZ::Data::AssetId assetId;
            MaterialComponentRequestBus::EventResult(
                assetId, m_entityId, &MaterialComponentRequestBus::Events::GetDefaultMaterialAssetId, m_id);
            return assetId;
        }

        AZStd::string EditorMaterialComponentSlot::GetLabel() const
        {
            AZStd::string label;
            MaterialComponentRequestBus::EventResult(label, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialSlotLabel, m_id);
            return label;
        }

        bool EditorMaterialComponentSlot::HasSourceData() const
        {
            // The slot only has valid source data if the source path is valid and the file has the correct extension
            const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetActiveAssetId());
            return !sourcePath.empty() && AZ::StringFunc::Path::IsExtension(sourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
        }

        void EditorMaterialComponentSlot::SetAsset(const Data::AssetId& assetId)
        {
            m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(assetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetMaterialOverride, m_id, m_materialAsset.GetId());
            OnDataChanged();
        }

        void EditorMaterialComponentSlot::SetAsset(const Data::Asset<RPI::MaterialAsset>& asset)
        {
            m_materialAsset = asset;
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetMaterialOverride, m_id, m_materialAsset.GetId());
            OnDataChanged();
        }

        void EditorMaterialComponentSlot::Clear()
        {
            m_materialAsset = {};
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetMaterialOverride, m_id, m_materialAsset.GetId());
            ClearOverrides();
        }

        void EditorMaterialComponentSlot::ClearOverrides()
        {
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetPropertyOverrides, m_id, MaterialPropertyOverrideMap());
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id, AZ::RPI::MaterialModelUvOverrideMap());
            OnDataChanged();
        }

        void EditorMaterialComponentSlot::OpenMaterialExporter()
        {
            // Because we are generating a source material from this specific slot there is only one entry
            // But we still need to allow the user to reconfigure it using the dialog
            EditorMaterialComponentExporter::ExportItemsContainer exportItems;
            {
                EditorMaterialComponentExporter::ExportItem exportItem{ GetDefaultAssetId(), GetLabel() };
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

                    // Generate a new asset ID utilizing the export file path so that we can update this material slot to reference the new
                    // asset
                    const auto& assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(exportItem.GetExportPath(), 0);
                    if (assetIdOutcome)
                    {
                        m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
                            assetIdOutcome.GetValue(), AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
                        changed = true;
                    }
                }
            }

            if (changed)
            {
                OnMaterialChanged();
            }
        }

        void EditorMaterialComponentSlot::OpenMaterialEditor() const
        {
            const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetActiveAssetId());
            if (!sourcePath.empty() && AZ::StringFunc::Path::IsExtension(sourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension))
            {
                EditorMaterialSystemComponentRequestBus::Broadcast(
                    &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor, sourcePath);
            }
        }

        void EditorMaterialComponentSlot::OpenMaterialInspector()
        {
            EditorMaterialSystemComponentRequestBus::Broadcast(
                &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialInspector, m_entityId, m_id);
        }

        void EditorMaterialComponentSlot::OpenUvNameMapInspector()
        {
            if (GetActiveAssetId().IsValid())
            {
                AZStd::unordered_set<AZ::Name> modelUvNames;
                MaterialReceiverRequestBus::EventResult(modelUvNames, m_entityId, &MaterialReceiverRequestBus::Events::GetModelUvNames);

                RPI::MaterialModelUvOverrideMap matModUvOverrides;
                MaterialComponentRequestBus::EventResult(
                    matModUvOverrides, m_entityId, &MaterialComponentRequestBus::Events::GetModelUvOverrides, m_id);

                auto applyMatModUvOverrideChangedCallback = [this](const RPI::MaterialModelUvOverrideMap& matModUvOverrides)
                {
                    MaterialComponentRequestBus::Event(
                        m_entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id, matModUvOverrides);
                };

                if (EditorMaterialComponentInspector::OpenInspectorDialog(
                        GetActiveAssetId(), matModUvOverrides, modelUvNames, applyMatModUvOverrideChangedCallback))
                {
                    OnDataChanged();
                }
            }
        }

        void EditorMaterialComponentSlot::OpenPopupMenu([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetType& assetType)
        {
            QMenu menu;

            QAction* action = nullptr;

            action = menu.addAction("Generate/Manage Source Material...", [this]() { OpenMaterialExporter(); });
            action->setEnabled(GetDefaultAssetId().IsValid());

            menu.addSeparator();

            action = menu.addAction("Edit Source Material...", [this]() { OpenMaterialEditor(); });
            action->setEnabled(HasSourceData());

            action = menu.addAction("Edit Material Instance...", [this]() { OpenMaterialInspector(); });
            action->setEnabled(GetActiveAssetId().IsValid());

            action = menu.addAction("Edit Material Instance UV Map...", [this]() { OpenUvNameMapInspector(); });
            action->setEnabled(GetActiveAssetId().IsValid());

            menu.addSeparator();

            MaterialPropertyOverrideMap propertyOverrides;
            MaterialComponentRequestBus::EventResult(
                propertyOverrides, m_entityId, &MaterialComponentRequestBus::Events::GetPropertyOverrides, m_id);
            RPI::MaterialModelUvOverrideMap matModUvOverrides;
            MaterialComponentRequestBus::EventResult(
                matModUvOverrides, m_entityId, &MaterialComponentRequestBus::Events::GetModelUvOverrides, m_id);

            action = menu.addAction("Clear Material Instance Overrides", [this]() { ClearOverrides(); });
            action->setEnabled(!propertyOverrides.empty() || !matModUvOverrides.empty());

            menu.exec(QCursor::pos());
        }

        void EditorMaterialComponentSlot::OnMaterialChanged() const
        {
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetMaterialOverride, m_id, m_materialAsset.GetId());
            OnDataChanged();
        }

        void EditorMaterialComponentSlot::OnDataChanged() const
        {
            // This is triggered whenever a material slot changes outside of normal inspector interactions
            // Handle undo, update configuration, and refresh the inspector to display the new values
            AzToolsFramework::ScopedUndoBatch undoBatch("Material slot changed.");
            AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, m_entityId);

            EditorMaterialSystemComponentRequestBus::Broadcast(
                &EditorMaterialSystemComponentRequestBus::Events::RenderMaterialPreview, m_entityId, m_id);
            m_updatePreview = false;

            MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialsEdited);

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }
    } // namespace Render
} // namespace AZ
