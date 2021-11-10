/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Material/MaterialComponentController.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AtomCore/Instance/InstanceDatabase.h>

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
            provided.push_back(AZ_CRC("MaterialProviderService", 0x64849a6b));
        }

        void MaterialComponentController::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("MaterialProviderService", 0x64849a6b));
        }

        void MaterialComponentController::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("MaterialReceiverService", 0x0d1a6a74));
        }

        MaterialComponentController::MaterialComponentController(const MaterialComponentConfig& config)
            : m_configuration(config)
        {
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
            TickBus::Handler::BusDisconnect();

            ReleaseMaterials();

            // Sending notification to wipe any previously assigned material overrides
            MaterialComponentNotificationBus::Event(
                m_entityId, &MaterialComponentNotifications::OnMaterialsUpdated, MaterialAssignmentMap());

            m_queuedMaterialUpdateNotification = false;
            m_entityId = AZ::EntityId(AZ::EntityId::InvalidEntityId);
        }

        void MaterialComponentController::SetConfiguration(const MaterialComponentConfig& config)
        {
            m_configuration = config;
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
            AZStd::unordered_set<MaterialAssignmentId> materialsWithDirtyProperties;
            AZStd::swap(m_materialsWithDirtyProperties, materialsWithDirtyProperties);

            // Iterate through all MaterialAssignmentId's that have property overrides and attempt to apply them
            // if material instance is already compiling, delay application of property overrides until next frame
            for (const auto& materialAssignmentId : materialsWithDirtyProperties)
            {
                const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
                if (materialIt != m_configuration.m_materials.end())
                {
                    if (!materialIt->second.ApplyProperties())
                    {
                        // If a material cannot currently be compiled then it must be queued again
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
            Data::AssetBus::MultiHandler::BusDisconnect();

            bool anyQueued = false;
            auto queueAsset = [&anyQueued, this](AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset) -> bool
            {
                if (materialAsset.GetId().IsValid() && !this->Data::AssetBus::MultiHandler::BusIsConnectedId(materialAsset.GetId()))
                {
                    anyQueued = true;
                    materialAsset.QueueLoad();
                    this->Data::AssetBus::MultiHandler::BusConnect(materialAsset.GetId());
                    return true;
                }
                return false;
            };

            for (auto& materialPair : m_configuration.m_materials)
            {
                if (materialPair.second.m_materialInstancePreCreated)
                {
                    continue;
                }

                materialPair.second.m_defaultMaterialAsset = {};
                if (!queueAsset(materialPair.second.m_materialAsset))
                {
                    // Only assign and load the default material if there was no material override and there are propoerties to apply
                    if (!materialPair.second.m_propertyOverrides.empty() || !materialPair.second.m_matModUvOverrides.empty())
                    {
                        materialPair.second.m_defaultMaterialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
                            GetDefaultMaterialAssetId(materialPair.first), AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
                        queueAsset(materialPair.second.m_defaultMaterialAsset);
                    }
                }
            }

            if (!anyQueued)
            {
                ReleaseMaterials();

                // If no other materials were loaded, the notification must still be sent in case there are externally managed material
                // instances in the configuration
                MaterialComponentNotificationBus::Event(
                    m_entityId, &MaterialComponentNotifications::OnMaterialsUpdated, m_configuration.m_materials);
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

            for (auto& materialPair : m_configuration.m_materials)
            {
                updateAsset(materialPair.second.m_materialAsset);
                updateAsset(materialPair.second.m_defaultMaterialAsset);
            }

            if (allReady)
            {
                //Do not start updating materials and properties until all materials are loaded and ready
                //This prevents property changes from being queued and notifications from being sent with pending materials
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
            Data::AssetBus::MultiHandler::BusDisconnect();

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

        AZ::Data::AssetId MaterialComponentController::GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const
        {
            RPI::ModelMaterialSlotMap modelMaterialSlots;
            MaterialReceiverRequestBus::EventResult(
                modelMaterialSlots, m_entityId, &MaterialReceiverRequestBus::Events::GetModelMaterialSlots);

            auto slotIter = modelMaterialSlots.find(materialAssignmentId.m_materialSlotStableId);
            return slotIter != modelMaterialSlots.end() ? slotIter->second.m_defaultMaterialAsset.GetId() : AZ::Data::AssetId();
        }

        AZStd::string MaterialComponentController::GetMaterialSlotLabel(const MaterialAssignmentId& materialAssignmentId) const
        {
            if (materialAssignmentId == DefaultMaterialAssignmentId)
            {
                return "Default Material";
            }

            RPI::ModelMaterialSlotMap modelMaterialSlots;
            MaterialReceiverRequestBus::EventResult(
                modelMaterialSlots, m_entityId, &MaterialReceiverRequestBus::Events::GetModelMaterialSlots);

            auto slotIter = modelMaterialSlots.find(materialAssignmentId.m_materialSlotStableId);
            if (slotIter != modelMaterialSlots.end())
            {
                const Name& displayName = slotIter->second.m_displayName;
                if (!displayName.IsEmpty())
                {
                    return displayName.GetStringView();
                }
            }

            return "<unknown>";
        }

        void MaterialComponentController::SetMaterialOverrides(const MaterialAssignmentMap& materials)
        {
            // this function is called twice once material asset is changed, a temp variable is
            // needed to prevent material asset going out of scope during second call
            // before LoadMaterials() is called [LYN-2249]
            auto temp = m_configuration.m_materials;
            m_configuration.m_materials = materials;
            LoadMaterials();
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
            AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                return materialPair.first.IsSlotIdOnly();
            });
            QueueMaterialUpdateNotification();
        }

        void MaterialComponentController::ClearLodMaterialOverrides()
        {
            AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
                return materialPair.first.IsLodAndSlotId();
            });
            QueueMaterialUpdateNotification();
        }

        void MaterialComponentController::ClearIncompatibleMaterialOverrides()
        {
            const MaterialAssignmentMap& originalMaterials = GetOriginalMaterialAssignments();
            AZStd::erase_if(m_configuration.m_materials, [&originalMaterials](const auto& materialPair) {
                return originalMaterials.find(materialPair.first) == originalMaterials.end();
            });
            QueueMaterialUpdateNotification();
        }

        void MaterialComponentController::ClearInvalidMaterialOverrides()
        {
            AZStd::erase_if(m_configuration.m_materials, [](const auto& materialPair) {
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
            QueueMaterialUpdateNotification();
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
                        materialPair.second.m_materialAsset = AZ::Data::Asset<AZ::RPI::MaterialAsset>(
                            GetDefaultMaterialAssetId(materialPair.first), AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
                    }
                }
            }
            LoadMaterials();
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
            }

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
            m_configuration.m_materials[materialAssignmentId].m_materialAsset =
                AZ::Data::Asset<AZ::RPI::MaterialAsset>(materialAssetId, AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid());
            LoadMaterials();
        }

        AZ::Data::AssetId MaterialComponentController::GetMaterialOverride(const MaterialAssignmentId& materialAssignmentId) const
        {
            auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() ? materialIt->second.m_materialAsset.GetId() : AZ::Data::AssetId();
        }

        void MaterialComponentController::ClearMaterialOverride(const MaterialAssignmentId& materialAssignmentId)
        {
            if (m_configuration.m_materials.erase(materialAssignmentId) > 0)
            {
                QueueMaterialUpdateNotification();
            }
        }

        void MaterialComponentController::SetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value)
        {
            auto& materialAssignment = m_configuration.m_materials[materialAssignmentId];
            const bool wasEmpty = materialAssignment.m_propertyOverrides.empty();
            materialAssignment.m_propertyOverrides[AZ::Name(propertyName)] = value;

            if (materialAssignment.RequiresLoading())
            {
                LoadMaterials();
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
            if (materialIt == m_configuration.m_materials.end())
            {
                return {};
            }

            const auto propertyIt = materialIt->second.m_propertyOverrides.find(AZ::Name(propertyName));
            if (propertyIt == materialIt->second.m_propertyOverrides.end())
            {
                return {};
            }

            return propertyIt->second;
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

            if (materialAssignment.RequiresLoading())
            {
                LoadMaterials();
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
            const bool wasEmpty = materialAssignment.m_matModUvOverrides.empty();
            materialAssignment.m_matModUvOverrides = modelUvOverrides;

            if (materialAssignment.RequiresLoading())
            {
                LoadMaterials();
                return;
            }

            if (wasEmpty != materialAssignment.m_matModUvOverrides.empty())
            {
                materialAssignment.RebuildInstance();
                QueueMaterialUpdateNotification();
            }

            QueuePropertyChanges(materialAssignmentId);
        }

        AZ::RPI::MaterialModelUvOverrideMap MaterialComponentController::GetModelUvOverrides(
            const MaterialAssignmentId& materialAssignmentId) const
        {
            const auto materialIt = m_configuration.m_materials.find(materialAssignmentId);
            return materialIt != m_configuration.m_materials.end() ? materialIt->second.m_matModUvOverrides : AZ::RPI::MaterialModelUvOverrideMap();
        }

        void MaterialComponentController::OnMaterialAssignmentsChanged()
        {
            for (const auto& materialPair : m_configuration.m_materials)
            {
                if (materialPair.second.RequiresLoading())
                {
                    LoadMaterials();
                    return;
                }
            }
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
    } // namespace Render
} // namespace AZ
