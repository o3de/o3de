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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Material/EditorMaterialComponent.h>
#include <Material/EditorMaterialComponentExporter.h>
#include <Material/EditorMaterialComponentSerializer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAction>
#include <QCursor>
#include <QMenu>
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

                serializeContext->Class<EditorMaterialComponent, BaseClass>()
                    ->Version(5, &EditorMaterialComponent::ConvertVersion)
                    ->Field("message", &EditorMaterialComponent::m_message)
                    ->Field("defaultMaterialSlot", &EditorMaterialComponent::m_defaultMaterialSlot)
                    ->Field("materialSlots", &EditorMaterialComponent::m_materialSlots)
                    ->Field("materialSlotsByLodEnabled", &EditorMaterialComponent::m_materialSlotsByLodEnabled)
                    ->Field("materialSlotsByLod", &EditorMaterialComponent::m_materialSlotsByLod)
                    ;

                serializeContext->RegisterGenericType<AZStd::unordered_map<MaterialAssignmentId, Data::AssetId, AZStd::hash<MaterialAssignmentId>, AZStd::equal_to<MaterialAssignmentId>, AZStd::allocator>>();
                serializeContext->RegisterGenericType<AZStd::unordered_map<MaterialAssignmentId, MaterialPropertyOverrideMap, AZStd::hash<MaterialAssignmentId>, AZStd::equal_to<MaterialAssignmentId>, AZStd::allocator>>();

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMaterialComponent>(
                        "Material", "The material component specifies the material to use for this entity")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/material/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::MaterialAsset>::Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::MultiLineEdit, &EditorMaterialComponent::m_message, "Message", "")
                            ->Attribute(AZ_CRC("PlaceholderText", 0xa23ec278), "Component cannot be edited with multiple entities selected")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMaterialComponent::GetMessageVisibility)
                            ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
                        ->UIElement(AZ::Edit::UIHandlers::Button, GenerateMaterialsButtonText, GenerateMaterialsToolTipText)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, GenerateMaterialsButtonText)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OpenMaterialExporter)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMaterialComponent::GetEditorVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_defaultMaterialSlot, "Default Material", "Materials assigned to this slot will be applied to the entire model unless specific model or LOD material overrides are set.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMaterialComponent::GetDefaultMaterialVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnConfigurationChanged)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_materialSlots, "Model Materials", "Materials assigned to these slots will be applied to every part of the model with same material slot name unless an overriding LOD material is specified.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnConfigurationChanged)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMaterialComponent::GetEditorVisibility)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMaterialComponent::m_materialSlotsByLodEnabled, "Enable LOD Materials", "When this flag is enabled, materials can be specified per LOD.")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMaterialComponent::OnLodsToggled)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMaterialComponent::GetEditorVisibility)
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
            MaterialReceiverNotificationBus::Handler::BusConnect(GetEntityId());
            MaterialComponentNotificationBus::Handler::BusConnect(GetEntityId());
            EditorMaterialSystemComponentNotificationBus::Handler::BusConnect();
            UpdateMaterialSlots();
        }

        void EditorMaterialComponent::Deactivate()
        {
            EditorMaterialSystemComponentNotificationBus::Handler::BusDisconnect();
            MaterialReceiverNotificationBus::Handler::BusDisconnect();
            MaterialComponentNotificationBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorMaterialComponent::AddContextMenuActions(QMenu* menu)
        {
            // Don't add menu options if more than one entity is selected
            if (!IsEditingAllowed())
            {
                return;
            }

            QAction* action = nullptr;

            menu->addSeparator();

            action = menu->addAction(GenerateMaterialsButtonText, [this]() { OpenMaterialExporter(); });
            action->setToolTip(GenerateMaterialsToolTipText);

            menu->addSeparator();

            action = menu->addAction("Clear All Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing all materials.");
                SetDirty();

                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearAllMaterialOverrides);

                m_materialSlotsByLodEnabled = false;

                UpdateMaterialSlots();
            });
            action->setToolTip("Clear all materials and properties then rebuild material slots from the associated model.");

            action = menu->addAction("Clear Model Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing model materials.");
                SetDirty();

                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearModelMaterialOverrides);

                UpdateMaterialSlots();
            });
            action->setToolTip("Clear model materials and properties then rebuild material slots from the associated model.");

            action = menu->addAction("Clear LOD Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing LOD materials.");
                SetDirty();

                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearLodMaterialOverrides);

                m_materialSlotsByLodEnabled = false;

                UpdateMaterialSlots();
            });
            action->setToolTip("Clear LOD materials and properties then rebuild material slots from the associated model.");

            action = menu->addAction("Clear Incompatible Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing incompatible materials.");
                SetDirty();

                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearIncompatibleMaterialOverrides);

                UpdateMaterialSlots();
            });
            action->setToolTip("Clear residual materials that don't correspond to the associated model.");

            action = menu->addAction("Clear Invalid Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing invalid materials.");
                SetDirty();

                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearInvalidMaterialOverrides);

                UpdateMaterialSlots();
            });
            action->setToolTip("Clear materials that reference missing assets.");

            action = menu->addAction("Repair Invalid Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Repairing invalid materials.");
                SetDirty();

                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::RepairInvalidMaterialOverrides);

                UpdateMaterialSlots();
            });
            action->setToolTip("Repair materials that reference missing assets by assigning the default asset.");
            
            action = menu->addAction("Apply Automatic Property Updates", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Applying automatic property updates.");
                SetDirty();

                uint32_t propertiesUpdated = 0;
                MaterialComponentRequestBus::EventResult(propertiesUpdated, GetEntityId(), &MaterialComponentRequestBus::Events::ApplyAutomaticPropertyUpdates);

                AZ_Printf("EditorMaterialComponent", "Updated %u property(s).", propertiesUpdated);

                UpdateMaterialSlots();
            });
            action->setToolTip("Repair material property overrides that reference missing properties by auto-renaming them where possible.");
        }

        void EditorMaterialComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::SetDefaultMaterialOverride, assetId);

            MaterialComponentNotificationBus::Event(GetEntityId(), &MaterialComponentNotifications::OnMaterialsEdited);

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
        }

        void EditorMaterialComponent::OnMaterialInstanceCreated(const MaterialAssignment& materialAssignment)
        {
            // PSO-impacting property changes are allowed in the editor
            // because the saved slice data can be analyzed to pre-compile the necessary PSOs.
            if (materialAssignment.m_materialInstance)
            {
                materialAssignment.m_materialInstance->SetPsoHandlingOverride(AZ::RPI::MaterialPropertyPsoHandling::Allowed);
            }
        }

        void EditorMaterialComponent::OnRenderMaterialPreviewComplete(
            [[maybe_unused]] const AZ::EntityId& entityId,
            [[maybe_unused]] const AZ::Render::MaterialAssignmentId& materialAssignmentId,
            [[maybe_unused]] const QPixmap& pixmap)
        {
            if (entityId == GetEntityId())
            {
                AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                    &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
            }
        }

        AZ::u32 EditorMaterialComponent::OnConfigurationChanged()
        {
            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        void EditorMaterialComponent::OnMaterialAssignmentsChanged()
        {
            UpdateMaterialSlots();
        }

        void EditorMaterialComponent::UpdateMaterialSlots()
        {
            SetDirty();
            m_defaultMaterialSlot = {};
            m_materialSlots = {};
            m_materialSlotsByLod = {};

            // Get current material assignments
            MaterialAssignmentMap currentMaterials;
            MaterialComponentRequestBus::EventResult(
                currentMaterials, GetEntityId(), &MaterialComponentRequestBus::Events::GetMaterialOverrides);

            // Get the known material assignment slots from the associated model or other source
            MaterialAssignmentMap originalMaterials;
            MaterialComponentRequestBus::EventResult(
                originalMaterials, GetEntityId(), &MaterialComponentRequestBus::Events::GetOriginalMaterialAssignments);
                        
            // Generate the table of editable materials using the source data to define number of groups, elements, and initial values
            for (const auto& materialPair : originalMaterials)
            {
                // Setup the material slot entry
                EditorMaterialComponentSlot slot;
                slot.m_entityId = GetEntityId();
                slot.m_id = materialPair.first;

                // if material is present in controller configuration, assign its data
                const MaterialAssignment& materialFromController = GetMaterialAssignmentFromMap(currentMaterials, slot.m_id);
                slot.m_materialAsset = materialFromController.m_materialAsset;

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

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
        }

        AZ::u32 EditorMaterialComponent::OpenMaterialExporter()
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Generating materials.");
            SetDirty();

            MaterialAssignmentMap originalMaterials;
            MaterialComponentRequestBus::EventResult(
                originalMaterials, GetEntityId(), &MaterialComponentRequestBus::Events::GetOriginalMaterialAssignments);

            // Generate a unique set of all material asset IDs that will be used for source data generation
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::string> assetIdToSlotNameMap;
            for (const auto& materialPair : originalMaterials)
            {
                const Data::AssetId originalAssetId = materialPair.second.m_materialAsset.GetId();
                if (originalAssetId.IsValid())
                {
                    MaterialComponentRequestBus::EventResult(
                        assetIdToSlotNameMap[originalAssetId], GetEntityId(), &MaterialComponentRequestBus::Events::GetMaterialSlotLabel,
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
                for (const EditorMaterialComponentExporter::ExportItem& exportItem : exportItems)
                {
                    if (!EditorMaterialComponentExporter::ExportMaterialSourceData(exportItem))
                    {
                        continue;
                    }

                    const auto& assetIdOutcome = AZ::RPI::AssetUtils::MakeAssetId(exportItem.GetExportPath(), 0);
                    if (assetIdOutcome)
                    {
                        for (const auto& materialPair : originalMaterials)
                        {
                            // We need to check whether replaced material corresponds to this slot's default material.
                            const Data::AssetId originalAssetId = materialPair.second.m_materialAsset.GetId();
                            if (originalAssetId == exportItem.GetOriginalAssetId())
                            {
                                if (m_materialSlotsByLodEnabled || !materialPair.first.IsLodAndSlotId())
                                {
                                    MaterialComponentRequestBus::Event(
                                        GetEntityId(), &MaterialComponentRequestBus::Events::SetMaterialOverride, materialPair.first,
                                        assetIdOutcome.GetValue());
                                }
                            }
                        }
                    }
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
                MaterialComponentRequestBus::Event(GetEntityId(), &MaterialComponentRequestBus::Events::ClearLodMaterialOverrides);
            }

            UpdateMaterialSlots();

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::Crc32 EditorMaterialComponent::GetLodVisibility() const
        {
            return IsEditingAllowed() && m_materialSlotsByLodEnabled ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::Crc32 EditorMaterialComponent::GetDefaultMaterialVisibility() const
        {
            return IsEditingAllowed() ? AZ::Edit::PropertyVisibility::ShowChildrenOnly : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::Crc32 EditorMaterialComponent::GetEditorVisibility() const
        {
            return IsEditingAllowed() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }

        AZ::Crc32 EditorMaterialComponent::GetMessageVisibility() const
        {
            return IsEditingAllowed() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
        }

        bool EditorMaterialComponent::IsEditingAllowed() const
        {
            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);
            return selectedEntities.size() == 1;
        }

        AZStd::string EditorMaterialComponent::GetLabelForLod(int lodIndex) const
        {
            return AZStd::string::format("LOD %d", lodIndex);
        }
    } // namespace Render
} // namespace AZ
