/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/MaterialBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

namespace PhysX
{
    /// PhysX implementation of Physics::Material interface
    /// ===================================================
    /// 
    /// PhysX::Material stores a reference to physx::PxMaterial and manages its lifetime
    /// The userData pointer of PxMaterial points to this class.
    /// 
    /// Usage example
    /// -------------------------
    /// You can construct instance of PhysX::Material directly and retrieve PxMaterial reference from it
    ///
    ///     Physics::MaterialConfiguration properties;
    ///     Material newMaterial{properties};
    ///     physx::PxMaterial* material = newMaterial.GetPxMaterial();
    ///
    /// You can use retrieved PxMaterial pointer on its own, provided you increment its reference count.
    /// If this class goes out of scope, the PxMaterial pointer will be valid, but its userData 
    /// will be cleaned up to point to nullptr.
    class Material
        : public Physics::Material
        , public AZStd::enable_shared_from_this<Material>
    {
    public:
        AZ_CLASS_ALLOCATOR(Material, AZ::SystemAllocator, 0);
        AZ_RTTI(Material, "{F5497337-DCFE-44BA-BB40-B9EF225D16D6}", Physics::Material);

        Material(const Physics::MaterialConfiguration& configuration);
        Material(Material&& material);
        virtual ~Material() = default;
        Material& operator=(Material&& material);
        Material(const Material& shape) = delete;
        Material& operator=(const Material& shape) = delete;

        void UpdateWithConfiguration(const Physics::MaterialConfiguration& configuration);

        physx::PxMaterial* GetPxMaterial();

        // Physics::Material
        AZ::Crc32 GetSurfaceType() const override;

        const AZStd::string& GetSurfaceTypeName() const override;
        void SetSurfaceTypeName(const AZStd::string& surfaceTypeName) override;

        float GetDynamicFriction() const override;
        void SetDynamicFriction(float dynamicFriction) override;

        float GetStaticFriction() const override;
        void SetStaticFriction(float staticFriction) override;

        float GetRestitution() const override;
        void SetRestitution(float restitution) override;

        CombineMode GetFrictionCombineMode() const override;
        void SetFrictionCombineMode(CombineMode mode) override;

        CombineMode GetRestitutionCombineMode() const override;
        void SetRestitutionCombineMode(CombineMode mode) override;

        float GetDensity() const override;
        void SetDensity(float density) override;

        AZ::Color GetDebugColor() const override;
        void SetDebugColor(const AZ::Color& debugColor) override;

        AZ::u32 GetCryEngineSurfaceId() const override;

        void* GetNativePointer() override;

    private:
        using PxMaterialUniquePtr = AZStd::unique_ptr<physx::PxMaterial, AZStd::function<void(physx::PxMaterial*)>>;

        PxMaterialUniquePtr m_pxMaterial;
        AZ::Crc32 m_surfaceType = 0;
        AZ::u32 m_cryEngineSurfaceId = -1;
        AZStd::string m_surfaceString;
        float m_density = 1000.0f;
        AZ::Color m_debugColor = AZ::Colors::White;
    };

    /// Bus with requests to MaterialsManager
    /// =====================================
    ///
    /// Can be used to retrieve material instance from user selection.\n
    /// Refer to MaterialsManager for more details. \n
    class MaterialManagerRequests
        : public Physics::PhysicsMaterialRequests
    {
    public:
        using MutexType = AZStd::mutex;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Returns weak pointers to physx::PxMaterial.
        /// Equivalent to PhysicsMaterialRequests::GetMaterials but it returns physx::PxMaterial pointers instead.
        /// @param materialSelection MaterialSelection instance to create or get materials for
        /// @param outMaterials vector of pointers to physx::PxMaterial to fill with. The vector will be cleared inside the function.
        virtual void GetPxMaterials(const Physics::MaterialSelection& materialSelection, AZStd::vector<physx::PxMaterial*>& outMaterials) = 0;

        /// Returns default material
        /// @return default PhysX::Material instance
        virtual AZStd::shared_ptr<Material> GetDefaultMaterial() = 0;

        /// Releases ownership of all materials created before.
        virtual void ReleaseAllMaterials() = 0;
    };
    using MaterialManagerRequestsBus = AZ::EBus<MaterialManagerRequests>;

    /// Manages materials created from MaterialLibraryAsset
    /// ===================================================
    ///
    /// Material managers creates PhysX::Material instances from MaterialLibraryAsset and assumes their ownership.
    /// Also keeps a reference to the default material.
    ///
    /// Note: Materials will be created on the fly while doing queries and
    ///       they will be updated when the material library changes.
    class MaterialsManager
        : public MaterialManagerRequestsBus::Handler
        , public Physics::PhysicsMaterialRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialsManager, AZ::SystemAllocator, 0);
        AZ_RTTI(MaterialsManager, "{4A6E59A7-D41A-470A-B31D-622BDA207FC7}");

        MaterialsManager();
        ~MaterialsManager() override;

        // PhysicsMaterialRequestBus::Handler overrides...
        void GetMaterials(const Physics::MaterialSelection& materialSelection
            , AZStd::vector<AZStd::shared_ptr<Physics::Material>>& outMaterials) override;
        AZStd::shared_ptr<Physics::Material> GetMaterialById(Physics::MaterialId id) override;
        AZStd::shared_ptr<Physics::Material> GetMaterialByName(const AZStd::string& name) override;
        void UpdateMaterialSelectionFromPhysicsAsset(
            const Physics::ShapeConfiguration& shapeConfiguration,
            Physics::MaterialSelection& materialSelection) override;
        AZStd::shared_ptr<Physics::Material> GetGenericDefaultMaterial() override;

        // MaterialManagerRequestsBus::Handler overrides...
        void GetPxMaterials(const Physics::MaterialSelection& materialSelection, AZStd::vector<physx::PxMaterial*>& outMaterials) override;
        AZStd::shared_ptr<Material> GetDefaultMaterial() override;
        void ReleaseAllMaterials() override;

        /// Connect to any necessary buses
        void Connect();

        /// Disconnect from any necessary buses
        void Disconnect();

    private:
        using Materials = AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<Material>>;

        /// Search a material by id, if it exists already it returns its iterator,
        /// if it doesn't exist it tries to create it and add it to the list.
        /// If the material id is null or not part of the material library then the
        /// iterator returned is end of material list.
        Materials::iterator FindOrCreateMaterial(Physics::MaterialId materialId);

        /// Search a material by name, if it exists already it returns its iterator,
        /// if it doesn't exist it tries to create it and add it to the list.
        /// If the material id is null or not part of the material library then the
        /// iterator returned is end of material list.
        Materials::iterator FindOrCreateMaterial(const AZStd::string& materialName);

        /// Returns the material library of the project.
        Physics::MaterialLibraryAsset* GetMaterialLibrary();

        void OnPhysicsConfigurationChanged(const AzPhysics::SystemConfiguration* config);
        void OnMaterialLibraryChanged(const AZ::Data::AssetId& materialLibraryAssetId);

        Materials m_materials;
        AZStd::shared_ptr<Material> m_defaultMaterial;
        Physics::MaterialConfiguration m_defaultMaterialConfiguration;

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physicsConfigChangedHandler;
        AzPhysics::SystemEvents::OnMaterialLibraryChangedEvent::Handler m_materialLibraryChangedHandler;
    };
}
