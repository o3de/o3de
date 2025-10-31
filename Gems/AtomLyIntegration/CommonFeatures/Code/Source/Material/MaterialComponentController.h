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
            , MaterialConsumerNotificationBus::Handler
            , Data::AssetBus::MultiHandler
            , SystemTickBus::Handler
        {
        public:
            friend class EditorMaterialComponent;

            AZ_CLASS_ALLOCATOR(MaterialComponentController, AZ::SystemAllocator);
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
            MaterialAssignmentMap GetDefaultMaterialMap() const override;
            MaterialAssignmentId FindMaterialAssignmentId(const MaterialAssignmentLodIndex lod, const AZStd::string& label) const override;
            AZ::Data::AssetId GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const override;
            bool IsDefaultMaterialAssetReady(const MaterialAssignmentId& materialAssignmentId) const override;
            AZStd::string GetMaterialLabel(const MaterialAssignmentId& materialAssignmentId) const override;
            void SetMaterialMap(const MaterialAssignmentMap& materials) override;
            const MaterialAssignmentMap& GetMaterialMap() const override;
            MaterialAssignmentMap GetMaterialMapCopy() const override;
            void ClearMaterialMap() override;
            void ClearMaterialsOnModelSlots() override;
            void ClearMaterialsOnLodSlots() override;
            void ClearMaterialsOnInvalidSlots() override;
            void ClearMaterialsWithMissingAssets() override;
            void RepairMaterialsWithMissingAssets() override;
            uint32_t RepairMaterialsWithRenamedProperties() override;
            void SetMaterialAssetIdOnDefaultSlot(const AZ::Data::AssetId& materialAssetId) override;
            const AZ::Data::AssetId GetMaterialAssetIdOnDefaultSlot() const override;
            void ClearMaterialAssetIdOnDefaultSlot() override;
            void SetMaterialAssetId(const MaterialAssignmentId& materialAssignmentId, const AZ::Data::AssetId& materialAssetId) override;
            AZ::Data::AssetId GetMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const override;
            bool IsMaterialAssetReady(const MaterialAssignmentId& materialAssignmentId) const override;
            void ClearMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) override;
            bool IsMaterialAssetIdOverridden(const MaterialAssignmentId& materialAssignmentId) const override;
            bool HasPropertiesOverridden(const MaterialAssignmentId& materialAssignmentId) const override;
            void SetPropertyValue(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value) override;
            AZStd::any GetPropertyValue(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const override;
            void ClearPropertyValue(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) override;
            void ClearPropertyValues(const MaterialAssignmentId& materialAssignmentId) override;
            void ClearAllPropertyValues() override;
            void SetPropertyValues(
                const MaterialAssignmentId& materialAssignmentId, const MaterialPropertyOverrideMap& propertyOverrides) override;
            MaterialPropertyOverrideMap GetPropertyValues(const MaterialAssignmentId& materialAssignmentId) const override;
            void SetModelUvOverrides(
                const MaterialAssignmentId& materialAssignmentId, const AZ::RPI::MaterialModelUvOverrideMap& modelUvOverrides) override;
            AZ::RPI::MaterialModelUvOverrideMap GetModelUvOverrides(const MaterialAssignmentId& materialAssignmentId) const override;

            //! MaterialConsumerNotificationBus::Handler overrides...
            void OnMaterialAssignmentSlotsChanged() override;

        private:

            AZ_DISABLE_COPY(MaterialComponentController);

            //! Data::AssetBus overrides...
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetError(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReloadError(Data::Asset<Data::AssetData> asset) override;

            // AZ::SystemTickBus overrides...
            void OnSystemTick() override;

            void LoadMaterials();
            // Typically called from thread context of Data::AssetBus::MultiHandler::OnAssetXXX.
            void InitializeMaterialInstance(Data::Asset<Data::AssetData> asset);
            // Must be called on main thread.
            // Called for each asset in @m_notifiedMaterialAssets only during SystemTick.
            void InitializeNotifiedMaterialAsset(Data::Asset<Data::AssetData> asset);
            void ReleaseMaterials();
            //! Queue applying property overrides to material instances until tick
            void QueuePropertyChanges(const MaterialAssignmentId& materialAssignmentId);
            //! Queue material instance creation notifications until tick
            void QueueMaterialsCreatedNotification();
            //! Queue material instance recreation notifications until tick
            void QueueMaterialsUpdatedNotification();
            //! Queue material reload so that it only occurs once per tick
            void QueueLoadMaterials();

            //! Converts property overrides storing image asset references into asset IDs. This addresses a problem where image property
            //! overrides are lost during prefab serialization and patching. This suboptimal function will be removed once the underlying
            //! problem is resolved.
            void ConvertAssetsForSerialization();
            void ConvertAssetsForSerialization(MaterialPropertyOverrideMap& propertyMap);
            AZStd::any ConvertAssetsForSerialization(const AZStd::any& value) const;

            void DisplayMissingAssetWarning(Data::Asset<Data::AssetData> asset) const;

            EntityId m_entityId;
            MaterialComponentConfig m_configuration;
            MaterialAssignmentMap m_defaultMaterialMap;
            AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::RPI::MaterialAsset>> m_uniqueMaterialMap;
            AZStd::unordered_set<MaterialAssignmentId> m_materialsWithDirtyProperties;
            //! We store here references to all the material assets for which we are connected to the
            //! AssetBus for notifications. Instead of taking action upon being notified, we simply store
            //! the asset here, and in the next SystemTick we'll process the assets. This is done, for the following reason:
            //! When AssetBus::OnAssetXXX functions are called, a deadlock may occur according to the following sequence:
            // 1. Starting from Main thread, AssetBus locks a mutex.
            // 2. AssetBus calls OnAssetReady and it enters in this function.
            // 3. Start the instantiation of a new StreamingImage.
            // 4. StreamingImage asynchronously queues work in the "Seconday Copy Queue".
            // 5. StreamingImage waits until the work completes.
            // 6. The thread of "Seconday Copy Queue" gets a new work item, which may hold a reference
            //    to an old StreamingImage.
            // 7. The old StreamingImage gets destroyed and it calls AssetBus::MultiHandler::BusDisconnect(GetAssetId());
            // 8. When calling AssetBus::MultiHandler::BusDisconnect(GetAssetId()); it tries to lock the same mutex
            //    from step 1. But the mutex is already locked on Main Thread in step 1.
            // 9. The "Seconday Copy Queue" thread deadlocks and never completes the work.
            // 10. Main thread is also deadlocked waiting for "Seconday Copy Queue" to complete.
            // The solution is to enqueue texture update on the next tick.
            AZStd::queue<Data::Asset<Data::AssetData>> m_notifiedMaterialAssets;
            bool m_queuedMaterialsCreatedNotification = false;
            bool m_queuedMaterialsUpdatedNotification = false;
            bool m_queuedLoadMaterials = false;
        };
    } // namespace Render
} // namespace AZ
