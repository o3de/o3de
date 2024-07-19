/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Material/EditorMaterialComponent.h>
#include <Material/EditorMaterialComponentExporter.h>
#include <Material/EditorMaterialComponentSerializer.h>
#include <Material/EditorMaterialComponentUtil.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QMenu>
#include <QProgressDialog>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        const char* EditorMaterialComponent::GenerateMaterialsButtonText = "Generate/Manage Source Materials...";
        const char* EditorMaterialComponent::GenerateMaterialsToolTipText = "Generate editable source material files from materials provided by the model.";

        // Update serialized data to the new format and data types
        bool EditorMaterialComponent::ConvertVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (!BaseClass::ConvertToEditorRenderComponentAdapter<1>(context, classElement))
            {
                return false;
            }

            if (classElement.GetVersion() < 3)
            {
                AZ_Error("EditorMaterialComponent", false, "Material Component version < 3 is no longer supported");
                return false;
            }

            if (classElement.GetVersion() < 4)
            {
                classElement.AddElementWithData(context, "materialSlotsByLodEnabled", true);
            }
            
            return true;
        }

        void EditorMaterialComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);
            EditorMaterialComponentSlot::Reflect(context);

            if (auto jsonContext = azrtti_cast<JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonEditorMaterialComponentSerializer>()->HandlesType<EditorMaterialComponent>();
            }

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<EditorMaterialComponentSlotContainer>();
                serializeContext->RegisterGenericType<EditorMaterialComponentSlotsByLodContainer>();
                serializeContext->RegisterGenericType<AZStd::unordered_map<MaterialAssignmentId, Data::AssetId, AZStd::hash<MaterialAssignmentId>, AZStd::equal_to<MaterialAssignmentId>, AZStd::allocator>>();
                serializeContext->RegisterGenericType<AZStd::unordered_map<MaterialAssignmentId, MaterialPropertyOverrideMap, AZStd::hash<MaterialAssignmentId>, AZStd::equal_to<MaterialAssignmentId>, AZStd::allocator>>();

                serializeContext->Class<EditorMaterialComponent, BaseClass>()
                    ->Version(5, &EditorMaterialComponent::ConvertVersion)
                    ->Field("defaultMaterialSlot", &EditorMaterialComponent::m_defaultMaterialSlot)
                    ->Field("materialSlots", &EditorMaterialComponent::m_materialSlots)
                    ->Field("materialSlotsByLodEnabled", &EditorMaterialComponent::m_materialSlotsByLodEnabled)
                    ->Field("materialSlotsByLod", &EditorMaterialComponent::m_materialSlotsByLod)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMaterialComponent>(
                        "Material", "The material component specifies the material to use for this entity")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Graphics/Mesh")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/material/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::MaterialAsset>::Uuid())
                        ->UIElement(AZ::Edit::UIHandlers::Button, GenerateMaterialsButtonText, GenerateMaterialsToolTipText)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, GenerateMaterialsButtonText)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OpenMaterialExporterFromRPE)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_defaultMaterialSlot, "Default Material", "Materials assigned to this slot will be applied to the entire model unless specific model or LOD materials are set.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_materialSlots, "Model Materials", "Materials assigned to these slots will be applied to every part of the model with same material slot name unless an overriding LOD material is specified.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_materialSlotsByLodEnabled, "Enable LOD Materials", "When this flag is enabled, materials can be specified per LOD.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnLodsToggled)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_materialSlotsByLod, "LOD Materials", "Materials assigned to these slots will take precedence over all other materials settings.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &EditorMaterialComponent::GetLabelForLod)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMaterialComponent::GetLodVisibility)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->ElementAttribute(AZ::Edit::Attributes::AutoExpand, false)
                            ->ElementAttribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ;

                    editContext->Class<MaterialComponentConfig>(
                        "Material Component Config", "Material Component Config")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialComponentConfig::m_materials, "Materials", "")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorMaterialComponentTypeId", BehaviorConstant(Uuid(EditorMaterialComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ;
            }
        }

        EditorMaterialComponent::EditorMaterialComponent(const MaterialComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorMaterialComponent::Activate()
        {
            BaseClass::Activate();
            MaterialComponentNotificationBus::Handler::BusConnect(GetEntityId());
            UpdateMaterialSlots();
        }

        void EditorMaterialComponent::Deactivate()
        {
            MaterialComponentNotificationBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorMaterialComponent::AddContextMenuActions(QMenu* menu)
        {
            const auto& entityIdsToEdit = EditorMaterialComponentUtil::GetSelectedEntitiesFromActiveInspector();

            QAction* action = nullptr;

            menu->addSeparator();

            action = menu->addAction(GenerateMaterialsButtonText, [this, entityIdsToEdit]() { OpenMaterialExporter(entityIdsToEdit); });
            action->setToolTip(GenerateMaterialsToolTipText);
            action->setEnabled(EditorMaterialComponentUtil::DoEntitiesHaveMatchingMaterialSlots(GetEntityId(), entityIdsToEdit));

            menu->addSeparator();

            action = menu->addAction("Clear Materials", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clear materials.");
                m_materialSlotsByLodEnabled = false;
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                    MaterialComponentRequestBus::Event(entityId, &MaterialComponentRequestBus::Events::ClearMaterialMap);
                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Clears all material and property overrides.");

            action = menu->addAction("Clear Materials On Model Slots", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clear materials on model slots.");
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                    MaterialComponentRequestBus::Event(entityId, &MaterialComponentRequestBus::Events::ClearMaterialsOnModelSlots);
                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Clears material and property overrides assigned to the Model Materials group.");

            action = menu->addAction("Clear Materials On LOD Slots", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clear materials on LOD slots.");
                m_materialSlotsByLodEnabled = false;
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                    MaterialComponentRequestBus::Event(entityId, &MaterialComponentRequestBus::Events::ClearMaterialsOnLodSlots);
                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Clears material and property overrides assigned to the LOD Materials group.");

            action = menu->addAction("Clear Materials On Invalid Slots", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clear materials on invalid slots.");
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                    MaterialComponentRequestBus::Event(
                        entityId, &MaterialComponentRequestBus::Events::ClearMaterialsOnInvalidSlots);
                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Clears residual or hidden material and property overrides assigned to slots that do not match the current layout.");

            action = menu->addAction("Clear Materials With Missing Assets", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clear materials with missing assets.");
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                    MaterialComponentRequestBus::Event(entityId, &MaterialComponentRequestBus::Events::ClearMaterialsWithMissingAssets);
                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Clears material overrides referencing missing assets.");

            action = menu->addAction("Repair Materials With Missing Assets", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Repair materials with missing assets.");
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);
                    MaterialComponentRequestBus::Event(entityId, &MaterialComponentRequestBus::Events::RepairMaterialsWithMissingAssets);
                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Removes references to any missing material assets.");
            
            action = menu->addAction("Repair Materials With Renamed Properties", [this, entityIdsToEdit]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Repair materials with renamed properties.");
                for (const AZ::EntityId& entityId : entityIdsToEdit)
                {
                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);

                    uint32_t propertiesUpdated = 0;
                    MaterialComponentRequestBus::EventResult(
                        propertiesUpdated, entityId, &MaterialComponentRequestBus::Events::RepairMaterialsWithRenamedProperties);
                    AZ_Printf("EditorMaterialComponent", "Updated %u property(s).", propertiesUpdated);

                    MaterialComponentNotificationBus::Event(entityId, &MaterialComponentNotifications::OnMaterialsEdited);
                }
                UpdateMaterialSlots();
            });
            action->setToolTip("Update material property overrides referencing names that have changed since they were set on the component.");
        }

        void EditorMaterialComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::SetMaterialAssetIdOnDefaultSlot, assetId);

            MaterialComponentNotificationBus::Event(GetEntityId(), &MaterialComponentNotifications::OnMaterialsEdited);

            InvalidatePropertyDisplay(AzToolsFramework::Refresh_AttributesAndValues);
        }

        void EditorMaterialComponent::OnMaterialsCreated(const MaterialAssignmentMap& materials)
        {
            // PSO-impacting property changes are allowed in the editor because the saved data can be analyzed to pre-compile the necessary PSOs.
            for (auto& materialAssignment : materials)
            {
                if (materialAssignment.second.m_materialInstance)
                {
                    materialAssignment.second.m_materialInstance->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);
                }
            }
        }

        void EditorMaterialComponent::OnEntityVisibilityChanged(bool visibility)
        {
            EditorRenderComponentAdapter::OnEntityVisibilityChanged(visibility);
            UpdateMaterialSlots();
        }

        AZ::u32 EditorMaterialComponent::OnConfigurationChanged()
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        void EditorMaterialComponent::OnMaterialSlotLayoutChanged()
        {
            UpdateMaterialSlots();
        }

        void EditorMaterialComponent::UpdateMaterialSlots()
        {
            SetDirty();
            m_defaultMaterialSlot = {};
            m_materialSlots = {};
            m_materialSlotsByLod = {};

            // Get the known material assignment slots from the associated model or other source
            MaterialAssignmentMap originalMaterials;
            MaterialComponentRequestBus::EventResult(
                originalMaterials, GetEntityId(), &MaterialComponentRequestBus::Events::GetDefaultMaterialMap);
                        
            // Generate the table of editable materials using the source data to define number of groups, elements, and initial values
            for (const auto& materialPair : originalMaterials)
            {
                // Setup the material slot entry
                EditorMaterialComponentSlot slot(GetEntityId(), materialPair.first);

                if (slot.m_id.IsDefault())
                {
                    m_defaultMaterialSlot = slot;
                    continue;
                }

                if (slot.m_id.IsSlotIdOnly())
                {
                    m_materialSlots.push_back(slot);
                    continue;
                }

                if (slot.m_id.IsLodAndSlotId())
                {
                    // Resize the containers to fit all elements
                    m_materialSlotsByLod.resize(
                        AZ::GetMax<size_t>(m_materialSlotsByLod.size(), aznumeric_cast<size_t>(slot.m_id.m_lodIndex + 1)));
                    m_materialSlotsByLod[slot.m_id.m_lodIndex].push_back(slot);
                    continue;
                }
            }

            // Sort all of the slots by label to ensure stable index values (originalMaterials is an unordered map)
            AZStd::sort(m_materialSlots.begin(), m_materialSlots.end(),
                [](const auto& a, const auto& b) { return a.GetLabel() < b.GetLabel(); });

            for (auto& lodSlots : m_materialSlotsByLod)
            {
                AZStd::sort(lodSlots.begin(), lodSlots.end(),
                    [](const auto& a, const auto& b) { return a.GetLabel() < b.GetLabel(); });
            }

            MaterialComponentNotificationBus::Event(GetEntityId(), &MaterialComponentNotifications::OnMaterialsEdited);

            InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
        }

        AZ::u32 EditorMaterialComponent::OpenMaterialExporterFromRPE()
        {
            return OpenMaterialExporter(EditorMaterialComponentUtil::GetEntitiesMatchingMaterialSlots(
                GetEntityId(), EditorMaterialComponentUtil::GetSelectedEntitiesFromActiveInspector()));
        }

        AZ::u32 EditorMaterialComponent::OpenMaterialExporter(const AzToolsFramework::EntityIdSet& entityIdsToEdit)
        {
            MaterialAssignmentMap originalMaterials;
            MaterialComponentRequestBus::EventResult(
                originalMaterials, GetEntityId(), &MaterialComponentRequestBus::Events::GetDefaultMaterialMap);

            // Generate a unique set of all material asset IDs that will be used for source data generation
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::string> assetIdToSlotNameMap;
            for (const auto& materialPair : originalMaterials)
            {
                const Data::AssetId originalAssetId = materialPair.second.m_materialAsset.GetId();
                if (originalAssetId.IsValid())
                {
                    MaterialComponentRequestBus::EventResult(
                        assetIdToSlotNameMap[originalAssetId], GetEntityId(), &MaterialComponentRequestBus::Events::GetMaterialLabel,
                        materialPair.first);
                }
            }

            // Convert the unique set of asset IDs into export items that can be configured in the dialog
            // The order should not matter because the table in the dialog can sort itself for a specific row
            EditorMaterialComponentExporter::ExportItemsContainer exportItems;
            exportItems.reserve(assetIdToSlotNameMap.size());

            for (const auto& [assetId, slotName] : assetIdToSlotNameMap)
            {
                exportItems.emplace_back(assetId, slotName);
            }

            // Display the export dialog so that the user can configure how they want different materials to be exported
            if (EditorMaterialComponentExporter::OpenExportDialog(exportItems))
            {
                AzToolsFramework::ScopedUndoBatch undoBatch("Generating materials.");

                // Create progress dialog to report the status of each material being generated.
                EditorMaterialComponentExporter::ProgressDialog progressDialog("Generating materials", "Generating material...", aznumeric_cast<int>(exportItems.size()));

                for (const EditorMaterialComponentExporter::ExportItem& exportItem : exportItems)
                {
                    // Creating material source data from a product asset and resaving it as a new source material.
                    if (!EditorMaterialComponentExporter::ExportMaterialSourceData(exportItem))
                    {
                        // This file was skipped because it was either marked to not be exported, not be overwritten, or another error occurred.
                        progressDialog.CompleteItem();
                        continue;
                    }

                    // After saving the source file, wait for it to be added to the catalog and processed by the AP so that a valid asset
                    // can be assigned to the material component without spamming warning messages.
                    const AZ::Data::AssetInfo assetInfo = progressDialog.ProcessItem(exportItem);

                    if (!assetInfo.m_assetId.IsValid())
                    {
                        UpdateMaterialSlots();
                        return AZ::Edit::PropertyRefreshLevels::EntireTree;
                    }

                    // Valid asset info has been found for the file that was just saved so it can be assigned to the material component.
                    for (const auto& materialPair : originalMaterials)
                    {
                        // We need to check if replaced material corresponds to this slot's default material.
                        const Data::AssetId originalAssetId = materialPair.second.m_materialAsset.GetId();
                        if (originalAssetId == exportItem.GetOriginalAssetId())
                        {
                            if (m_materialSlotsByLodEnabled || !materialPair.first.IsLodAndSlotId())
                            {
                                for (const AZ::EntityId& entityId : entityIdsToEdit)
                                {
                                    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
                                        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, entityId);

                                    MaterialComponentRequestBus::Event(
                                        entityId,
                                        &MaterialComponentRequestBus::Events::SetMaterialAssetId,
                                        materialPair.first,
                                        assetInfo.m_assetId);
                                }
                            }
                        }
                    }

                    // Increment and update the progress dialog
                    progressDialog.CompleteItem();
                }
            }

            UpdateMaterialSlots();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 EditorMaterialComponent::OnLodsToggled()
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Toggling LOD materials.");
            SetDirty();

            if (!m_materialSlotsByLodEnabled)
            {
                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearMaterialsOnLodSlots);
            }

            UpdateMaterialSlots();

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::Crc32 EditorMaterialComponent::GetLodVisibility() const
        {
            return m_materialSlotsByLodEnabled ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZStd::string EditorMaterialComponent::GetLabelForLod(int lodIndex) const
        {
            return AZStd::string::format("LOD %d", lodIndex);
        }
    } // namespace Render
} // namespace AZ
