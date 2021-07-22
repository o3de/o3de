/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/EditorMaterialComponent.h>
#include <Material/EditorMaterialComponentExporter.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyId.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QMenu>
#include <QAction>
#include <QCursor>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace Render
    {
        const char* EditorMaterialComponent::GenerateMaterialsButtonText = "Generate/Manage Source Materials...";
        const char* EditorMaterialComponent::GenerateMaterialsToolTipText = "Generate editable source material files from materials provided by the model.";

        const char* EditorMaterialComponent::ResetMaterialsButtonText = "Reset Materials";
        const char* EditorMaterialComponent::ResetMaterialsToolTipText = "Clear all settings, materials, and properties then rebuild material slots from the associated model.";

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

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
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

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMaterialComponent>(
                        "Material", "The material component specifies the material to use for this entity")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/")
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

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
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
            UpdateMaterialSlots();
        }

        void EditorMaterialComponent::Deactivate()
        {
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

            action = menu->addAction(ResetMaterialsButtonText, [this]() { ResetMaterialSlots(); });
            action->setToolTip(ResetMaterialsToolTipText);

            menu->addSeparator();

            action = menu->addAction("Clear Model Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing model materials.");
                SetDirty();

                for (auto& materialSlotPair : GetMaterialSlots())
                {
                    EditorMaterialComponentSlot* materialSlot = materialSlotPair.second;
                    if (materialSlot->m_id.IsSlotIdOnly())
                    {
                        materialSlot->Clear();
                    }
                }
                });
            action = menu->addAction("Clear LOD Materials", [this]() {
                AzToolsFramework::ScopedUndoBatch undoBatch("Clearing LOD materials.");
                SetDirty();

                for (auto& materialSlotPair : GetMaterialSlots())
                {
                    EditorMaterialComponentSlot* materialSlot = materialSlotPair.second;
                    if (materialSlot->m_id.IsLodAndSlotId())
                    {
                        materialSlot->Clear();
                    }
                }
                });
            action->setEnabled(m_materialSlotsByLodEnabled);
        }

        void EditorMaterialComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            m_controller.SetDefaultMaterialOverride(assetId);
        }

        AZ::u32 EditorMaterialComponent::OnConfigurationChanged()
        {
            // Whenever the user makes changes to the editor component data the controller configuration must be rebuilt
            m_configurationChangeInProgress = true;
            UpdateController();
            m_configurationChangeInProgress = false;

            return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        void EditorMaterialComponent::OnMaterialAssignmentsChanged()
        {
            // [GFX TODO][ATOM-4604] remove flag after mesh component material handling is fixed to not recreate/reload mesh for material changes
            if (!m_configurationChangeInProgress)
            {
                UpdateMaterialSlots();
            }
        }

        void EditorMaterialComponent::OnMaterialsEdited(const MaterialAssignmentMap& materials)
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Materials edited.");
            SetDirty();

            // The layout of the materials slots is already set.
            // We just need to read the values from any edited overrides into the editor component
            // and refresh.
            for (auto& materialSlotPair : GetMaterialSlots())
            {
                EditorMaterialComponentSlot& slot = *materialSlotPair.second;
                const MaterialAssignment& materialFromController = GetMaterialAssignmentFromMap(materials, slot.m_id);
                slot.m_materialAsset = materialFromController.m_materialAsset;
                slot.m_propertyOverrides = materialFromController.m_propertyOverrides;
                slot.m_matModUvOverrides = materialFromController.m_matModUvOverrides;
            }

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_AttributesAndValues);
        }

        void EditorMaterialComponent::UpdateConfiguration(const MaterialComponentConfig& config)
        {
            m_controller.SetMaterialOverrides(config.m_materials);
        }

        void EditorMaterialComponent::UpdateController()
        {
            SetDirty();

            // Build the controller configuration from the editor configuration
            MaterialComponentConfig config = m_controller.GetConfiguration();
            config.m_materials.clear();
            
            for (const auto& materialSlotPair : GetMaterialSlots())
            {
                const EditorMaterialComponentSlot* materialSlot = materialSlotPair.second;

                // Do not apply materials for lods if they are disabled
                if (materialSlot->m_id.m_lodIndex != MaterialAssignmentId::NonLodIndex && !m_materialSlotsByLodEnabled)
                {
                    continue;
                }

                // Only material slots with a valid asset IDs or property overrides will be copied
                // to minimize the amount of data stored in the controller and game component
                if (materialSlot->m_materialAsset.GetId().IsValid())
                {
                    MaterialAssignment& materialAssignment = config.m_materials[materialSlot->m_id];
                    materialAssignment.m_materialAsset = materialSlot->m_materialAsset;
                    materialAssignment.m_propertyOverrides = materialSlot->m_propertyOverrides;
                    materialAssignment.m_matModUvOverrides = materialSlot->m_matModUvOverrides;
                }
                else if (!materialSlot->m_propertyOverrides.empty() || !materialSlot->m_matModUvOverrides.empty())
                {
                    MaterialAssignment& materialAssignment = config.m_materials[materialSlot->m_id];
                    materialAssignment.m_materialAsset = materialSlot->m_defaultMaterialAsset;
                    materialAssignment.m_propertyOverrides = materialSlot->m_propertyOverrides;
                    materialAssignment.m_matModUvOverrides = materialSlot->m_matModUvOverrides;
                }
            }

            UpdateConfiguration(config);
        }

        void EditorMaterialComponent::UpdateMaterialSlots()
        {
            SetDirty();
            m_defaultMaterialSlot = {};
            m_materialSlots = {};
            m_materialSlotsByLod = {};

            const MaterialComponentConfig& config = m_controller.GetConfiguration();

            // Get the known material assignment slots from the associated model or other source
            MaterialAssignmentMap materialsFromSource;
            MaterialReceiverRequestBus::EventResult(materialsFromSource, GetEntityId(), &MaterialReceiverRequestBus::Events::GetMaterialAssignments);
                        
            RPI::ModelMaterialSlotMap modelMaterialSlots;
            MaterialReceiverRequestBus::EventResult(modelMaterialSlots, GetEntityId(), &MaterialReceiverRequestBus::Events::GetModelMaterialSlots);

            // Generate the table of editable materials using the source data to define number of groups, elements, and initial values
            for (const auto& materialPair : materialsFromSource)
            {
                // Setup the material slot entry
                EditorMaterialComponentSlot slot;
                slot.m_id = materialPair.first;
                slot.m_materialChangedCallback = [this]() {
                    // This callback is triggered whenever an individual material slot changes outside of normal inspector interactions
                    // So we must manually handle undo, update configuration, and refresh the inspector to display the new values
                    AzToolsFramework::ScopedUndoBatch undoBatch("Material slot changed.");
                    SetDirty();

                    OnConfigurationChanged();

                    AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                        &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                        AzToolsFramework::Refresh_AttributesAndValues);
                };
                slot.m_propertyChangedCallback = [this]() {
                    OnConfigurationChanged();
                };

                const char* UnknownSlotName = "<unknown>";

                // If this is the default material assignment ID then it represents the default slot which is not contained in any other group
                if (slot.m_id == DefaultMaterialAssignmentId)
                {
                    slot.m_label = "Default Material";
                }
                else
                {
                    auto slotIter = modelMaterialSlots.find(slot.m_id.m_materialSlotStableId);
                    if (slotIter != modelMaterialSlots.end())
                    {
                        const Name& displayName = slotIter->second.m_displayName;
                        slot.m_label = !displayName.IsEmpty() ? displayName.GetStringView() : UnknownSlotName;

                        slot.m_defaultMaterialAsset = slotIter->second.m_defaultMaterialAsset;
                    }
                    else
                    {
                        slot.m_label = UnknownSlotName;
                    }
                }

                // if material is present in controller configuration, assign its data
                const MaterialAssignment& materialFromController = GetMaterialAssignmentFromMap(config.m_materials, slot.m_id);
                if (materialFromController.m_materialAsset != slot.m_defaultMaterialAsset) // Prevents the default material from showing up as a filled-in value in the property field
                {
                    slot.m_materialAsset = materialFromController.m_materialAsset;
                }

                slot.m_propertyOverrides = materialFromController.m_propertyOverrides;
                slot.m_matModUvOverrides = materialFromController.m_matModUvOverrides;

                // Attempt to get the UV names from model meshes.
                MaterialReceiverRequestBus::EventResult(slot.m_modelUvNames, GetEntityId(), &MaterialReceiverRequestBus::Events::GetModelUvNames);

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
                    m_materialSlotsByLod.resize(AZ::GetMax<size_t>(m_materialSlotsByLod.size(), aznumeric_cast<size_t>(slot.m_id.m_lodIndex + 1)));
                    m_materialSlotsByLod[slot.m_id.m_lodIndex].push_back(slot);
                    continue;
                }
            }

            // Sort all of the slots by label to ensure stable index values (materialsFromSource is an unordered map)
            AZStd::sort(m_materialSlots.begin(), m_materialSlots.end(),
                [](const auto& a, const auto& b) { return a.GetLabel() < b.GetLabel(); });

            for (auto& lodSlots : m_materialSlotsByLod)
            {
                AZStd::sort(lodSlots.begin(), lodSlots.end(),
                    [](const auto& a, const auto& b) { return a.GetLabel() < b.GetLabel(); });
            }

            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_EntireTree);
        }

        AZ::u32 EditorMaterialComponent::ResetMaterialSlots()
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Resetting materials.");
            SetDirty();

            UpdateConfiguration(MaterialComponentConfig());
            UpdateMaterialSlots();

            m_materialSlotsByLodEnabled = false;

            // Forcing refresh in case triggered from context menu action
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_EntireTree);

            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 EditorMaterialComponent::OpenMaterialExporter()
        {
            AzToolsFramework::ScopedUndoBatch undoBatch("Generating materials.");
            SetDirty();
            
            Data::AssetId modelAssetId;
            MeshComponentRequestBus::EventResult(modelAssetId, GetEntityId(), &MeshComponentRequestBus::Events::GetModelAssetId);

            // First generating a unique set of all material asset IDs that will be used for source data generation
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::string /*slot name*/> assetIdMap;

            auto materialSlots = GetMaterialSlots();
            for (auto& materialSlotPair : materialSlots)
            {
                Data::AssetId defaultMaterialAssetId = materialSlotPair.second->m_defaultMaterialAsset.GetId();
                if (defaultMaterialAssetId.IsValid())
                {
                    assetIdMap[defaultMaterialAssetId] = materialSlotPair.second->GetLabel();
                }
            }

            // Convert the unique set of asset IDs into export items that can be configured in the dialog 
            // The order should not matter because the table in the dialog can sort itself for a specific row
            EditorMaterialComponentExporter::ExportItemsContainer exportItems;
            for (auto assetIdInfo : assetIdMap)
            {
                EditorMaterialComponentExporter::ExportItem exportItem{assetIdInfo.first, assetIdInfo.second};
                exportItems.push_back(exportItem);
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
                        for (auto& materialSlotPair : materialSlots)
                        {
                            EditorMaterialComponentSlot* editorMaterialSlot = materialSlotPair.second;

                            if (editorMaterialSlot)
                            {
                                // We need to check whether replaced material corresponds to this slot's default material.
                                if (editorMaterialSlot->m_defaultMaterialAsset.GetId() == exportItem.GetOriginalAssetId())  
                                {
                                    editorMaterialSlot->m_materialAsset.Create(assetIdOutcome.GetValue());
                                }
                            }
                        }
                    }
                }
            }

            // Forcing refresh in case triggered from context menu action
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_AttributesAndValues);

            return OnConfigurationChanged();
        }

        AZ::u32 EditorMaterialComponent::OnLodsToggled()
        {
            OnConfigurationChanged();
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

        template<typename ComponentType, typename ContainerType>
        void EditorMaterialComponent::BuildMaterialSlotMap(ComponentType& component, ContainerType& materialSlots)
        {
            materialSlots[DefaultMaterialAssignmentId] = &component.m_defaultMaterialSlot;

            for (auto& slot : component.m_materialSlots)
            {
                materialSlots[slot.m_id] = &slot;
            }

            for (auto& slotsForLod : component.m_materialSlotsByLod)
            {
                for (auto& slot : slotsForLod)
                {
                    materialSlots[slot.m_id] = &slot;
                }
            }
        }

        AZStd::unordered_map<MaterialAssignmentId, EditorMaterialComponentSlot*> EditorMaterialComponent::GetMaterialSlots()
        {
            AZStd::unordered_map<MaterialAssignmentId, EditorMaterialComponentSlot*> materialSlots;
            BuildMaterialSlotMap(*this, materialSlots);
            return AZStd::move(materialSlots);
        }

        AZStd::unordered_map<MaterialAssignmentId, const EditorMaterialComponentSlot*> EditorMaterialComponent::GetMaterialSlots() const
        {
            AZStd::unordered_map<MaterialAssignmentId, const EditorMaterialComponentSlot*> materialSlots;
            BuildMaterialSlotMap(*this, materialSlots);
            return AZStd::move(materialSlots);
        }
    } // namespace Render
} // namespace AZ

