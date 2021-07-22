/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>

#include <NvBlastExtDamageShaders.h>
#include <NvBlastExtStressSolver.h>

namespace AZ
{
    class ReflectContext;
}

namespace Blast
{
    class BlastMaterialConfiguration;

    namespace Attributes
    {
        const static AZ::Crc32 BlastMaterialLibraryAssetId = AZ_CRC("BlastMaterialAssetId");
    }

    //! Blast material.
    //! Wrapper around NvBlastExtMaterial.
    class BlastMaterial
    {
    public:
        BlastMaterial(const BlastMaterialConfiguration& configuration);

        const AZStd::string& GetMaterialName() const;

        //! Amount of damage destructible object with this material can withstand.
        float GetHealth() const;
        //! Amount by which magnitude of stress forces applied is divided before being subtracted from health.
        float GetForceDivider() const;
        //! Any amount lower than this threshold will not be applied. Only affects non-stress damage.
        float GetMinDamageThreshold() const;
        //! Any amount higher than this threshold will be capped by it. Only affects non-stress damage.
        float GetMaxDamageThreshold() const;
        //! Factor with which linear stress is applied to destructible objects. Linear stress includes
        //! direct application of BlastFamilyDamageRequests::StressDamage, collisions and gravity (only for static
        //! actors).
        float GetStressLinearFactor() const;
        //! Factor with which angular stress is applied to destructible objects. Angular stress is
        //! calculated based on angular velocity of an object (only non-static actors).
        float GetStressAngularFactor() const;
        //! Normalizes the non-stress damage based on the thresholds.
        float GetNormalizedDamage(float damage) const;
        //! Generates NvBlast stress solver settings from this material and provided iterationsCount.
        Nv::Blast::ExtStressSolverSettings GetStressSolverSettings(uint32_t iterationsCount) const;

        //! Returns underlying pointer of the native type.
        void* GetNativePointer();

    private:
        NvBlastExtMaterial m_material;
        AZStd::string m_name;
        float m_health;
        float m_stressLinearFactor;
        float m_stressAngularFactor;
    };

    //! Default values used for initializing materials.
    //! Use BlastMaterialConfiguration to define properties for materials at the time of creation.
    class BlastMaterialConfiguration
    {
    public:
        AZ_TYPE_INFO(BlastMaterialConfiguration, "{BEC875B1-26E4-4A4A-805E-0E880372720D}");

        static void Reflect(AZ::ReflectContext* context);

        float m_health = 1.0;
        float m_forceDivider = 1.0;
        float m_minDamageThreshold = 0.0;
        float m_maxDamageThreshold = 1.0;
        float m_stressLinearFactor = 1.0f;
        float m_stressAngularFactor = 1.0f;

        AZStd::string m_materialName{"Default"};
    };

    //! Class that is used to identify the material in the collection of materials
    //! Collection of the materials is stored in BlastMaterialLibraryAsset.
    class BlastMaterialId
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastMaterialId, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(BlastMaterialId, "{BDB30505-C93E-4A83-BDD7-41027802DE0A}");
        static void Reflect(AZ::ReflectContext* context);

        static BlastMaterialId Create();
        bool IsNull() const
        {
            return m_id.IsNull();
        }
        bool operator==(const BlastMaterialId& other) const
        {
            return m_id == other.m_id;
        }
        const AZ::Uuid& GetUuid() const
        {
            return m_id;
        }

