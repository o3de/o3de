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
                constexpr AZ::u32 materialIdDataCrc = AZ_CRC_CE("id");

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
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponentSlot::OnMaterialChangedFromRPE)
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
                    ->Method("GetPreviewPixmapData", &EditorMaterialComponentSlot::GetPreviewPixmapData)
                    ->Method("GetActiveAssetId", &EditorMaterialComponentSlot::GetActiveAssetId)
                    ->Method("GetDefaultAssetId", &EditorMaterialComponentSlot::GetDefaultAssetId)
                    ->Method("GetLabel", &EditorMaterialComponentSlot::GetLabel)
                    ->Method("HasSourceData", &EditorMaterialComponentSlot::HasSourceData)
                    ->Method("SetAsset", &EditorMaterialComponentSlot::SetAsset)
                    ->Method("SetAssetId", &EditorMaterialComponentSlot::SetAssetId)
                    ->Method("Clear", &EditorMaterialComponentSlot::Clear)
                    ->Method("ClearMaterial", &EditorMaterialComponentSlot::ClearMaterial)
                    ->Method("ClearProperties", &EditorMaterialComponentSlot::ClearProperties)
                    ->Method("OpenMaterialExporter", &EditorMaterialComponentSlot::OpenMaterialExporter)
                    ->Method("OpenMaterialEditor", &EditorMaterialComponentSlot::OpenMaterialEditor)
                    ->Method("OpenMaterialInspector", &EditorMaterialComponentSlot::OpenMaterialInspector)
                    ->Method("OpenUvNameMapInspector", &EditorMaterialComponentSlot::OpenUvNameMapInspector)
                    ->Method("ExportMaterial", &EditorMaterialComponentSlot::ExportMaterial)
                    ;
            }
        }

        EditorMaterialComponentSlot::EditorMaterialComponentSlot(
            const AZ::EntityId& entityId, const MaterialAssignmentId& materialAssignmentId)
            : m_entityId(entityId)
            , m_id(materialAssignmentId)
        {
            bool isOverridden = false;
            MaterialComponentRequestBus::EventResult(
                isOverridden, m_entityId, &MaterialComponentRequestBus::Events::IsMaterialAssetIdOverridden, m_id);

            AZ::Data::AssetId assetId = {};
            MaterialComponentRequestBus::EventResult(assetId, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId, m_id);
            if (assetId.IsValid())
            {
                AZ::Data::AssetBus::Handler::BusConnect(assetId);
                if (isOverridden)
                {
                    m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(assetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
                }
            }

            EditorMaterialSystemComponentNotificationBus::Handler::BusConnect();
        }

        EditorMaterialComponentSlot::~EditorMaterialComponentSlot()
        {
            EditorMaterialSystemComponentNotificationBus::Handler::BusDisconnect();
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        AZStd::vector<char> EditorMaterialComponentSlot::GetPreviewPixmapData() const
        {
            // Don't display a custom image if there is no material asset assigned to this slot.
            if (!GetActiveAssetId().IsValid())
            {
                return {};
            }

            // Don't display a custom image if no properties have been overridden. It will fall back to the default thumbnail.
            bool hasPropertiesOverridden = false;
            MaterialComponentRequestBus::EventResult(
                hasPropertiesOverridden, m_entityId, &MaterialComponentRequestBus::Events::HasPropertiesOverridden, m_id);
            if (!hasPropertiesOverridden)
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
                    UpdatePreview();
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
            AZ::Data::AssetId assetId = {};
            MaterialComponentRequestBus::EventResult(
                assetId, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId, m_id);
            return assetId;
        }

        AZ::Data::AssetId EditorMaterialComponentSlot::GetDefaultAssetId() const
        {
            AZ::Data::AssetId assetId = {};
            MaterialComponentRequestBus::EventResult(
                assetId, m_entityId, &MaterialComponentRequestBus::Events::GetDefaultMaterialAssetId, m_id);
            return assetId;
        }

        AZStd::string EditorMaterialComponentSlot::GetLabel() const
        {
            AZStd::string label;
            MaterialComponentRequestBus::EventResult(label, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialLabel, m_id);
            return label;
        }

        bool EditorMaterialComponentSlot::HasSourceData() const
        {
            // The slot only has valid source data if the source path is valid and the file has the correct extension
            const AZStd::string& sourcePath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetActiveAssetId());
            return !sourcePath.empty() && AZ::StringFunc::Path::IsExtension(sourcePath.c_str(), AZ::RPI::MaterialSourceData::Extension);
        }

        void EditorMaterialComponentSlot::SetAsset(const Data::Asset<RPI::MaterialAsset>& asset)
        {
            m_materialAsset = asset;
            OnDataChanged({ m_entityId }, true);
        }

        void EditorMaterialComponentSlot::SetAssetId(const Data::AssetId& assetId)
        {
            SetAsset(AZ::Data::Asset<AZ::RPI::MaterialAsset>(assetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid()));
        }

        void EditorMaterialComponentSlot::Clear()
        {
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetPropertyValues, m_id, MaterialPropertyOverrideMap());
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id, AZ::RPI::MaterialModelUvOverrideMap());
            SetAsset({});
        }

        void EditorMaterialComponentSlot::ClearMaterial()
        {
            SetAsset({});
        }

        void EditorMaterialComponentSlot::ClearProperties()
        {
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetPropertyValues, m_id, MaterialPropertyOverrideMap());
            MaterialComponentRequestBus::Event(
                m_entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id, AZ::RPI::MaterialModelUvOverrideMap());
            OnDataChanged({ m_entityId }, false);
        }

        void EditorMaterialComponentSlot::OpenMaterialExporter(const AzToolsFramework::EntityIdSet& entityIdsToEdit)
        {
            // Because we are generating a source material from this specific slot there is only one entry
            // But we still need to allow the user to reconfigure it using the dialog
            EditorMaterialComponentExporter::ExportItemsContainer exportItems;
            exportItems.emplace_back(GetDefaultAssetId(), GetLabel());

            if (EditorMaterialComponentExporter::OpenExportDialog(exportItems))
            {
                const auto& exportItem = exportItems.front();
                if (EditorMaterialComponentExporter::ExportMaterialSourceData(exportItem))
                {
                    if (const auto& assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(exportItem.GetExportPath(), 0))
                    {
                        m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
                            assetIdOutcome.GetValue(), AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
                        OnDataChanged(entityIdsToEdit, true);
                    }
                }
            }
        }

        void EditorMaterialComponentSlot::ExportMaterial(const AZStd::string& exportPath, bool overwrite)
        {
            EditorMaterialComponentExporter::ProgressDialog progressDialog("Generating materials", "Generating material...", 1);

            EditorMaterialComponentExporter::ExportItem exportItem(GetDefaultAssetId(), GetLabel(), exportPath);
            exportItem.SetOverwrite(overwrite);

            if (EditorMaterialComponentExporter::ExportMaterialSourceData(exportItem))
            {
                const AZ::Data::AssetInfo assetInfo = progressDialog.ProcessItem(exportItem);
                if (assetInfo.m_assetId.IsValid())
                {
                    SetAssetId(assetInfo.m_assetId);
                    progressDialog.CompleteItem();
                }
            }
        }

        void EditorMaterialComponentSlot::OpenMaterialCanvas() const
        {
            EditorMaterialSystemComponentRequestBus::Broadcast(&EditorMaterialSystemComponentRequestBus::Events::OpenMaterialCanvas, "");
        }

        void EditorMaterialComponentSlot::OpenMaterialEditor() const
        {
            EditorMaterialSystemComponentRequestBus::Broadcast(
                &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialEditor,
                AZ::RPI::AssetUtils::GetSourcePathByAssetId(GetActiveAssetId()));
        }

        void EditorMaterialComponentSlot::OpenMaterialInspector(const AzToolsFramework::EntityIdSet& entityIdsToEdit)
        {
            EditorMaterialSystemComponentRequestBus::Broadcast(
                &EditorMaterialSystemComponentRequestBus::Events::OpenMaterialInspector, m_entityId, entityIdsToEdit, m_id);
        }

        void EditorMaterialComponentSlot::OpenUvNameMapInspector(const AzToolsFramework::EntityIdSet& entityIdsToEdit)
        {
            if (GetActiveAssetId().IsValid())
            {
                AZStd::unordered_set<AZ::Name> modelUvNames;
                MaterialConsumerRequestBus::EventResult(modelUvNames, m_entityId, &MaterialConsumerRequestBus::Events::GetModelUvNames);

                RPI::MaterialModelUvOverrideMap matModUvOverrides;
                MaterialComponentRequestBus::EventResult(
                    matModUvOverrides, m_entityId, &MaterialComponentRequestBus::Events::GetModelUvOverrides, m_id);

                auto applyMatModUvOverrideChangedCallback = [&](const RPI::MaterialModelUvOverrideMap& matModUvOverrides)
                {
                    for (const AZ::EntityId& entityId : entityIdsToEdit)
                    {
                        MaterialComponentRequestBus::Event(
                            entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id, matModUvOverrides);
                    }
                };

                if (EditorMaterialComponentInspector::OpenInspectorDialog(
                        GetActiveAssetId(), matModUvOverrides, modelUvNames, applyMatModUvOverrideChangedCallback))
                {
                    OnDataChanged(entityIdsToEdit, false);
                }
            }
        }

        void EditorMaterialComponentSlot::OpenPopupMenu([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetType& assetType)
        {
            const auto& entityIdsToEdit = EditorMaterialComponentUtil::GetSelectedEntitiesFromActiveInspector();
            const bool hasMatchingSlots = EditorMaterialComponentUtil::DoEntitiesHaveMatchingMaterialSlots(m_entityId, entityIdsToEdit);
            const bool hasMatchingMaterialTypes = EditorMaterialComponentUtil::DoEntitiesHaveMatchingMaterialTypes(m_entityId, entityIdsToEdit, m_id);

            QMenu menu;

            QAction* action = nullptr;

            action = menu.addAction("Generate/Manage Source Material...", [this, entityIdsToEdit]() { OpenMaterialExporter(entityIdsToEdit); });
            action->setEnabled(GetDefaultAssetId().IsValid() && hasMatchingSlots);

            menu.addSeparator();

            action = menu.addAction("Open Material Editor...", [this]() { OpenMaterialEditor(); });
            action = menu.addAction("Open Material Canvas...", [this]() { OpenMaterialCanvas(); });

            action = menu.addAction("Open Material Instance Editor...", [this, entityIdsToEdit]() { OpenMaterialInspector(entityIdsToEdit); });
            action->setEnabled(GetActiveAssetId().IsValid() && hasMatchingMaterialTypes);

            action = menu.addAction("Open Material Instance UV Map Editor...", [this, entityIdsToEdit]() { OpenUvNameMapInspector(entityIdsToEdit); });
            action->setEnabled(GetActiveAssetId().IsValid() && hasMatchingMaterialTypes);

            menu.addSeparator();

            action = menu.addAction("Clear Material Instance Overrides", [this, entityIdsToEdit]() {
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::SetPropertyValues, m_id, MaterialPropertyOverrideMap());
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id,
                        AZ::RPI::MaterialModelUvOverrideMap());
                }
                OnDataChanged(entityIdsToEdit, false);
            });

            action = menu.addAction("Clear Material And Properties", [this, entityIdsToEdit]() {
                m_materialAsset = {};
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::SetPropertyValues, m_id, MaterialPropertyOverrideMap());
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::SetModelUvOverrides, m_id,
                        AZ::RPI::MaterialModelUvOverrideMap());
                }
                OnDataChanged(entityIdsToEdit, true);
            });

            menu.exec(QCursor::pos());
        }

        void EditorMaterialComponentSlot::OnMaterialChangedFromRPE()
        {
            // Because this function is being from an edit context attribute it will automatically be applied to all selected entities
            OnDataChanged({ m_entityId }, true);
        }

        void EditorMaterialComponentSlot::OnDataChanged(const AzToolsFramework::EntityIdSet& entityIdsToEdit, bool updateAsset)
        {
            // Handle undo, update configuration, and refresh the inspector to display the new values
            AzToolsFramework::ScopedUndoBatch undoBatch("Material slot changed.");

            for (const AZ::EntityId& entityId : entityIdsToEdit)
            {
                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);

                if (updateAsset)
                {
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::SetMaterialAssetId, m_id, m_materialAsset.GetId());
                }

                EditorMaterialSystemComponentRequestBus::Broadcast(
                    &EditorMaterialSystemComponentRequestBus::Events::RenderMaterialPreview, entityId, m_id);

                MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
            }

            m_updatePreview = false;

            // Reconnect the asset bus to the current active material asset ID so that the preview can be refreshed if the asset changes
            AZ::Data::AssetId assetId = {};
            MaterialComponentRequestBus::EventResult(assetId, m_entityId, &MaterialComponentRequestBus::Events::GetMaterialAssetId, m_id);

            if (!AZ::Data::AssetBus::Handler::BusIsConnectedId(assetId))
            {
                AZ::Data::AssetBus::Handler::BusDisconnect();
                if (assetId.IsValid())
                {
                    AZ::Data::AssetBus::Handler::BusConnect(assetId);
                }
            }

            // Refresh the attributes and values for the inspector UI
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }

        void EditorMaterialComponentSlot::OnRenderMaterialPreviewReady(
            [[maybe_unused]] const AZ::EntityId& entityId,
            [[maybe_unused]] const AZ::Render::MaterialAssignmentId& materialAssignmentId,
            [[maybe_unused]] const QPixmap& pixmap)
        {
            if (entityId == m_entityId && materialAssignmentId == m_id)
            {
                AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
            }
        }

        void EditorMaterialComponentSlot::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            UpdatePreview();
        }

        void EditorMaterialComponentSlot::UpdatePreview() const
        {
            m_updatePreview = false;

            bool hasPropertiesOverridden = false;
            MaterialComponentRequestBus::EventResult(
                hasPropertiesOverridden, m_entityId, &MaterialComponentRequestBus::Events::HasPropertiesOverridden, m_id);
            if (!hasPropertiesOverridden)
            {
                return;
            }

            EditorMaterialSystemComponentRequestBus::Broadcast(
                &EditorMaterialSystemComponentRequestBus::Events::RenderMaterialPreview, m_entityId, m_id);
        }
    } // namespace Render
} // namespace AZ
