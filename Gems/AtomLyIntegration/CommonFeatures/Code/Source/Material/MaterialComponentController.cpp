/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Image/AttachmentImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Material/MaterialComponentController.h>

namespace AZ
{
    namespace Render
    {
        void MaterialComponentController::Reflect(ReflectContext* context)
        {
            MaterialComponentConfig::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &MaterialComponentController::m_configuration)
                    ;
            }
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<MaterialComponentRequestBus>("MaterialComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("GetOriginalMaterialAssignments", &MaterialComponentRequestBus::Events::GetOriginalMaterialAssignments)
                    ->Event("FindMaterialAssignmentId", &MaterialComponentRequestBus::Events::FindMaterialAssignmentId)
                    ->Event("GetActiveMaterialAssetId", &MaterialComponentRequestBus::Events::GetActiveMaterialAssetId)
                    ->Event("GetDefaultMaterialAssetId", &MaterialComponentRequestBus::Events::GetDefaultMaterialAssetId)
                    ->Event("GetMaterialSlotLabel", &MaterialComponentRequestBus::Events::GetMaterialSlotLabel)
                    ->Event("SetMaterialOverrides", &MaterialComponentRequestBus::Events::SetMaterialOverrides)
                    ->Event("GetMaterialOverrides", &MaterialComponentRequestBus::Events::GetMaterialOverrides)
                    ->Event("ClearAllMaterialOverrides", &MaterialComponentRequestBus::Events::ClearAllMaterialOverrides)
                    ->Event("SetDefaultMaterialOverride", &MaterialComponentRequestBus::Events::SetDefaultMaterialOverride)
                    ->Event("GetDefaultMaterialOverride", &MaterialComponentRequestBus::Events::GetDefaultMaterialOverride)
                    ->Event("ClearDefaultMaterialOverride", &MaterialComponentRequestBus::Events::ClearDefaultMaterialOverride)
                    ->Event("ClearModelMaterialOverrides", &MaterialComponentRequestBus::Events::ClearModelMaterialOverrides)
                    ->Event("ClearLodMaterialOverrides", &MaterialComponentRequestBus::Events::ClearLodMaterialOverrides)
                    ->Event("ClearIncompatibleMaterialOverrides", &MaterialComponentRequestBus::Events::ClearIncompatibleMaterialOverrides)
                    ->Event("ClearInvalidMaterialOverrides", &MaterialComponentRequestBus::Events::ClearInvalidMaterialOverrides)
                    ->Event("RepairInvalidMaterialOverrides", &MaterialComponentRequestBus::Events::RepairInvalidMaterialOverrides)
                    ->Event("ApplyAutomaticPropertyUpdates", &MaterialComponentRequestBus::Events::ApplyAutomaticPropertyUpdates)
                    ->Event("SetMaterialOverride", &MaterialComponentRequestBus::Events::SetMaterialOverride)
                    ->Event("GetMaterialOverride", &MaterialComponentRequestBus::Events::GetMaterialOverride)
                    ->Event("ClearMaterialOverride", &MaterialComponentRequestBus::Events::ClearMaterialOverride)
                    ->Event("SetPropertyOverride", &MaterialComponentRequestBus::Events::SetPropertyOverride)
                    ->Event("SetPropertyOverrideBool", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<bool>)
                    ->Event("SetPropertyOverrideInt32", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<int32_t>)
                    ->Event("SetPropertyOverrideUInt32", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<uint32_t>)
                    ->Event("SetPropertyOverrideFloat", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<float>)
                    ->Event("SetPropertyOverrideVector2", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<AZ::Vector2>)
                    ->Event("SetPropertyOverrideVector3", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<AZ::Vector3>)
                    ->Event("SetPropertyOverrideVector4", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<AZ::Vector4>)
                    ->Event("SetPropertyOverrideColor", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<AZ::Color>)
                    ->Event("SetPropertyOverrideImage", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<AZ::Data::AssetId>)
                    ->Event("SetPropertyOverrideString", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<AZStd::string>)
                    ->Event("SetPropertyOverrideEnum", &MaterialComponentRequestBus::Events::SetPropertyOverrideT<uint32_t>)
                    ->Event("GetPropertyOverride", &MaterialComponentRequestBus::Events::GetPropertyOverride)
                    ->Event("GetPropertyOverrideBool", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<bool>)
                    ->Event("GetPropertyOverrideInt32", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<int32_t>)
                    ->Event("GetPropertyOverrideUInt32", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<uint32_t>)
                    ->Event("GetPropertyOverrideFloat", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<float>)
                    ->Event("GetPropertyOverrideVector2", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<AZ::Vector2>)
                    ->Event("GetPropertyOverrideVector3", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<AZ::Vector3>)
                    ->Event("GetPropertyOverrideVector4", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<AZ::Vector4>)
                    ->Event("GetPropertyOverrideColor", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<AZ::Color>)
                    ->Event("GetPropertyOverrideImage", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<AZ::Data::AssetId>)
                    ->Event("GetPropertyOverrideString", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<AZStd::string>)
                    ->Event("GetPropertyOverrideEnum", &MaterialComponentRequestBus::Events::GetPropertyOverrideT<uint32_t>)
                    ->Event("ClearPropertyOverride", &MaterialComponentRequestBus::Events::ClearPropertyOverride)
                    ->Event("ClearPropertyOverrides", &MaterialComponentRequestBus::Events::ClearPropertyOverrides)
                    ->Event("ClearAllPropertyOverrides", &MaterialComponentRequestBus::Events::ClearAllPropertyOverrides)
                    ->Event("SetPropertyOverrides", &MaterialComponentRequestBus::Events::SetPropertyOverrides)
                    ->Event("GetPropertyOverrides", &MaterialComponentRequestBus::Events::GetPropertyOverrides)
                    ;
            }
        }

        void MaterialComponentController::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("MaterialProviderService"));
        }

        void MaterialComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("MaterialProviderService"));
        }

        void MaterialComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("MaterialReceiverService"));
        }

        MaterialComponentController::MaterialComponentController(const MaterialComponentConfig& config)
            : m_configuration(config)
        {
            ConvertAssetsForSerialization();
        }

        void MaterialComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_queuedMaterialUpdateNotification = false;

            MaterialComponentRequestBus::Handler::BusConnect(m_entityId);
            MaterialReceiverNotificationBus::Handler::BusConnect(m_entityId);
            LoadMaterials();
        }

        void MaterialComponentController::Deactivate()
        {
            MaterialComponentRequestBus::Handler::BusDisconnect();
            MaterialReceiverNotificationBus::Handler::BusDisconnect();

            ReleaseMaterials();

            // Sending notification to wipe any previously assigned material overrides
            MaterialComponentNotificationBus::Event(
                m_entityId, &MaterialComponentNotifications::OnMaterialsUpdated, MaterialAssignmentMap());

            m_entityId = AZ::EntityId(AZ::EntityId::InvalidEntityId);
        }

        void MaterialComponentController::SetConfiguration(const MaterialComponentConfig& config)
        {
            m_configuration = config;
            ConvertAssetsForSerialization();
        }

        const MaterialComponentConfig& MaterialComponentController::GetConfiguration() const
        {
            return m_configuration;
        }

        void MaterialComponentController::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            InitializeMaterialInstance(asset);
        }

        void MaterialComponentController::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            InitializeMaterialInstance(asset);
        }

        void MaterialComponentController::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (m_queuedLoadMaterials)
            {
                LoadMaterials();
                m_queuedLoadMaterials = false;
            }

            AZStd::unordered_set<MaterialAssignmentId> materialsWithDirtyProperties;
            AZStd::swap(m_materialsWithDirtyProperties, materialsWithDirtyProperties);

            // Iterate through all MaterialAssignmentId's that have property overrides and attempt to apply them
            for (const auto& materialAssignmentId : materialsWithDirtyProperties)
            {
                const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
                if (materialIt != m_configuration.m_materials.end())
                {
                    if (!materialIt->second.ApplyProperties())
                    {
                        // If the material had properties to apply and it could not be compiled then queue it again
                        m_materialsWithDirtyProperties.emplace(materialAssignmentId);
                    }
                }
            }

            // Only disconnect from tick bus and send notification after all pending properties have been applied
            if (m_materialsWithDirtyProperties.empty())
            {
                if (m_queuedMaterialUpdateNotification)
                {
                    // Materials have been edited and instances have changed but the notification will only be sent once per tick
                    m_queuedMaterialUpdateNotification = false;
                    MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialsUpdated, m_configuration.m_materials);
                }

                TickBus::Handler::BusDisconnect();
            }
        }

        void MaterialComponentController::LoadMaterials()
        {
            // Caching previously loaded unique materials to avoid unloading and reloading assets that have not changed
            const auto uniqueMaterialMapBeforeLoad = m_uniqueMaterialMap;

            // Clear any previously loaded or queued material assets, instances, or notifications
            ReleaseMaterials();

            // Build tables of all referenced materials so that we can load and look up defaults
            for (const auto& [materialAssignmentId, materialAssignment] : GetOriginalMaterialAssignments())
            {
                const auto& defaultMaterialAsset = materialAssignment.m_materialAsset;
                m_uniqueMaterialMap[defaultMaterialAsset.GetId()] = defaultMaterialAsset;

                // Insert entry for quick look up of the default material asset
                m_defaultMaterialMap[materialAssignmentId] = defaultMaterialAsset;

                auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
                if (materialIt != m_configuration.m_materials.end())
                {
                    const auto& overrideMaterialAsset = materialIt->second.m_materialAsset;
                    m_uniqueMaterialMap[overrideMaterialAsset.GetId()] = overrideMaterialAsset;

                    materialIt->second.m_defaultMaterialAsset = defaultMaterialAsset;
                }
            }

            // Begin loading all unique, referenced material assets
            bool anyQueued = false;
            for (auto& [assetId, uniqueMaterial] : m_uniqueMaterialMap)
            {
                if (uniqueMaterial.GetId().IsValid())
                {
                    anyQueued = true;
                    uniqueMaterial.QueueLoad();
                    AZ::Data::AssetBus::MultiHandler::BusConnect(uniqueMaterial.GetId());
                }
            }

            if (!anyQueued)
            {
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::InitializeMaterialInstance(const Data::Asset<Data::AssetData>& asset)
        {
            bool allReady = true;
            auto updateAsset = [&](AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset)
            {
                if (materialAsset.GetId() == asset.GetId())
                {
                    materialAsset = asset;
                }

                if (materialAsset.GetId().IsValid() && !materialAsset.IsReady())
                {
                    allReady = false;
                }
            };

            // Update all of the material asset containers to reference the newly loaded asset
            for (auto& materialPair : m_defaultMaterialMap)
            {
                updateAsset(materialPair.second);
            }

            for (auto& materialPair : m_uniqueMaterialMap)
            {
                updateAsset(materialPair.second);
            }

            for (auto& materialPair : m_configuration.m_materials)
            {
                updateAsset(materialPair.second.m_materialAsset);
                updateAsset(materialPair.second.m_defaultMaterialAsset);
            }

            if (allReady)
            {
                // Only start updating materials and instances after all assets that can be loaded have been loaded.
                // This ensures that property changes and notifications only occur once everything is fully loaded.
                for (auto& materialPair : m_configuration.m_materials)
                {
                    materialPair.second.RebuildInstance();
                    MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialInstanceCreated, materialPair.second);
                    QueuePropertyChanges(materialPair.first);
                }
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::ReleaseMaterials()
        {
            TickBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();

            m_defaultMaterialMap.clear();
            m_uniqueMaterialMap.clear();
            m_materialsWithDirtyProperties.clear();
            m_queuedMaterialUpdateNotification = false;
            for (auto& materialPair : m_configuration.m_materials)
            {
                materialPair.second.Release();
            }
        }

        MaterialAssignmentMap MaterialComponentController::GetOriginalMaterialAssignments() const
        {
            MaterialAssignmentMap originalMaterials;
            MaterialReceiverRequestBus::EventResult(
                originalMaterials, m_entityId, &MaterialReceiverRequestBus::Events::GetMaterialAssignments);
            return originalMaterials;
        }

        MaterialAssignmentId MaterialComponentController::FindMaterialAssignmentId(
            const MaterialAssignmentLodIndex lod, const AZStd::string& label) const
        {
            MaterialAssignmentId materialAssignmentId;
            MaterialReceiverRequestBus::EventResult(
                materialAssignmentId, m_entityId, &MaterialReceiverRequestBus::Events::FindMaterialAssignmentId, lod, label);
            return materialAssignmentId;
        }

        AZ::Data::AssetId MaterialComponentController::GetActiveMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const
        {
            // If there is a material override return that asset ID
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt != m_configuration.m_materials.end() && materialIt->second.m_materialAsset.GetId().IsValid())
            {
                return materialIt->second.m_materialAsset.GetId();
            }

            // Otherwise return the cached default material asset ID
            return GetDefaultMaterialAssetId(materialAssignmentId);
        }

        AZ::Data::AssetId MaterialComponentController::GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const
        {
            auto materialIt = m_defaultMaterialMap.find(materialAssignmentId);
            return materialIt != m_defaultMaterialMap.end() ? materialIt->second.GetId() : AZ::Data::AssetId();
        }

        AZStd::string MaterialComponentController::GetMaterialSlotLabel(const MaterialAssignmentId& materialAssignmentId) const
        {
            MaterialAssignmentLabelMap labels;
            MaterialReceiverRequestBus::EventResult(labels, m_entityId, &MaterialReceiverRequestBus::Events::GetMaterialAssignmentLabels);

            auto labelIt = labels.find(materialAssignmentId);
            return labelIt != labels.end() ? labelIt->second : "<unknown>";
        }

        void MaterialComponentController::SetMaterialOverrides(const MaterialAssignmentMap& materials)
        {
            m_configuration.m_materials = materials;
            ConvertAssetsForSerialization();
            QueueLoadMaterials();
        }

        const MaterialAssignmentMap& MaterialComponentController::GetMaterialOverrides() const
        {
            return m_configuration.m_materials;
        }

        void MaterialComponentController::ClearAllMaterialOverrides()
        {
            if (!m_configuration.m_materials.empty())
            {
                m_configuration.m_materials.clear();
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::ClearModelMaterialOverrides()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                return materialPair.first.IsSlotIdOnly();
            });
            if (numErased > 0)
            {
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::ClearLodMaterialOverrides()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                return materialPair.first.IsLodAndSlotId();
            });
            if (numErased > 0)
            {
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::ClearIncompatibleMaterialOverrides()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [this](const auto& materialPair) {
                return m_defaultMaterialMap.find(materialPair.first) == m_defaultMaterialMap.end();
            });
            if (numErased > 0)
            {
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::ClearInvalidMaterialOverrides()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                if (materialPair.second.m_materialAsset.GetId().IsValid())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById,
                        materialPair.second.m_materialAsset.GetId());
                    return !assetInfo.m_assetId.IsValid();
                }
                return false;
            });
            if (numErased > 0)
            {
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::RepairInvalidMaterialOverrides()
        {
            for (auto& materialPair : m_configuration.m_materials)
            {
                if (materialPair.second.m_materialAsset.GetId().IsValid())
                {
                    AZ::Data::AssetInfo assetInfo;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById,
                        materialPair.second.m_materialAsset.GetId());
                    if (!assetInfo.m_assetId.IsValid())
                    {
                        materialPair.second.m_materialAsset = {};
                        QueueMaterialUpdateNotification();
                    }
                }
            }
        }
        
        uint32_t MaterialComponentController::ApplyAutomaticPropertyUpdates()
        {
            uint32_t propertiesUpdated = 0;

            for (auto& materialAssignmentPair : m_configuration.m_materials)
            {
                MaterialAssignment& materialAssignment = materialAssignmentPair.second;
                
                AZStd::vector<AZStd::pair<Name, Name>> renamedProperties;

                for (const auto& propertyPair : materialAssignment.m_propertyOverrides)
                {
                    Name propertyId = propertyPair.first;

                    if (materialAssignment.m_materialInstance->GetAsset()->GetMaterialTypeAsset()->ApplyPropertyRenames(propertyId))
                    {
                        renamedProperties.emplace_back(propertyPair.first, propertyId);
                        ++propertiesUpdated;
                    }
                }
                
                for (const auto& [oldName, newName] : renamedProperties)
                {
                    materialAssignment.m_propertyOverrides[newName] = materialAssignment.m_propertyOverrides[oldName];
                    materialAssignment.m_propertyOverrides.erase(oldName);
                }
                QueuePropertyChanges(materialAssignmentPair.first);
            }

            QueueMaterialUpdateNotification();
            return propertiesUpdated;
        }

        void MaterialComponentController::SetDefaultMaterialOverride(const AZ::Data::AssetId& materialAssetId)
        {
            SetMaterialOverride(DefaultMaterialAssignmentId, materialAssetId);
        }

        const AZ::Data::AssetId MaterialComponentController::GetDefaultMaterialOverride() const
        {
            return GetMaterialOverride(DefaultMaterialAssignmentId);
        }

        void MaterialComponentController::ClearDefaultMaterialOverride()
        {
            ClearMaterialOverride(DefaultMaterialAssignmentId);
        }

        void MaterialComponentController::SetMaterialOverride(
            const MaterialAssignmentId& materialAssignmentId, const AZ::Data::AssetId& materialAssetId)
        {
            auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt != m_configuration.m_materials.end())
            {
                // If the asset ID is invalid and there are no other property overrides then clear the entry
                if (!materialAssetId.IsValid() &&
                    materialIt->second.m_propertyOverrides.empty() &&
                    materialIt->second.m_matModUvOverrides.empty())
                {
                    m_configuration.m_materials.erase(materialAssignmentId);
                    QueueMaterialUpdateNotification();
                    return;
                }

                // If the asset ID is different than what's already assigned then replace it
                if (materialIt->second.m_materialAsset.GetId() != materialAssetId)
                {
                    materialIt->second.m_materialAsset =
                        AZ::Data::Asset<AZ::RPI::MaterialAsset>(materialAssetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
                    QueueLoadMaterials();
                    return;
                }
            }

            m_configuration.m_materials[materialAssignmentId].m_materialAsset =
                AZ::Data::Asset<AZ::RPI::MaterialAsset>(materialAssetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
            QueueLoadMaterials();
        }

        AZ::Data::AssetId MaterialComponentController::GetMaterialOverride(const MaterialAssignmentId& materialAssignmentId) const
        {
            auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() ? materialIt->second.m_materialAsset.GetId() : AZ::Data::AssetId();
        }

        void MaterialComponentController::ClearMaterialOverride(const MaterialAssignmentId& materialAssignmentId)
        {
            SetMaterialOverride(materialAssignmentId, {});
        }

        void MaterialComponentController::SetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value)
        {
            auto& materialAssignment = m_configuration.m_materials[materialAssignmentId];
            const bool wasEmpty = materialAssignment.m_propertyOverrides.empty();
            materialAssignment.m_propertyOverrides[AZ::Name(propertyName)] = value;
            ConvertAssetsForSerialization();

            if (materialAssignment.RequiresLoading())
            {
                QueueLoadMaterials();
                return;
            }

            if (wasEmpty != materialAssignment.m_propertyOverrides.empty())
            {
                materialAssignment.RebuildInstance();
                MaterialComponentNotificationBus::Event(
                    m_entityId, &MaterialComponentNotifications::OnMaterialInstanceCreated, materialAssignment);
                QueueMaterialUpdateNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        AZStd::any MaterialComponentController::GetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt != m_configuration.m_materials.end())
            {
                const auto propertyIt = materialIt->second.m_propertyOverrides.find(AZ::Name(propertyName));
                if (propertyIt != materialIt->second.m_propertyOverrides.end())
                {
                    return propertyIt->second;
                }

                if (materialIt->second.m_materialAsset.IsReady())
                {
                    const auto index =
                        materialIt->second.m_materialAsset->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(propertyName));
                    if (index.IsValid())
                    {
                        return AZ::RPI::MaterialPropertyValue::ToAny(
                            materialIt->second.m_materialAsset->GetPropertyValues()[index.GetIndex()]);
                    }
                }
            }

            const auto defaultMaterialIt = m_defaultMaterialMap.find(materialAssignmentId);
            if (defaultMaterialIt != m_defaultMaterialMap.end() && defaultMaterialIt->second.IsReady())
            {
                const auto index = defaultMaterialIt->second->GetMaterialPropertiesLayout()->FindPropertyIndex(AZ::Name(propertyName));
                if (index.IsValid())
                {
                    return AZ::RPI::MaterialPropertyValue::ToAny(defaultMaterialIt->second->GetPropertyValues()[index.GetIndex()]);
                }
            }

            return {};
        }

        void MaterialComponentController::ClearPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName)
        {
            auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt == m_configuration.m_materials.end())
            {
                return;
            }

            auto propertyIt = materialIt->second.m_propertyOverrides.find(AZ::Name(propertyName));
            if (propertyIt == materialIt->second.m_propertyOverrides.end())
            {
                return;
            }

            materialIt->second.m_propertyOverrides.erase(propertyIt);
            if (materialIt->second.m_propertyOverrides.empty())
            {
                materialIt->second.RebuildInstance();
                MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialInstanceCreated, materialIt->second);
                QueueMaterialUpdateNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        void MaterialComponentController::ClearPropertyOverrides(const MaterialAssignmentId& materialAssignmentId)
        {
            auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt == m_configuration.m_materials.end())
            {
                return;
            }

            if (!materialIt->second.m_propertyOverrides.empty())
            {
                materialIt->second.m_propertyOverrides = {};
                materialIt->second.RebuildInstance();
                MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialInstanceCreated, materialIt->second);
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::ClearAllPropertyOverrides()
        {
            for (auto& materialPair : m_configuration.m_materials)
            {
                if (!materialPair.second.m_propertyOverrides.empty())
                {
                    materialPair.second.m_propertyOverrides = {};
                    materialPair.second.RebuildInstance();
                    MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialInstanceCreated, materialPair.second);
                    QueueMaterialUpdateNotification();
                }
            }
        }

        void MaterialComponentController::SetPropertyOverrides(
            const MaterialAssignmentId& materialAssignmentId, const MaterialPropertyOverrideMap& propertyOverrides)
        {
            auto& materialAssignment = m_configuration.m_materials[materialAssignmentId];
            const bool wasEmpty = materialAssignment.m_propertyOverrides.empty();
            materialAssignment.m_propertyOverrides = propertyOverrides;
            ConvertAssetsForSerialization();

            if (materialAssignment.RequiresLoading())
            {
                QueueLoadMaterials();
                return;
            }

            if (wasEmpty != materialAssignment.m_propertyOverrides.empty())
            {
                materialAssignment.RebuildInstance();
                QueueMaterialUpdateNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        MaterialPropertyOverrideMap MaterialComponentController::GetPropertyOverrides(
            const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() ? materialIt->second.m_propertyOverrides : MaterialPropertyOverrideMap();
        }

        void MaterialComponentController::SetModelUvOverrides(
            const MaterialAssignmentId& materialAssignmentId, const AZ::RPI::MaterialModelUvOverrideMap& modelUvOverrides)
        {
            auto& materialAssignment = m_configuration.m_materials[materialAssignmentId];
            materialAssignment.m_matModUvOverrides = modelUvOverrides;

            if (materialAssignment.RequiresLoading())
            {
                QueueLoadMaterials();
                return;
            }

            // Unlike material properties which are applied to the material itself, UV overrides are applied outside the material
            // by the MeshFeatureProcessor. So all we have to do is notify the mesh component that the materials were updated and it
            // will pass the updated data to the MeshFeatureProcessor.
            QueueMaterialUpdateNotification();
        }

        AZ::RPI::MaterialModelUvOverrideMap MaterialComponentController::GetModelUvOverrides(
            const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() ? materialIt->second.m_matModUvOverrides : AZ::RPI::MaterialModelUvOverrideMap();
        }

        void MaterialComponentController::OnMaterialAssignmentSlotsChanged()
        {
            LoadMaterials();
            MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialSlotLayoutChanged);

        }

        void MaterialComponentController::QueuePropertyChanges(const MaterialAssignmentId& materialAssignmentId)
        {
            m_materialsWithDirtyProperties.emplace(materialAssignmentId);
            if (!TickBus::Handler::BusIsConnected())
            {
                TickBus::Handler::BusConnect();
            }
        }

        void MaterialComponentController::QueueMaterialUpdateNotification()
        {
            m_queuedMaterialUpdateNotification = true;
            if (!TickBus::Handler::BusIsConnected())
            {
                TickBus::Handler::BusConnect();
            }
        }

        void MaterialComponentController::QueueLoadMaterials()
        {
            m_queuedLoadMaterials = true;
            if (!TickBus::Handler::BusIsConnected())
            {
                TickBus::Handler::BusConnect();
            }
        }

        void MaterialComponentController::ConvertAssetsForSerialization()
        {
            for (auto& materialAssignmentPair : m_configuration.m_materials)
            {
                MaterialAssignment& materialAssignment = materialAssignmentPair.second;
                for (auto& propertyPair : materialAssignment.m_propertyOverrides)
                {
                    auto& value = propertyPair.second;
                    if (value.is<AZ::Data::Asset<AZ::Data::AssetData>>())
                    {
                        value = AZStd::any_cast<AZ::Data::Asset<AZ::Data::AssetData>>(value).GetId();
                    }
                    else if (value.is<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>())
                    {
                        value = AZStd::any_cast<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>(value).GetId();
                    }
                    else if (value.is<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>())
                    {
                        value = AZStd::any_cast<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(value).GetId();
                    }
                    else if (value.is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
                    {
                        value = AZStd::any_cast<AZ::Data::Asset<AZ::RPI::ImageAsset>>(value).GetId();
                    }
                    else if (value.is<AZ::Data::Instance<AZ::RPI::Image>>())
                    {
                        value = AZStd::any_cast<AZ::Data::Instance<AZ::RPI::Image>>(value)->GetAssetId();
                    }
                }
            }
        }
    } // namespace Render
} // namespace AZ
