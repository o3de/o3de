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

            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialComponentController>()
                    ->Version(1)
                    ->Field("Configuration", &MaterialComponentController::m_configuration)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<MaterialComponentRequestBus>("MaterialComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "render")
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Event("GetDefaultMaterialMap", &MaterialComponentRequestBus::Events::GetDefaultMaterialMap, "GetDefautMaterialMap")
                    ->Event("FindMaterialAssignmentId", &MaterialComponentRequestBus::Events::FindMaterialAssignmentId)
                    ->Event("GetActiveMaterialAssetId", &MaterialComponentRequestBus::Events::GetMaterialAssetId) // This function is now redundant but cannot be marked deprecated or removed in case it's still referenced in script
                    ->Event("GetDefaultMaterialAssetId", &MaterialComponentRequestBus::Events::GetDefaultMaterialAssetId)
                    ->Event("IsDefaultMaterialAssetReady", &MaterialComponentRequestBus::Events::IsDefaultMaterialAssetReady)
                    ->Event("GetMaterialLabel", &MaterialComponentRequestBus::Events::GetMaterialLabel, "GetMaterialSlotLabel")
                    ->Event("SetMaterialMap", &MaterialComponentRequestBus::Events::SetMaterialMap, "SetMaterialOverrides")
                    ->Event("GetMaterialMap", &MaterialComponentRequestBus::Events::GetMaterialMap, "GetMaterialOverrides")
                    ->Event("GetMaterialMapCopy", &MaterialComponentRequestBus::Events::GetMaterialMapCopy)
                    ->Event("ClearMaterialMap", &MaterialComponentRequestBus::Events::ClearMaterialMap, "ClearAllMaterialOverrides")
                    ->Event("SetMaterialAssetIdOnDefaultSlot", &MaterialComponentRequestBus::Events::SetMaterialAssetIdOnDefaultSlot, "SetDefaultMaterialOverride")
                    ->Event("GetMaterialAssetIdOnDefaultSlot", &MaterialComponentRequestBus::Events::GetMaterialAssetIdOnDefaultSlot, "GetDefaultMaterialOverride")
                    ->Event("ClearMaterialAssetIdOnDefaultSlot", &MaterialComponentRequestBus::Events::ClearMaterialAssetIdOnDefaultSlot, "ClearDefaultMaterialOverride")
                    ->Event("ClearMaterialsOnModelSlots", &MaterialComponentRequestBus::Events::ClearMaterialsOnModelSlots, "ClearModelMaterialOverrides")
                    ->Event("ClearMaterialsOnLodSlots", &MaterialComponentRequestBus::Events::ClearMaterialsOnLodSlots, "ClearLodMaterialOverrides")
                    ->Event("ClearMaterialsOnInvalidSlots", &MaterialComponentRequestBus::Events::ClearMaterialsOnInvalidSlots, "ClearIncompatibleMaterialOverrides")
                    ->Event("ClearMaterialsWithMissingAssets", &MaterialComponentRequestBus::Events::ClearMaterialsWithMissingAssets, "ClearInvalidMaterialOverrides")
                    ->Event("RepairMaterialsWithMissingAssets", &MaterialComponentRequestBus::Events::RepairMaterialsWithMissingAssets, "RepairInvalidMaterialOverrides")
                    ->Event("RepairMaterialsWithRenamedProperties", &MaterialComponentRequestBus::Events::RepairMaterialsWithRenamedProperties, "ApplyAutomaticPropertyUpdates")
                    ->Event("SetMaterialAssetId", &MaterialComponentRequestBus::Events::SetMaterialAssetId, "SetMaterialOverride")
                    ->Event("GetMaterialAssetId", &MaterialComponentRequestBus::Events::GetMaterialAssetId, "GetMaterialOverride")
                    ->Event("IsMaterialAssetReady", &MaterialComponentRequestBus::Events::IsMaterialAssetReady)
                    ->Event("ClearMaterialAssetId", &MaterialComponentRequestBus::Events::ClearMaterialAssetId, "ClearMaterialOverride")
                    ->Event("IsMaterialAssetIdOverridden", &MaterialComponentRequestBus::Events::IsMaterialAssetIdOverridden)
                    ->Event("HasPropertiesOverridden", &MaterialComponentRequestBus::Events::HasPropertiesOverridden)
                    ->Event("SetPropertyValue", &MaterialComponentRequestBus::Events::SetPropertyValue, "SetPropertyOverride")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ->Event("SetPropertyValueBool", &MaterialComponentRequestBus::Events::SetPropertyValueT<bool>, "SetPropertyOverrideBool")
                    ->Event("SetPropertyValueInt32", &MaterialComponentRequestBus::Events::SetPropertyValueT<int32_t>, "SetPropertyOverrideInt32")
                    ->Event("SetPropertyValueUInt32", &MaterialComponentRequestBus::Events::SetPropertyValueT<uint32_t>, "SetPropertyOverrideUInt32")
                    ->Event("SetPropertyValueFloat", &MaterialComponentRequestBus::Events::SetPropertyValueT<float>, "SetPropertyOverrideFloat")
                    ->Event("SetPropertyValueVector2", &MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Vector2>, "SetPropertyOverrideVector2")
                    ->Event("SetPropertyValueVector3", &MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Vector3>, "SetPropertyOverrideVector3")
                    ->Event("SetPropertyValueVector4", &MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Vector4>, "SetPropertyOverrideVector4")
                    ->Event("SetPropertyValueColor", &MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Color>, "SetPropertyOverrideColor")
                    ->Event("SetPropertyValueImage", &MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Data::AssetId>, "SetPropertyOverrideImage")
                    ->Event("SetPropertyValueString", &MaterialComponentRequestBus::Events::SetPropertyValueT<AZStd::string>, "SetPropertyOverrideString")
                    ->Event("SetPropertyValueEnum", &MaterialComponentRequestBus::Events::SetPropertyValueT<uint32_t>, "SetPropertyOverrideEnum")
                    ->Event("GetPropertyValue", &MaterialComponentRequestBus::Events::GetPropertyValue, "GetPropertyOverride")
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ->Event("GetPropertyValueBool", &MaterialComponentRequestBus::Events::GetPropertyValueT<bool>, "GetPropertyOverrideBool")
                    ->Event("GetPropertyValueInt32", &MaterialComponentRequestBus::Events::GetPropertyValueT<int32_t>, "GetPropertyOverrideInt32")
                    ->Event("GetPropertyValueUInt32", &MaterialComponentRequestBus::Events::GetPropertyValueT<uint32_t>, "GetPropertyOverrideUInt32")
                    ->Event("GetPropertyValueFloat", &MaterialComponentRequestBus::Events::GetPropertyValueT<float>, "GetPropertyOverrideFloat")
                    ->Event("GetPropertyValueVector2", &MaterialComponentRequestBus::Events::GetPropertyValueT<AZ::Vector2>, "GetPropertyOverrideVector2")
                    ->Event("GetPropertyValueVector3", &MaterialComponentRequestBus::Events::GetPropertyValueT<AZ::Vector3>, "GetPropertyOverrideVector3")
                    ->Event("GetPropertyValueVector4", &MaterialComponentRequestBus::Events::GetPropertyValueT<AZ::Vector4>, "GetPropertyOverrideVector4")
                    ->Event("GetPropertyValueColor", &MaterialComponentRequestBus::Events::GetPropertyValueT<AZ::Color>, "GetPropertyOverrideColor")
                    ->Event("GetPropertyValueImage", &MaterialComponentRequestBus::Events::GetPropertyValueT<AZ::Data::AssetId>, "GetPropertyOverrideImage")
                    ->Event("GetPropertyValueString", &MaterialComponentRequestBus::Events::GetPropertyValueT<AZStd::string>, "GetPropertyOverrideString")
                    ->Event("GetPropertyValueEnum", &MaterialComponentRequestBus::Events::GetPropertyValueT<uint32_t>, "GetPropertyOverrideEnum")
                    ->Event("ClearPropertyValue", &MaterialComponentRequestBus::Events::ClearPropertyValue, "ClearPropertyOverride")
                    ->Event("ClearPropertyValues", &MaterialComponentRequestBus::Events::ClearPropertyValues, "ClearPropertyOverrides")
                    ->Event("ClearAllPropertyValues", &MaterialComponentRequestBus::Events::ClearAllPropertyValues, "ClearAllPropertyOverrides")
                    ->Event("SetPropertyValues", &MaterialComponentRequestBus::Events::SetPropertyValues, "SetPropertyOverrides")
                    ->Event("GetPropertyValues", &MaterialComponentRequestBus::Events::GetPropertyValues, "GetPropertyOverrides")
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
            required.push_back(AZ_CRC_CE("MaterialConsumerService"));
        }

        MaterialComponentController::MaterialComponentController(const MaterialComponentConfig& config)
            : m_configuration(config)
        {
            ConvertAssetsForSerialization();
        }

        void MaterialComponentController::Activate(EntityId entityId)
        {
            m_entityId = entityId;
            m_queuedMaterialsCreatedNotification = false;
            m_queuedMaterialsUpdatedNotification = false;

            MaterialComponentRequestBus::Handler::BusConnect(m_entityId);
            MaterialConsumerNotificationBus::Handler::BusConnect(m_entityId);
            LoadMaterials();
        }

        void MaterialComponentController::Deactivate()
        {
            MaterialComponentRequestBus::Handler::BusDisconnect();
            MaterialConsumerNotificationBus::Handler::BusDisconnect();

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

        void MaterialComponentController::OnAssetError(Data::Asset<Data::AssetData> asset)
        {
            DisplayMissingAssetWarning(asset);
            InitializeMaterialInstance(asset);
        }

        void MaterialComponentController::OnAssetReloadError(Data::Asset<Data::AssetData> asset)
        {
            DisplayMissingAssetWarning(asset);
            InitializeMaterialInstance(asset);
        }

        void MaterialComponentController::OnSystemTick()
        {
            while (!m_notifiedMaterialAssets.empty())
            {
                auto materialAsset = m_notifiedMaterialAssets.front();
                m_notifiedMaterialAssets.pop();
                InitializeNotifiedMaterialAsset(materialAsset);
            }

            if (m_queuedLoadMaterials)
            {
                m_queuedLoadMaterials = false;
                LoadMaterials();
            }

            if (m_queuedMaterialsCreatedNotification)
            {
                m_queuedMaterialsCreatedNotification = false;
                MaterialComponentNotificationBus::Event(
                    m_entityId, &MaterialComponentNotifications::OnMaterialsCreated, m_configuration.m_materials);
            }

            bool propertiesChanged = false;
            AZStd::unordered_set<MaterialAssignmentId> materialsWithDirtyProperties;
            AZStd::swap(m_materialsWithDirtyProperties, materialsWithDirtyProperties);

            // Iterate through all MaterialAssignmentId's that have property overrides and attempt to apply them
            for (const auto& materialAssignmentId : materialsWithDirtyProperties)
            {
                const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
                if (materialIt != m_configuration.m_materials.end())
                {
                    if (materialIt->second.ApplyProperties())
                    {
                        propertiesChanged = true;
                    }
                    else
                    {
                        // If the material had properties to apply and it could not be compiled then queue it again
                        m_materialsWithDirtyProperties.emplace(materialAssignmentId);
                    }
                }
            }

            if (propertiesChanged)
            {
                MaterialComponentNotificationBus::Event(
                    m_entityId, &MaterialComponentNotifications::OnMaterialPropertiesUpdated, m_configuration.m_materials);
            }

            // Only send notifications that materials have been updated after all pending properties have been applied
            if (m_queuedMaterialsUpdatedNotification && m_materialsWithDirtyProperties.empty())
            {
                // Materials have been edited and instances have changed but the notification will only be sent once per tick
                m_queuedMaterialsUpdatedNotification = false;
                MaterialComponentNotificationBus::Event(
                    m_entityId, &MaterialComponentNotifications::OnMaterialsUpdated, m_configuration.m_materials);
            }

            // Only disconnect from the tick bus if there is no remaining work for the next tick. It's possible that additional work was
            // queued while notifications were in progress.
            if (!m_queuedLoadMaterials &&
                !m_queuedMaterialsCreatedNotification &&
                !m_queuedMaterialsUpdatedNotification &&
                m_materialsWithDirtyProperties.empty() &&
                m_notifiedMaterialAssets.empty())
            {
                SystemTickBus::Handler::BusDisconnect();
            }
        }

        void MaterialComponentController::LoadMaterials()
        {
            // Caching previously loaded unique materials to avoid unloading and reloading assets that have not changed
            AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::MaterialAsset>> uniqueMaterialMapBeforeLoad;
            AZStd::swap(uniqueMaterialMapBeforeLoad, m_uniqueMaterialMap);

            // Clear any previously loaded or queued material assets, instances, or notifications
            ReleaseMaterials();

            MaterialConsumerRequestBus::EventResult(
                m_defaultMaterialMap, m_entityId, &MaterialConsumerRequestBus::Events::GetDefaultMaterialMap);

            // Build tables of all referenced materials so that we can load and look up defaults
            for (const auto& [materialAssignmentId, materialAssignment] : m_defaultMaterialMap)
            {
                const auto& defaultMaterialAsset = materialAssignment.m_materialAsset;
                m_uniqueMaterialMap[defaultMaterialAsset.GetId()] = defaultMaterialAsset;

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
                    AZ::Data::AssetLoadParameters loadParams;
                    loadParams.m_dependencyRules = AZ::Data::AssetDependencyLoadRules::LoadAll;
                    loadParams.m_reloadMissingDependencies = true;
                    if (uniqueMaterial.QueueLoad(loadParams))
                    {
                        anyQueued = true;
                    }
                    else
                    {
                        DisplayMissingAssetWarning(uniqueMaterial);
                    }

                    AZ::Data::AssetBus::MultiHandler::BusConnect(uniqueMaterial.GetId());
                }
            }

            if (!anyQueued)
            {
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::InitializeNotifiedMaterialAsset(Data::Asset<Data::AssetData> asset)
        {
            bool allReady = true;
            auto updateAsset = [&](AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset)
            {
                if (materialAsset.GetId() == asset.GetId())
                {
                    materialAsset = asset;
                }

                if (materialAsset.GetId().IsValid() && !materialAsset.IsReady() && !materialAsset.IsError())
                {
                    allReady = false;
                }
            };

            // Update all of the material asset containers to reference the newly loaded asset
            for (auto& materialPair : m_uniqueMaterialMap)
            {
                updateAsset(materialPair.second);
            }

            for (auto& materialPair : m_defaultMaterialMap)
            {
                updateAsset(materialPair.second.m_materialAsset);
                updateAsset(materialPair.second.m_defaultMaterialAsset);
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
                    QueuePropertyChanges(materialPair.first);
                }
                QueueMaterialsCreatedNotification();
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::InitializeMaterialInstance(Data::Asset<Data::AssetData> asset)
        {
            // See header file, where @m_notifiedMaterialAssets is declared for details.
            m_notifiedMaterialAssets.push(asset);
            SystemTickBus::Handler::BusConnect();
        }

        void MaterialComponentController::ReleaseMaterials()
        {
            SystemTickBus::Handler::BusDisconnect();
            Data::AssetBus::MultiHandler::BusDisconnect();

            m_defaultMaterialMap.clear();
            m_uniqueMaterialMap.clear();
            m_materialsWithDirtyProperties.clear();
            m_queuedMaterialsCreatedNotification = false;
            m_queuedMaterialsUpdatedNotification = false;
            for (auto& materialPair : m_configuration.m_materials)
            {
                materialPair.second.Release();
            }
            decltype(m_notifiedMaterialAssets) tmpQueue;
            AZStd::swap(m_notifiedMaterialAssets, tmpQueue);
        }

        MaterialAssignmentMap MaterialComponentController::GetDefaultMaterialMap() const
        {
            return m_defaultMaterialMap;
        }

        MaterialAssignmentId MaterialComponentController::FindMaterialAssignmentId(
            const MaterialAssignmentLodIndex lod, const AZStd::string& label) const
        {
            MaterialAssignmentId materialAssignmentId;
            MaterialConsumerRequestBus::EventResult(
                materialAssignmentId, m_entityId, &MaterialConsumerRequestBus::Events::FindMaterialAssignmentId, lod, label);
            return materialAssignmentId;
        }

        AZ::Data::AssetId MaterialComponentController::GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_defaultMaterialMap.find(materialAssignmentId);
            return materialIt != m_defaultMaterialMap.end() ? materialIt->second.m_materialAsset.GetId() : AZ::Data::AssetId();
        }

        bool MaterialComponentController::IsDefaultMaterialAssetReady(const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_defaultMaterialMap.find(materialAssignmentId);
            return (materialIt != m_defaultMaterialMap.end()) && materialIt->second.m_materialAsset.IsReady();
        }

        AZStd::string MaterialComponentController::GetMaterialLabel(const MaterialAssignmentId& materialAssignmentId) const
        {
            MaterialAssignmentLabelMap labels;
            MaterialConsumerRequestBus::EventResult(labels, m_entityId, &MaterialConsumerRequestBus::Events::GetMaterialLabels);

            auto labelIt = labels.find(materialAssignmentId);
            return labelIt != labels.end() ? labelIt->second : "<unknown>";
        }

        void MaterialComponentController::SetMaterialMap(const MaterialAssignmentMap& materials)
        {
            m_configuration.m_materials = materials;
            ConvertAssetsForSerialization();
            QueueLoadMaterials();
        }

        const MaterialAssignmentMap& MaterialComponentController::GetMaterialMap() const
        {
            return m_configuration.m_materials;
        }

        MaterialAssignmentMap MaterialComponentController::GetMaterialMapCopy() const
        {
            return m_configuration.m_materials;
        }

        void MaterialComponentController::ClearMaterialMap()
        {
            if (!m_configuration.m_materials.empty())
            {
                m_configuration.m_materials.clear();
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::ClearMaterialsOnModelSlots()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                return materialPair.first.IsSlotIdOnly();
            });
            if (numErased > 0)
            {
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::ClearMaterialsOnLodSlots()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                return materialPair.first.IsLodAndSlotId();
            });
            if (numErased > 0)
            {
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::ClearMaterialsOnInvalidSlots()
        {
            const auto numErased = AZStd::erase_if(m_configuration.m_materials, [this](const auto& materialPair) {
                return m_defaultMaterialMap.find(materialPair.first) == m_defaultMaterialMap.end();
            });
            if (numErased > 0)
            {
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::ClearMaterialsWithMissingAssets()
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
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::RepairMaterialsWithMissingAssets()
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
                        QueueMaterialsUpdatedNotification();
                    }
                }
            }
        }
        
        uint32_t MaterialComponentController::RepairMaterialsWithRenamedProperties()
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

            QueueMaterialsUpdatedNotification();
            return propertiesUpdated;
        }

        void MaterialComponentController::SetMaterialAssetIdOnDefaultSlot(const AZ::Data::AssetId& materialAssetId)
        {
            SetMaterialAssetId(DefaultMaterialAssignmentId, materialAssetId);
        }

        const AZ::Data::AssetId MaterialComponentController::GetMaterialAssetIdOnDefaultSlot() const
        {
            return GetMaterialAssetId(DefaultMaterialAssignmentId);
        }

        void MaterialComponentController::ClearMaterialAssetIdOnDefaultSlot()
        {
            ClearMaterialAssetId(DefaultMaterialAssignmentId);
        }

        void MaterialComponentController::SetMaterialAssetId(
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
                    QueueMaterialsUpdatedNotification();
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

        AZ::Data::AssetId MaterialComponentController::GetMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const
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

        bool MaterialComponentController::IsMaterialAssetReady(const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return (materialIt != m_configuration.m_materials.end()) && materialIt->second.m_materialAsset.IsReady();
        }

        void MaterialComponentController::ClearMaterialAssetId(const MaterialAssignmentId& materialAssignmentId)
        {
            SetMaterialAssetId(materialAssignmentId, {});
        }

        bool MaterialComponentController::IsMaterialAssetIdOverridden(const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() && materialIt->second.m_materialAsset.GetId().IsValid();
        }

        bool MaterialComponentController::HasPropertiesOverridden(const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() && !materialIt->second.m_propertyOverrides.empty();
        }

        void MaterialComponentController::SetPropertyValue(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value)
        {
            auto& materialAssignment = m_configuration.m_materials[materialAssignmentId];
            const bool wasEmpty = materialAssignment.m_propertyOverrides.empty();
            materialAssignment.m_propertyOverrides[AZ::Name(propertyName)] = ConvertAssetsForSerialization(value);

            if (materialAssignment.RequiresLoading())
            {
                QueueLoadMaterials();
                return;
            }

            if (wasEmpty != materialAssignment.m_propertyOverrides.empty())
            {
                materialAssignment.RebuildInstance();
                QueueMaterialsCreatedNotification();
                QueueMaterialsUpdatedNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        AZStd::any MaterialComponentController::GetPropertyValue(
            const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const
        {
            AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;

            // First, check if there is an explicit property value override matching the name and material slot
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt != m_configuration.m_materials.end())
            {
                // If there is an explicit property value override return it immediately
                const auto propertyIt = materialIt->second.m_propertyOverrides.find(AZ::Name(propertyName));
                if (propertyIt != materialIt->second.m_propertyOverrides.end() && !propertyIt->second.empty())
                {
                    return propertyIt->second;
                }

                // Otherwise, determine the highest priority material asset to pull property values from
                materialAsset = materialIt->second.m_materialAsset;
                if (!materialAsset.IsReady())
                {
                    materialAsset = materialIt->second.m_defaultMaterialAsset;
                }
            }

            // No matching value or asset has been found, so check against the default material map
            if (!materialAsset.IsReady())
            {
                const auto defaultMaterialIt = m_defaultMaterialMap.find(materialAssignmentId);
                if (defaultMaterialIt != m_defaultMaterialMap.end() && defaultMaterialIt->second.m_materialAsset.IsReady())
                {
                    materialAsset = defaultMaterialIt->second.m_materialAsset;
                }
            }

            // If a valid, ready asset was identified, attempt to read the default property value from it.
            if (materialAsset.IsReady())
            {
                const auto layout = materialAsset->GetMaterialPropertiesLayout();
                const auto index = layout->FindPropertyIndex(AZ::Name(propertyName));
                if (index.IsValid())
                {
                    auto propertyValue = materialAsset->GetPropertyValues()[index.GetIndex()];
                    return ConvertAssetsForSerialization(AZ::RPI::MaterialPropertyValue::ToAny(propertyValue));
                }
            }

            return {};
        }

        void MaterialComponentController::ClearPropertyValue(
            const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName)
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
                QueueMaterialsCreatedNotification();
                QueueMaterialsUpdatedNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        void MaterialComponentController::ClearPropertyValues(const MaterialAssignmentId& materialAssignmentId)
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
                QueueMaterialsCreatedNotification();
                QueueMaterialsUpdatedNotification();
            }
        }

        void MaterialComponentController::ClearAllPropertyValues()
        {
            for (auto& materialPair : m_configuration.m_materials)
            {
                if (!materialPair.second.m_propertyOverrides.empty())
                {
                    materialPair.second.m_propertyOverrides = {};
                    materialPair.second.RebuildInstance();
                    QueueMaterialsCreatedNotification();
                    QueueMaterialsUpdatedNotification();
                }
            }
        }

        void MaterialComponentController::SetPropertyValues(
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
                QueueMaterialsCreatedNotification();
                QueueMaterialsUpdatedNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        MaterialPropertyOverrideMap MaterialComponentController::GetPropertyValues(const MaterialAssignmentId& materialAssignmentId) const
        {
            MaterialPropertyOverrideMap properties;
            AZ::Data::Asset<AZ::RPI::MaterialAsset> materialAsset;

            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            if (materialIt != m_configuration.m_materials.end())
            {
                // First, insert all the explicit material property overrides into the returned property map
                properties.insert(materialIt->second.m_propertyOverrides.begin(), materialIt->second.m_propertyOverrides.end());

                // Determine if there is an asset ready with additional values to populate the map
                materialAsset = materialIt->second.m_materialAsset;
                if (!materialAsset.IsReady())
                {
                    materialAsset = materialIt->second.m_defaultMaterialAsset;
                }
            }

            // If no hacen has been identified, fall back to the default material mapping
            if (!materialAsset.IsReady())
            {
                const auto defaultMaterialIt = m_defaultMaterialMap.find(materialAssignmentId);
                if (defaultMaterialIt != m_defaultMaterialMap.end() && defaultMaterialIt->second.m_materialAsset.IsReady())
                {
                    materialAsset = defaultMaterialIt->second.m_materialAsset;
                }
            }

            // If any valid, already has said was found then read the rest of the material values from it
            if (materialAsset.IsReady() && materialAsset->GetMaterialTypeAsset().IsReady())
            {
                const auto layout = materialAsset->GetMaterialPropertiesLayout();
                for (size_t propertyIndex = 0; propertyIndex < layout->GetPropertyCount(); ++propertyIndex)
                {
                    auto descriptor = layout->GetPropertyDescriptor(AZ::RPI::MaterialPropertyIndex{ propertyIndex });
                    auto propertyValue = materialAsset->GetPropertyValues()[propertyIndex];
                    properties.insert({ descriptor->GetName().GetStringView(),
                                        ConvertAssetsForSerialization(AZ::RPI::MaterialPropertyValue::ToAny(propertyValue)) });
                }
            }

            return properties;
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
            QueueMaterialsUpdatedNotification();
        }

        AZ::RPI::MaterialModelUvOverrideMap MaterialComponentController::GetModelUvOverrides(
            const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() ? materialIt->second.m_matModUvOverrides
                                                                   : AZ::RPI::MaterialModelUvOverrideMap();
        }

        void MaterialComponentController::OnMaterialAssignmentSlotsChanged()
        {
            LoadMaterials();
            MaterialComponentNotificationBus::Event(m_entityId, &MaterialComponentNotifications::OnMaterialSlotLayoutChanged);
        }

        void MaterialComponentController::QueuePropertyChanges(const MaterialAssignmentId& materialAssignmentId)
        {
            m_materialsWithDirtyProperties.emplace(materialAssignmentId);
            SystemTickBus::Handler::BusConnect();
        }

        void MaterialComponentController::QueueMaterialsCreatedNotification()
        {
            m_queuedMaterialsCreatedNotification = true;
            SystemTickBus::Handler::BusConnect();
        }

        void MaterialComponentController::QueueMaterialsUpdatedNotification()
        {
            m_queuedMaterialsUpdatedNotification = true;
            SystemTickBus::Handler::BusConnect();
        }

        void MaterialComponentController::QueueLoadMaterials()
        {
            m_queuedLoadMaterials = true;
            SystemTickBus::Handler::BusConnect();
        }

        void MaterialComponentController::ConvertAssetsForSerialization()
        {
            for (auto& materialAssignmentPair : m_configuration.m_materials)
            {
                ConvertAssetsForSerialization(materialAssignmentPair.second.m_propertyOverrides);
            }
        }

        void MaterialComponentController::ConvertAssetsForSerialization(MaterialPropertyOverrideMap& propertyMap)
        {
            for (auto& propertyPair : propertyMap)
            {
                ConvertAssetsForSerialization(propertyPair.second);
            }
        }

        AZStd::any MaterialComponentController::ConvertAssetsForSerialization(const AZStd::any& value) const
        {
            if (value.is<AZ::Data::Asset<AZ::Data::AssetData>>())
            {
                return AZStd::any(AZStd::any_cast<AZ::Data::Asset<AZ::Data::AssetData>>(value).GetId());
            }
            if (value.is<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>())
            {
                return AZStd::any(AZStd::any_cast<AZ::Data::Asset<AZ::RPI::AttachmentImageAsset>>(value).GetId());
            }
            if (value.is<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>())
            {
                return AZStd::any(AZStd::any_cast<AZ::Data::Asset<AZ::RPI::StreamingImageAsset>>(value).GetId());
            }
            if (value.is<AZ::Data::Asset<AZ::RPI::ImageAsset>>())
            {
                return AZStd::any(AZStd::any_cast<AZ::Data::Asset<AZ::RPI::ImageAsset>>(value).GetId());
            }

            return value;
        }

        void MaterialComponentController::DisplayMissingAssetWarning([[maybe_unused]] Data::Asset<Data::AssetData> asset) const
        {
            AZ_Warning(
                "MaterialComponent",
                false,
                "Material component on entity %s failed to load asset %s. The material component might contain additional material and "
                "property data if the component was copied or the associated model changed. This data can be cleared using the material "
                "component request bus or from the editor material component context menu.",
                m_entityId.ToString().c_str(),
                asset.ToString<AZStd::string>().c_str());
        }
    } // namespace Render
} // namespace AZ
