/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/Math/Color.h>

#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

#include <PxPhysicsAPI.h>

namespace Physics
{
    class MaterialSlots;
};

namespace PhysX
{
    //! Enumeration that determines how two materials properties are combined when
    //! processing collisions.
    enum class CombineMode : AZ::u8
    {
        Average,
        Minimum,
        Maximum,
        Multiply
    };

    //! Runtime PhysX material instance.
    //! It handles the reloading of its data if the material asset it
    //! was created from is modified.
    //! It also provides functions to create PhysX materials.
    class Material
        : public Physics::Material
        , public AZStd::enable_shared_from_this<Material>
        , protected AZ::Data::AssetBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(Material, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysX::Material, "{57A9681F-4025-4D66-891B-80CBC78BDEB9}", Physics::Material);

        //! Function to create a material instance from an asset.
        //! The material id will be constructed from the asset id.
        //! If the material id is found in the manager it returns the existing material instance.
        //! @param materialAsset Material asset to create the material instance from.
        //! @return Material instance created or found. It can return nullptr if the creation failed or if the asset passed is invalid.
        static AZStd::shared_ptr<Material> FindOrCreateMaterial(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        //! Function to create material instances from material slots.
        //! The material ids will be constructed from the asset ids of the assets assigned to the slots.
        //! It will alway retun a valid list of materials, the slots with invalid or no assets will put
        //! the default material instance.
        //! @param materialSlots Material slots with the list of material assets to create the material instances from.
        //! @return List of material instances created. It will always return a valid list.
        static AZStd::vector<AZStd::shared_ptr<Material>> FindOrCreateMaterials(const Physics::MaterialSlots& materialSlots);

        //! Function to create a material instance from an asset.
        //! A random material will be used. This function is useful to create several instances from the same asset.
        //! @param materialAsset Material asset to create the material instance from.
        //! @return Material instance created. It can return nullptr if the creation failed or if the asset passed is invalid.
        static AZStd::shared_ptr<Material> CreateMaterialWithRandomId(const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        static constexpr float MinDensityLimit = 0.01f; //!< Minimum possible value of density.
        static constexpr float MaxDensityLimit = 100000.0f; //!< Maximum possible value of density.

        ~Material() override;

        // Physics::Material overrides ...
        float GetProperty(const AZStd::string& propertyName) const override;
        void SetProperty(const AZStd::string& propertyName, float value) override;

        float GetDynamicFriction() const;
        void SetDynamicFriction(float dynamicFriction);

        float GetStaticFriction() const;
        void SetStaticFriction(float staticFriction);

        float GetRestitution() const;
        void SetRestitution(float restitution);

        CombineMode GetFrictionCombineMode() const;
        void SetFrictionCombineMode(CombineMode mode);

        CombineMode GetRestitutionCombineMode() const;
        void SetRestitutionCombineMode(CombineMode mode);

        float GetDensity() const;
        void SetDensity(float density);

        const AZ::Color& GetDebugColor() const;
        void SetDebugColor(const AZ::Color& debugColor);

        const physx::PxMaterial* GetPxMaterial() const;

    protected:
        // AssetBus overrides...
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

    private:
        friend class MaterialManager;

        Material(
            const Physics::MaterialId& id,
            const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset);

        using PxMaterialUniquePtr = AZStd::unique_ptr<physx::PxMaterial, AZStd::function<void(physx::PxMaterial*)>>;

        PxMaterialUniquePtr m_pxMaterial;
        float m_density = 1000.0f;
        AZ::Color m_debugColor = AZ::Colors::White;
    };
} // namespace PhysX
