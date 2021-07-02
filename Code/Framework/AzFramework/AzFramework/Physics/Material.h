/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    static constexpr AZStd::string_view DefaultPhysicsMaterialLabel = "<Default Physics Material>";

    /// Physics material
    /// =========================
    /// This is the interface to the wrapper around native material type (such as PxMaterial in PhysX gem)
    /// that stores extra metadata, like Surface Type name.
    ///
    /// Usage example
    /// -------------------------
    /// Create new material using Physics::SystemRequestBus and Physics::MaterialConfiguration:
    ///
    ///     Physics::MaterialConfiguration materialProperties;
    ///     AZStd::shared_ptr<Physics::Material> newMaterial = AZ::Interface<Physics::System>::Get()->CreateMaterial(materialProperties);
    /// 
    class Material
    {
    public:
        AZ_CLASS_ALLOCATOR(Material, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::Material, "{44636CEA-46DD-4D4A-B1EF-5ED6DEA7F714}");

        /// Enumeration that determines how two materials properties are combined when
        /// processing collisions.
        enum class CombineMode : AZ::u8
        {
            Average,
            Minimum,
            Maximum,
            Multiply
        };

        /// Returns AZ::Crc32 of the surface name.
        virtual AZ::Crc32 GetSurfaceType() const = 0;

        virtual const AZStd::string& GetSurfaceTypeName() const = 0;
        virtual void SetSurfaceTypeName(const AZStd::string& surfaceTypeName) = 0;

        virtual float GetDynamicFriction() const = 0;
        virtual void SetDynamicFriction(float dynamicFriction) = 0;

        virtual float GetStaticFriction() const = 0;
        virtual void SetStaticFriction(float staticFriction) = 0;

        virtual float GetRestitution() const = 0;
        virtual void SetRestitution(float restitution) = 0;

        virtual CombineMode GetFrictionCombineMode() const = 0;
        virtual void SetFrictionCombineMode(CombineMode mode) = 0;

        virtual CombineMode GetRestitutionCombineMode() const = 0;
        virtual void SetRestitutionCombineMode(CombineMode mode) = 0;

        virtual float GetDensity() const = 0;
        virtual void SetDensity(float density) = 0;

        virtual AZ::Color GetDebugColor() const = 0;
        virtual void SetDebugColor(const AZ::Color& debugColor) = 0;

        /// If the name of this material matches the name of one of the CrySurface types, it will return its CrySurface Id.\n
        /// If there's no match it will return default CrySurface Id.\n
        /// CrySurface types are defined in libs/materialeffects/surfacetypes.xml
        virtual AZ::u32 GetCryEngineSurfaceId() const = 0;

        /// Returns underlying pointer of the native physics type (for example PxMaterial in PhysX).
        virtual void* GetNativePointer() = 0;
    };

    /// Default values used for initializing materials
    /// ===================
    ///
    /// Use MaterialConfiguration to define properties for materials at the time of creation. \n
    /// Use Physics::SystemRequestBus to create new materials.
    class MaterialConfiguration
    {
    public:
        AZ_RTTI(Physics::MaterialConfiguration, "{8807CAA1-AD08-4238-8FDB-2154ADD084A1}");

        static void Reflect(AZ::ReflectContext* context);

        MaterialConfiguration() = default;
        virtual ~MaterialConfiguration() = default;

        const static AZ::Crc32 s_stringGroup; ///< Edit context data attribute. Identifies a string group instance. String values in the same group are unique.
        const static AZ::Crc32 s_forbiddenStringSet; ///<  Edit context data attribute. A set of strings that are not acceptable as values to the data element. Can be AZStd::unordered_set<AZStd::string>, AZStd::set<AZStd::string>, AZStd::vector<AZStd::string>
        const static AZ::Crc32 s_configLineEdit; ///< Edit context data element handler. Creates custom line edit widget that allows string values to be unique in a group.
        static constexpr float MinDensityLimit = 0.01f; //!< Minimum possible value of density.
        static constexpr float MaxDensityLimit = 100000.0f; //!< Maximum possible value of density.

        AZStd::string m_surfaceType{ "Default" };
        float m_dynamicFriction = 0.5f;
        float m_staticFriction = 0.5f;
        float m_restitution = 0.5f;
        float m_density = 1000.0f;

        Material::CombineMode m_restitutionCombine = Material::CombineMode::Average;
        Material::CombineMode m_frictionCombine = Material::CombineMode::Average;

        AZ::Color m_debugColor = AZ::Colors::White;

        bool operator==(const MaterialConfiguration& other) const;
        bool operator!=(const MaterialConfiguration& other) const;

    protected:
        virtual bool IsNameReadOnly() const { return false; }

    private:
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static AZ::Color GenerateDebugColor(const char* materialName);
    };

    class DefaultMaterialConfiguration
        : public MaterialConfiguration
    {
    public:
        AZ_RTTI(Physics::DefaultMaterialConfiguration, "{A1F64C5C-D413-4757-9D42-51DD0EBFC270}", Physics::MaterialConfiguration);

        static void Reflect(AZ::ReflectContext* context);
        
        DefaultMaterialConfiguration();

    protected:
        bool IsNameReadOnly() const override { return true; }
    };

    namespace Attributes
    {
        const static AZ::Crc32 MaterialLibraryAssetId = AZ_CRC("MaterialAssetId", 0x4a88a3f5);
    }

    /// Class that is used to identify the material in the collection of materials
    /// ============================================================
    ///
    /// Collection of the materials is stored in MaterialLibraryAsset.
    class MaterialId
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialId, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Physics::MaterialId, "{744CCE6C-9F69-4E2F-B950-DAB8514F870B}");
        static void Reflect(AZ::ReflectContext* context);

        static MaterialId Create();
        static MaterialId FromUUID(const AZ::Uuid& uuid);
        bool IsNull() const { return m_id.IsNull(); }
        bool operator==(const MaterialId& other) const { return m_id == other.m_id; }
        bool operator!=(const MaterialId& other) const { return !(*this == other); }
        const AZ::Uuid& GetUuid() const { return m_id; }

    private:
        AZ::Uuid m_id = AZ::Uuid::CreateNull();
    };

    /// A single Material entry in the material library
    /// ===============================================
    ///
    /// MaterialLibraryAsset holds a collection of MaterialFromAssetConfiguration instances.
    class MaterialFromAssetConfiguration
    {
    public:
        AZ_TYPE_INFO(Physics::MaterialFromAssetConfiguration, "{FBD76628-DE57-435E-BE00-6FFAE64DDF1D}");

        static void Reflect(AZ::ReflectContext* context);

        MaterialConfiguration m_configuration;
        MaterialId m_id;

        bool operator==(const MaterialFromAssetConfiguration& other) const;
        bool operator!=(const MaterialFromAssetConfiguration& other) const;
    };

    /// An asset that holds a list of materials to be edited and assigned in Open 3D Engine Editor
    /// ======================================================================================
    ///
    /// Use Asset Editor to create a MaterialLibraryAsset and add materials to it.\n
    /// You can assign this library on primitive colliders, terrain layers, mesh colliders.
    /// You can later select a specific material out of the library.\n
    /// Please note, MaterialLibraryAsset is used only to provide a way to edit materials in the
    /// Editor, if you need to create materials at runtime (for example, from custom configuration files)
    /// please use Physics::Material class directly.
    class MaterialLibraryAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialLibraryAsset, AZ::SystemAllocator, 0);
        AZ_RTTI(Physics::MaterialLibraryAsset, "{9E366D8C-33BB-4825-9A1F-FA3ADBE11D0F}", AZ::Data::AssetData);

        MaterialLibraryAsset() = default;
        virtual ~MaterialLibraryAsset() = default;

        static void Reflect(AZ::ReflectContext* context);

        /// Finds MaterialFromAssetConfiguration in the library given MaterialId
        /// @param materialId material id to find the configuration for
        /// @param data stores the material data if material with such ID exists
        /// @return true if MaterialFromAssetConfiguration was found, False otherwise
        bool GetDataForMaterialId(const MaterialId& materialId, MaterialFromAssetConfiguration& configuration) const;

        /// Retrieves if there is any data with the given Material Id
        /// @param materialId material id to find
        /// @return true if material with that id was found, False otherwise
        bool HasDataForMaterialId(const MaterialId& materialId) const;

        /// Finds MaterialFromAssetConfiguration in the library given material name
        /// @param materialName material name to find the configuration for
        /// @param data stores the material data if material with such name exists
        /// @return true if MaterialFromAssetConfiguration was found, False otherwise
        bool GetDataForMaterialName(const AZStd::string& materialName, MaterialFromAssetConfiguration& configuration) const;

        /// Adds material data to the asset library.\n
        /// If MaterialId is not set, it'll be generated automatically.\n
        /// If MaterialId is set and is unique for this collection it'll be added to the library unchanged.\n
        /// @param data Material data to add
        void AddMaterialData(const MaterialFromAssetConfiguration& data);

        /// Returns all MaterialFromAssetConfiguration instances from this library
        /// @return a Vector of all MaterialFromAssetConfiguration stored in this library
        const AZStd::vector<MaterialFromAssetConfiguration>& GetMaterialsData() const { return m_materialLibrary; }

    protected:
        friend class MaterialLibraryAssetEventHandler;
        void GenerateMissingIds();
        AZStd::vector<MaterialFromAssetConfiguration> m_materialLibrary;
    };

    /// The class is used to expose a default material and material library asset to Edit Context
    /// =======================================================================
    ///
    /// Since AZ::Data::Asset doesn't reflect the data to EditContext 
    /// we have to have a wrapper doing it.
    class MaterialInfoReflectionWrapper
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialInfoReflectionWrapper, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Physics::MaterialInfoReflectionWrapper, "{02AB8CBC-D35B-4E0F-89BA-A96D94DAD4F9}");
        static void Reflect(AZ::ReflectContext* context);

        Physics::DefaultMaterialConfiguration m_defaultMaterialConfiguration;
        AZ::Data::Asset<Physics::MaterialLibraryAsset> m_materialLibraryAsset =
            AZ::Data::AssetLoadBehavior::NoLoad;
    };

    /// The class is used to store a vector of MaterialIds selected from the library
    /// =======================================================================
    ///
    /// This class is used to store the user's selection of the materials from this library.
    /// You can reflect this class in EditorContext to provide UI for selecting materials
    /// on any custom component or QWidget.
    class MaterialSelection
    {
        friend class MaterialSelectionEventHandler;
    public:
        AZ_CLASS_ALLOCATOR(MaterialSelection, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Physics::MaterialSelection, "{F571AFF4-C4BB-4590-A204-D11D9EEABBC4}");

        using SlotsArray = AZStd::vector<AZStd::string>;

        static void Reflect(AZ::ReflectContext* context);

        /// Sets an array of material slots to pick MaterialIds for. Having multiple slots is required for assigning multiple materials on a mesh 
        /// or heightfield object. SlotsArray can be empty and in this case Default slot will be created.
        /// @param slots Array of names for slots. Can be empty, in this case Default slot will be created
        void SetMaterialSlots(const SlotsArray& slots);

        /// Returns a list of MaterialId that were assigned for each corresponding slot.
        const AZStd::vector<Physics::MaterialId>& GetMaterialIdsAssignedToSlots() const;

        /// Sets the MaterialId as the selected material at a specific slotIndex.
        /// @param materialId MaterialId that user selected
        /// @param slotIndex Index of the slot to set the MaterialId
        void SetMaterialId(const Physics::MaterialId& materialId, int slotIndex = 0);

        /// Returns the material id assigned to this selection at a specific slotIndex.
        /// @param slotIndex Index of the slot to retrieve the MaterialId
        Physics::MaterialId GetMaterialId(int slotIndex = 0) const;

        /// Called when the material library has changed
        void OnMaterialLibraryChanged(const AZ::Data::AssetId& defaultMaterialLibraryId);

        /// Set if the material slots are editable in the edit context
        void SetSlotsReadOnly(bool readOnly);

    private:
        AZStd::vector<Physics::MaterialId> m_materialIdsAssignedToSlots;
        SlotsArray m_materialSlots;
        bool m_slotsReadOnly = false;
        
        void SyncSelectionToMaterialLibrary();
        
        static const AZ::Data::Asset<Physics::MaterialLibraryAsset>& GetMaterialLibrary();
        static const AZ::Data::AssetId& GetMaterialLibraryId();

        bool AreMaterialSlotsReadOnly() const;

        // EditorContext callbacks
        AZStd::string GetMaterialSlotLabel(int index);

        // Only used for Edit Context as it requires to have an asset reflected.
        // To get the material library use GetMaterialLibraryId()
        AZ::Data::Asset<Physics::MaterialLibraryAsset> m_editContextMaterialLibrary{ AZ::Data::AssetLoadBehavior::NoLoad };
    };

    /// Editor Bus used to assign material to terrain surface id.
    /// ========================================================
    ///
    /// Used by Terrain Layer Editor window to save material selection for a specific Terrain Layer. \n
    /// Must be used before cooking the terrain, in-game usage won't have any effect.
    class EditorTerrainMaterialRequests
        : public AZ::ComponentBus
    {
    public:

        virtual void SetMaterialSelectionForSurfaceId(int surfaceId, const MaterialSelection& selection) = 0;
        virtual bool GetMaterialSelectionForSurfaceId(int surfaceId, MaterialSelection& selection) = 0;

    protected:
        ~EditorTerrainMaterialRequests() = default;
    };

    using EditorTerrainMaterialRequestsBus = AZ::EBus<EditorTerrainMaterialRequests>;

    /// Alias for surface id to terrain material unordered map.
    using TerrainMaterialSurfaceIdMap = AZStd::unordered_map<int, Physics::MaterialSelection>;

    /// Bus that is used to retrieve SurfaceType id from the legacy material system
    /// ========================================================
    ///
    /// Returns CrySurfaceType Id given Crc32 of its name
    class LegacySurfaceTypeRequests
        : public AZ::EBusTraits
    {
    public:
        virtual ~LegacySurfaceTypeRequests() {}
        /// Returns CrySurfaceType Id given Crc32 of its name
        /// @param Crc32 hash of the CrySurfaceType name
        /// @return ID of the CrySurfaceType. If such type does not exists it will default to 0.
        virtual AZ::u32 GetLegacySurfaceType(AZ::Crc32 pxMaterialType) = 0;
        virtual AZ::u32 GetLegacySurfaceTypeFronName(const AZStd::string& pxMaterialTypeName) = 0;
    };

    using LegacySurfaceTypeRequestsBus = AZ::EBus<LegacySurfaceTypeRequests>;

} // namespace Physics
