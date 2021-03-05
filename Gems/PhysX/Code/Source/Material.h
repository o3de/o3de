/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/MaterialBus.h>
#include <AzCore/Asset/AssetCommon.h>
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
        void SetSurfaceType(AZ::Crc32 surfaceType) override;

        const AZStd::string& GetSurfaceTypeName() const override { return m_surfaceString; }

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

        AZ::u32 GetCryEngineSurfaceId() const override;

        void* GetNativePointer() override;

    private:
        using PxMaterialUniquePtr = AZStd::unique_ptr<physx::PxMaterial, AZStd::function<void(physx::PxMaterial*)>>;

        PxMaterialUniquePtr m_pxMaterial;
        AZ::Crc32 m_surfaceType = 0;
        AZ::u32 m_cryEngineSurfaceId = -1;
        AZStd::string m_surfaceString;
        float m_density = 1000.0f;
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

        virtual void GetPxMaterials(const Physics::MaterialSelection& materialSelection, AZStd::vector<physx::PxMaterial*>& outMaterials) = 0;

        virtual const AZStd::shared_ptr<Material>& GetDefaultMaterial() = 0;
        virtual void ReleaseAllMaterials() = 0;
    };
    using MaterialManagerRequestsBus = AZ::EBus<MaterialManagerRequests>;

    /// Manages materials created from MaterialLibraryAsset
    /// ===================================================
    ///
    /// Material managers creates PhysX::Material instances from MaterialLibraryAsset and assumes their ownership.
    /// Also keeps a reference to the default material.
    class MaterialsManager
        : public MaterialManagerRequestsBus::Handler
        , public Physics::PhysicsMaterialRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialsManager, AZ::SystemAllocator, 0);
        AZ_RTTI(MaterialsManager, "{4A6E59A7-D41A-470A-B31D-622BDA207FC7}");

        MaterialsManager();
        ~MaterialsManager() override;

        /// Returns a vector of weak pointers to materials selected. 
        /// To be notified if the pointers are deleted, connect to PhysicsMaterialNotifications::MaterialsReleased().
        /// @param materialSelection MaterialSelection instance to create or get materials for.
        /// @param outMaterials Collection of material weak pointers corresponding to the material selection to be returned.
        void GetMaterials(const Physics::MaterialSelection& materialSelection
            , AZStd::vector<AZStd::weak_ptr<Physics::Material>>& outMaterials) override;

        /// Returns a weak pointer to physics material with the given name.
        AZStd::weak_ptr<Physics::Material> GetMaterialByName(const AZStd::string& name) override;

        /// Returns index of selected material in its material library. 0 is the Default material.
        /// @param materialSelection Selection of materials.
        AZ::u32 GetFirstSelectedMaterialIndex(const Physics::MaterialSelection& materialSelection) override;

        /// Slightly faster version of GetMaterials that returns physx::PxMaterial pointers instead. \n
        /// The rest is equivalent to GetMaterials function.
        /// @param materialSelection MaterialSelection instance to create or get materials for
        /// @param outMaterials vector of pointers to physx::PxMaterial to fill with. The vector will be cleared inside the function.
        void GetPxMaterials(const Physics::MaterialSelection& materialSelection, AZStd::vector<physx::PxMaterial*>& outMaterials) override;

        /// Returns default material
        /// @return default PhysX::Material instance
        const AZStd::shared_ptr<Material>& GetDefaultMaterial() override;

        /// Return default material
        /// @return default Physics::Material instance
        AZStd::shared_ptr<Physics::Material> GetGenericDefaultMaterial() override;

        /// Releases ownership of all materials created before.
        void ReleaseAllMaterials() override;

        /// Connect to any necessary buses
        void Connect();

        /// Disconnect from any necessary buses
        void Disconnect();

    private:
        void InitializeMaterials(const Physics::MaterialSelection& materialSelection);

        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<Material>> m_materialsFromAssets;
        AZStd::shared_ptr<Material> m_defaultMaterial;
    };
}
