/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        //! Can be paired with renderable components (MeshComponent for example)
        //! to provide material overrides on a per-entity basis.
        class MaterialComponentController final
            : MaterialComponentRequestBus::Handler
            , MaterialReceiverNotificationBus::Handler
            , Data::AssetBus::MultiHandler
            , TickBus::Handler
        {
        public:
            friend class EditorMaterialComponent;

            AZ_CLASS_ALLOCATOR(MaterialComponentController, AZ::SystemAllocator, 0);
            AZ_RTTI(MaterialComponentController, "{34AD7ED0-9866-44CD-93B6-E86840214B91}");

            static void Reflect(ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            MaterialComponentController() = default;
            MaterialComponentController(const MaterialComponentConfig& config);

            void Activate(AZ::EntityId entityId);
            void Deactivate();
            void SetConfiguration(const MaterialComponentConfig& config);
            const MaterialComponentConfig& GetConfiguration() const;

            //! MaterialComponentRequestBus overrides...
            MaterialAssignmentMap GetOriginalMaterialAssignments() const override;
            MaterialAssignmentId FindMaterialAssignmentId(const MaterialAssignmentLodIndex lod, const AZStd::string& label) const override;
            AZ::Data::AssetId GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const override;
            AZStd::string GetMaterialSlotLabel(const MaterialAssignmentId& materialAssignmentId) const override;
            void SetMaterialOverrides(const MaterialAssignmentMap& materials) override;
            const MaterialAssignmentMap& GetMaterialOverrides() const override;
            void ClearAllMaterialOverrides() override;
            void ClearModelMaterialOverrides() override;
            void ClearLodMaterialOverrides() override;
            void ClearIncompatibleMaterialOverrides() override;
            void ClearInvalidMaterialOverrides() override;
            void RepairInvalidMaterialOverrides() override;
            uint32_t ApplyAutomaticPropertyUpdates() override;
            void SetDefaultMaterialOverride(const AZ::Data::AssetId& materialAssetId) override;
            const AZ::Data::AssetId GetDefaultMaterialOverride() const override;
            void ClearDefaultMaterialOverride() override;
            void SetMaterialOverride(const MaterialAssignmentId& materialAssignmentId, const AZ::Data::AssetId& materialAssetId) override;
            AZ::Data::AssetId GetMaterialOverride(const MaterialAssignmentId& materialAssignmentId) const override;
            void ClearMaterialOverride(const MaterialAssignmentId& materialAssignmentId) override;
            void SetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value) override;
            AZStd::any GetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const override;
            void ClearPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) override;
            void ClearPropertyOverrides(const MaterialAssignmentId& materialAssignmentId) override;
            void ClearAllPropertyOverrides() override;
            void SetPropertyOverrides(
                const MaterialAssignmentId& materialAssignmentId, const MaterialPropertyOverrideMap& propertyOverrides) override;
            MaterialPropertyOverrideMap GetPropertyOverrides(const MaterialAssignmentId& materialAssignmentId) const override;
            void SetModelUvOverrides(
                const MaterialAssignmentId& materialAssignmentId, const AZ::RPI::MaterialModelUvOverrideMap& modelUvOverrides) override;
            AZ::RPI::MaterialModelUvOverrideMap GetModelUvOverrides(const MaterialAssignmentId& materialAssignmentId) const override;

            //! MaterialReceiverNotificationBus::Handler overrides...
            void OnMaterialAssignmentsChanged() override;

        private:

            AZ_DISABLE_COPY(MaterialComponentController);

            //! Data::AssetBus overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // AZ::TickBus overrides...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            void LoadMaterials();
            void InitializeMaterialInstance(const Data::Asset<Data::AssetData>& asset);
            void ReleaseMaterials();
            //! Queue applying property overrides to material instances until tick
            void QueuePropertyChanges(const MaterialAssignmentId& materialAssignmentId);
            //! Queue material instance recreation notifiucations until tick
            void QueueMaterialUpdateNotification();

            EntityId m_entityId;
            MaterialComponentConfig m_configuration;
            AZStd::unordered_set<MaterialAssignmentId> m_materialsWithDirtyProperties;
            bool m_queuedMaterialUpdateNotification = false;
        };
    } // namespace Render
} // namespace AZ