    private:
        AZ::Uuid m_id = AZ::Uuid::CreateNull();
    };

    //! A single BlastMaterial entry in the material library
    //! BlastMaterialLibraryAsset holds a collection of BlastMaterialFromAssetConfiguration instances.
    class BlastMaterialFromAssetConfiguration
    {
    public:
        AZ_TYPE_INFO(BlastMaterialFromAssetConfiguration, "{E380E174-BCA3-4BBB-AA39-8FAD39005B12}");

        static void Reflect(AZ::ReflectContext* context);

        BlastMaterialConfiguration m_configuration;
        BlastMaterialId m_id;
    };

    //! An asset that holds a list of materials to be edited and assigned in Open 3D Engine Editor
    //! Use Asset Editor to create a BlastMaterialLibraryAsset and add materials to it.
    //! Please note, BlastMaterialLibraryAsset is used only to provide a way to edit materials in the
    //! Editor, if you need to create materials at runtime (for example, from custom configuration files)
    //! please use Blast::BlastMaterial class directly.
    class BlastMaterialLibraryAsset : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastMaterialLibraryAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(BlastMaterialLibraryAsset, "{55F38C86-0767-4E7F-830A-A4BF624BE4DA}", AZ::Data::AssetData);

        BlastMaterialLibraryAsset() = default;
        virtual ~BlastMaterialLibraryAsset() = default;

        static void Reflect(AZ::ReflectContext* context);

        //! Finds BlastMaterialFromAssetConfiguration in the library given BlastMaterialId
        //! @param materialId material id to find the configuration for
        //! @param data stores the material data if material with such ID exists
        //! @return true if BlastMaterialFromAssetConfiguration was found, False otherwise
        bool GetDataForMaterialId(
            const BlastMaterialId& materialId, BlastMaterialFromAssetConfiguration& configuration) const;

        //! Retrieves if there is any data with the given BlastMaterialId
        //! @param materialId material id to find
        //! @return true if material with that id was found, False otherwise
        bool HasDataForMaterialId(const BlastMaterialId& materialId) const;

        //! Finds BlastMaterialFromAssetConfiguration in the library given material name
        //! @param materialName material name to find the configuration for
        //! @return true if BlastMaterialFromAssetConfiguration was found, false otherwise
        bool GetDataForMaterialName(
            const AZStd::string& materialName, BlastMaterialFromAssetConfiguration& configuration) const;

        //! Adds material data to the asset library.
        //! If BlastMaterialId is not set, it'll be generated automatically.
        //! If BlastMaterialId is set and is unique for this collection it'll be added to the library unchanged.
        //! If BlastMaterialId is set and is not unique nothing happens.
        //! @param data BlastMaterial data to add
        void AddMaterialData(const BlastMaterialFromAssetConfiguration& data);

        //! Returns all BlastMaterialFromAssetConfiguration instances from this library
        //! @return a Vector of all BlastMaterialFromAssetConfiguration stored in this library
        const AZStd::vector<BlastMaterialFromAssetConfiguration>& GetMaterialsData() const
        {
            return m_materialLibrary;
        }

    protected:
        friend class BlastMaterialLibraryAssetEventHandler;
        void GenerateMissingIds();
        AZStd::vector<BlastMaterialFromAssetConfiguration> m_materialLibrary;
    };

    //! Class that describes configuration of the rigid bodies used in Blast Actors.
    class BlastActorConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(BlastActorConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(BlastActorConfiguration, "{949E731B-0418-4B70-8969-2871F66CF463}");
        static void Reflect(AZ::ReflectContext* context);

        BlastActorConfiguration() = default;
        BlastActorConfiguration(const BlastActorConfiguration&) = default;
        virtual ~BlastActorConfiguration() = default;

        AzPhysics::CollisionLayer m_collisionLayer; //! Which collision layer is this actor's collider on.
        AzPhysics::CollisionGroups::Id m_collisionGroupId; //! Which layers does this actor's collider collide with.
        bool m_isSimulated = true; //! Should this actor's shapes partake in collision in the physical simulation.
        bool m_isInSceneQueries =
            true; //! Should this actor's shapes partake in scene queries (ray casts, overlap tests, sweeps).
        bool m_isCcdEnabled = true; //! Should this actor's rigid body be using CCD.
        AZStd::string m_tag; //! Identification tag for the collider
    };
} // namespace Blast
