/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>

#include "Material.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <PxPhysicsAPI.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <PhysX/MeshAsset.h>

namespace PhysX
{
    Material::Material(Material&& material) 
        : m_pxMaterial(AZStd::move(material.m_pxMaterial))
        , m_surfaceType(material.m_surfaceType)
        , m_surfaceString(AZStd::move(material.m_surfaceString))
        , m_cryEngineSurfaceId(material.m_cryEngineSurfaceId)
        , m_density(material.m_density)
        , m_debugColor(AZStd::move(material.m_debugColor))
    {
        m_pxMaterial->userData = this;
    }

    Material& Material::operator=(Material&& material)
    {
        m_pxMaterial = AZStd::move(material.m_pxMaterial);
        m_surfaceType = material.m_surfaceType;
        m_surfaceString = AZStd::move(material.m_surfaceString);
        m_cryEngineSurfaceId = material.m_cryEngineSurfaceId;
        m_density = material.m_density;
        m_debugColor = AZStd::move(material.m_debugColor);

        m_pxMaterial->userData = this;

        return *this;
    }

    static Material::CombineMode FromPxCombineMode(physx::PxCombineMode::Enum pxMode)
    {
        switch (pxMode)
        {
        case physx::PxCombineMode::eAVERAGE: return Material::CombineMode::Average;
        case physx::PxCombineMode::eMULTIPLY: return Material::CombineMode::Multiply;
        case physx::PxCombineMode::eMAX: return Material::CombineMode::Maximum;
        case physx::PxCombineMode::eMIN: return Material::CombineMode::Minimum;
        default: return Material::CombineMode::Average;
        }
    }

    static physx::PxCombineMode::Enum ToPxCombineMode(Material::CombineMode mode)
    {
        switch (mode)
        {
        case Material::CombineMode::Average: return physx::PxCombineMode::eAVERAGE;
        case Material::CombineMode::Multiply: return physx::PxCombineMode::eMULTIPLY;
        case Material::CombineMode::Maximum: return physx::PxCombineMode::eMAX;
        case Material::CombineMode::Minimum: return physx::PxCombineMode::eMIN;
        default: return physx::PxCombineMode::eAVERAGE;
        }
    }

    Material::Material(const Physics::MaterialConfiguration& materialConfiguration)
    {
        float staticFriction = materialConfiguration.m_staticFriction;
        AZ_Warning("PhysX Material", staticFriction >= 0.0f, "Static friction %f for material %s is out of range [0, PX_MAX_F32)",
            staticFriction, materialConfiguration.m_surfaceType.c_str());

        float dynamicFriction = materialConfiguration.m_dynamicFriction;
        AZ_Warning("PhysX Material", dynamicFriction >= 0.0f, "Dynamic friction %f for material %s is out of range [0, PX_MAX_F32)",
            dynamicFriction, materialConfiguration.m_surfaceType.c_str());

        float restitution = materialConfiguration.m_restitution;
        AZ_Warning("PhysX Material", restitution >= 0 && restitution <= 1.0f, "Restitution %f for material %s is out of range [0, 1]",
            restitution, materialConfiguration.m_surfaceType.c_str());

        // Clamp the values to valid ranges
        staticFriction = AZ::GetMax(0.0f, staticFriction);
        dynamicFriction = AZ::GetMax(0.0f, dynamicFriction);
        restitution = AZ::GetClamp(restitution, 0.0f, 1.0f);

        SetDensity(materialConfiguration.m_density);

        auto pxMaterial = PxGetPhysics().createMaterial(staticFriction, dynamicFriction, restitution);

        auto materialDestructor = [](auto material)
        {
            material->release();
            material->userData = nullptr;
        };

        pxMaterial->setFrictionCombineMode(ToPxCombineMode(materialConfiguration.m_frictionCombine));
        pxMaterial->setRestitutionCombineMode(ToPxCombineMode(materialConfiguration.m_restitutionCombine));
        pxMaterial->userData = this;

        m_pxMaterial = PxMaterialUniquePtr(pxMaterial, materialDestructor);

        SetSurfaceTypeName(materialConfiguration.m_surfaceType);

        SetDebugColor(materialConfiguration.m_debugColor);

        Physics::LegacySurfaceTypeRequestsBus::BroadcastResult(
            m_cryEngineSurfaceId, 
            &Physics::LegacySurfaceTypeRequestsBus::Events::GetLegacySurfaceTypeFronName,
            m_surfaceString);
    }

    void Material::UpdateWithConfiguration(const Physics::MaterialConfiguration& configuration)
    {
        AZ_Assert(m_pxMaterial != nullptr, "Material can't be null!");

        SetRestitution(configuration.m_restitution);
        SetStaticFriction(configuration.m_staticFriction);
        SetDynamicFriction(configuration.m_dynamicFriction);

        SetFrictionCombineMode(configuration.m_frictionCombine);
        SetRestitutionCombineMode(configuration.m_restitutionCombine);

        SetDensity(configuration.m_density);

        SetSurfaceTypeName(configuration.m_surfaceType);

        SetDebugColor(configuration.m_debugColor);

        Physics::LegacySurfaceTypeRequestsBus::BroadcastResult(
            m_cryEngineSurfaceId,
            &Physics::LegacySurfaceTypeRequestsBus::Events::GetLegacySurfaceTypeFronName,
            m_surfaceString);
    }

    physx::PxMaterial* Material::GetPxMaterial()
    {
        return m_pxMaterial.get();
    }

    AZ::Crc32 Material::GetSurfaceType() const
    {
        return m_surfaceType;
    }

    const AZStd::string& Material::GetSurfaceTypeName() const
    {
        return m_surfaceString;
    }

    void Material::SetSurfaceTypeName(const AZStd::string& surfaceTypeName)
    {
        m_surfaceString = surfaceTypeName;
        m_surfaceType = AZ::Crc32(m_surfaceString.c_str());
    }

    float Material::GetDynamicFriction() const
    {
        return m_pxMaterial ? m_pxMaterial->getDynamicFriction() : 0.0f;
    }

    void Material::SetDynamicFriction(float dynamicFriction)
    {
        AZ_Warning("PhysX Material", dynamicFriction >= 0.0f, 
            "SetDynamicFriction: Dynamic friction %f for material %s is out of range [0, PX_MAX_F32)",
            dynamicFriction, m_surfaceString.c_str());

        if (m_pxMaterial)
        {
            m_pxMaterial->setDynamicFriction(AZ::GetMax(0.0f, dynamicFriction));
        }
    }

    float Material::GetStaticFriction() const
    {
        return m_pxMaterial ? m_pxMaterial->getStaticFriction() : 0.0f;
    }

    void Material::SetStaticFriction(float staticFriction)
    {
        AZ_Warning("PhysX Material", staticFriction >= 0.0f, 
            "SetStaticFriction: Static friction %f for material %s is out of range [0, PX_MAX_F32)",
            staticFriction, m_surfaceString.c_str());
        
        if (m_pxMaterial)
        {
            m_pxMaterial->setStaticFriction(AZ::GetMax(0.0f, staticFriction));
        }
    }

    float Material::GetRestitution() const
    {
        return m_pxMaterial ? m_pxMaterial->getRestitution() : 0.0f;
    }

    void Material::SetRestitution(float restitution)
    {
        AZ_Warning("PhysX Material", restitution >= 0 && restitution <= 1.0f, 
            "SetRestitution: Restitution %f for material %s is out of range [0, 1]",
            restitution, m_surfaceString.c_str());

        if (m_pxMaterial)
        {
            m_pxMaterial->setRestitution(AZ::GetClamp(restitution, 0.0f, 1.0f));
        }
    }

    Material::CombineMode Material::GetFrictionCombineMode() const
    {
        return m_pxMaterial ? FromPxCombineMode(m_pxMaterial->getFrictionCombineMode()) : CombineMode::Average;
    }

    void Material::SetFrictionCombineMode(CombineMode mode)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setFrictionCombineMode(ToPxCombineMode(mode));
        }
    }

    Material::CombineMode Material::GetRestitutionCombineMode() const
    {
        return m_pxMaterial ? FromPxCombineMode(m_pxMaterial->getRestitutionCombineMode()) : CombineMode::Average;
    }

    void Material::SetRestitutionCombineMode(CombineMode mode)
    {
        if (m_pxMaterial)
        {
            m_pxMaterial->setRestitutionCombineMode(ToPxCombineMode(mode));
        }
    }

    float Material::GetDensity() const
    {
        return m_density;
    }

    void Material::SetDensity(const float density)
    {
        using Physics::MaterialConfiguration;

        AZ_Warning("PhysX Material", density >= MaterialConfiguration::MinDensityLimit && density <= MaterialConfiguration::MaxDensityLimit,
            "Density %f for material %s should be in range [%f, %f].", density, m_surfaceString.c_str(),
            MaterialConfiguration::MinDensityLimit, MaterialConfiguration::MaxDensityLimit);
        m_density = AZ::GetClamp(density,
            MaterialConfiguration::MinDensityLimit, MaterialConfiguration::MaxDensityLimit);
    }

    AZ::Color Material::GetDebugColor() const
    {
        return m_debugColor;
    }

    void Material::SetDebugColor(const AZ::Color& debugColor)
    {
        m_debugColor = debugColor;
    }

    AZ::u32 Material::GetCryEngineSurfaceId() const
    {
        return m_cryEngineSurfaceId;
    }

    void* Material::GetNativePointer()
    {
        return m_pxMaterial.get();
    }

    MaterialsManager::MaterialsManager()
        : m_physicsConfigChangedHandler(
            [this](const AzPhysics::SystemConfiguration* config)
            {
                OnPhysicsConfigurationChanged(config);
            })
        , m_materialLibraryChangedHandler(
            [this](const AZ::Data::AssetId& materialLibraryAssetId)
            {
                OnMaterialLibraryChanged(materialLibraryAssetId);
            })
    {
    }

    MaterialsManager::~MaterialsManager()
    {
    }

    void MaterialsManager::Connect()
    {
        Physics::PhysicsMaterialRequestBus::Handler::BusConnect();
        MaterialManagerRequestsBus::Handler::BusConnect();

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RegisterSystemConfigurationChangedEvent(m_physicsConfigChangedHandler);
            physicsSystem->RegisterOnMaterialLibraryChangedEventHandler(m_materialLibraryChangedHandler);
        }
    }

    void MaterialsManager::Disconnect()
    {
        m_materialLibraryChangedHandler.Disconnect();
        m_physicsConfigChangedHandler.Disconnect();
        MaterialManagerRequestsBus::Handler::BusDisconnect();
        Physics::PhysicsMaterialRequestBus::Handler::BusDisconnect();
    }

    void MaterialsManager::GetMaterials(const Physics::MaterialSelection& materialSelection
        , AZStd::vector<AZStd::shared_ptr<Physics::Material>>& outMaterials)
    {
        outMaterials.clear();

        const auto& materialIdsAssignedToSlots = materialSelection.GetMaterialIdsAssignedToSlots();
        if (materialIdsAssignedToSlots.empty())
        {
            // The material selection doesn't have any slots, return empty list.
            return;
        }

        // It is important to return exactly the amount of materials specified in materialSelection
        // If a number of materials different to what was cooked is assigned on a physx mesh it will lead to undefined 
        // behavior and subtle bugs. Unfortunately, there's no warning or assertion on physx side at the shape creation time, 
        // nor mention of this in the documentation
        outMaterials.resize(materialIdsAssignedToSlots.size(), GetDefaultMaterial());

        for (size_t slotIndex = 0; slotIndex < materialIdsAssignedToSlots.size(); ++slotIndex)
        {
            const auto& materialId = materialIdsAssignedToSlots[slotIndex];

            if (auto iterator = FindOrCreateMaterial(materialId);
                iterator != m_materials.end())
            {
                outMaterials[slotIndex] = iterator->second;
            }
        }
    }

    AZStd::shared_ptr<Physics::Material> MaterialsManager::GetMaterialById(Physics::MaterialId id)
    {
        if (auto it = FindOrCreateMaterial(id);
            it != m_materials.end())
        {
            return it->second;
        }
        return nullptr;
    }

    AZStd::shared_ptr<Physics::Material> MaterialsManager::GetMaterialByName(const AZStd::string& name)
    {
        if (auto it = FindOrCreateMaterial(name);
            it != m_materials.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void MaterialsManager::GetPxMaterials(const Physics::MaterialSelection& materialSelection
        , AZStd::vector<physx::PxMaterial*>& outMaterials)
    {
        AZStd::vector<AZStd::shared_ptr<Physics::Material>> materials;
        GetMaterials(materialSelection, materials);

        outMaterials.reserve(materials.size());
        for (const auto& material : materials)
        {
            PhysX::Material* physxMaterial = azrtti_cast<PhysX::Material*>(material.get());
            AZ_Assert(physxMaterial, "Invalid physx material");

            outMaterials.emplace_back(physxMaterial->GetPxMaterial());
        }
    }

    void MaterialsManager::UpdateMaterialSelectionFromPhysicsAsset(
        const Physics::ShapeConfiguration& shapeConfiguration,
        Physics::MaterialSelection& materialSelection)
    {
        if (shapeConfiguration.GetShapeType() != Physics::ShapeType::PhysicsAsset)
        {
            return;
        }

        const Physics::PhysicsAssetShapeConfiguration& assetConfiguration =
            static_cast<const Physics::PhysicsAssetShapeConfiguration&>(shapeConfiguration);

        if (!assetConfiguration.m_asset.GetId().IsValid())
        {
            // Set the default selection if there's no physics asset.
            materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
            return;
        }

        if (!assetConfiguration.m_asset.IsReady())
        {
            // The asset is valid but is still loading, 
            // Do not set the empty slots in this case to avoid the entity being in invalid state
            return;
        }

        Pipeline::MeshAsset* meshAsset = assetConfiguration.m_asset.GetAs<Pipeline::MeshAsset>();
        if (!meshAsset)
        {
            materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
            AZ_Warning("PhysX", false, "UpdateMaterialSelectionFromPhysicsAsset: MeshAsset is invalid");
            return;
        }

        // Set the slots from the mesh asset
        materialSelection.SetMaterialSlots(meshAsset->m_assetData.m_materialNames);

        if (!assetConfiguration.m_useMaterialsFromAsset)
        {
            // Not using the materials from the asset. Nothing else to do.
            return;
        }

        // Update material IDs in the selection for each slot
        const AZStd::vector<AZStd::string>& physicsMaterialNames = meshAsset->m_assetData.m_physicsMaterialNames;
        for (size_t slotIndex = 0; slotIndex < physicsMaterialNames.size(); ++slotIndex)
        {
            const AZStd::string& physicsMaterialNameFromPhysicsAsset = physicsMaterialNames[slotIndex];
            if (physicsMaterialNameFromPhysicsAsset.empty() ||
                physicsMaterialNameFromPhysicsAsset == Physics::DefaultPhysicsMaterialLabel)
            {
                materialSelection.SetMaterialId(Physics::MaterialId(), slotIndex);
                continue;
            }

            if (auto it = FindOrCreateMaterial(physicsMaterialNameFromPhysicsAsset);
                it != m_materials.end())
            {
                materialSelection.SetMaterialId(Physics::MaterialId::FromUUID(it->first), slotIndex);
            }
            else
            {
                AZ_Warning("PhysX", false,
                    "UpdateMaterialSelectionFromPhysicsAsset: Physics material '%s' not found in the material library. Mesh material '%s' will use the default physics material.",
                    physicsMaterialNameFromPhysicsAsset.c_str(),
                    meshAsset->m_assetData.m_materialNames[slotIndex].c_str());
                materialSelection.SetMaterialId(Physics::MaterialId(), slotIndex);
            }
        }
    }

    AZStd::shared_ptr<Physics::Material> MaterialsManager::GetGenericDefaultMaterial()
    {
        return GetDefaultMaterial();
    }

    AZStd::shared_ptr<Material> MaterialsManager::GetDefaultMaterial()
    {
        if (!m_defaultMaterial)
        {
            // Get default material from physics configuration
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                m_defaultMaterialConfiguration = physicsSystem->GetConfiguration()->m_defaultMaterialConfiguration;
            }
            else
            {
                AZ_Warning("MaterialsManager", false, "Unable to get Physics System, default material will not be in sync with PhysX Configuration");
            }

            m_defaultMaterial = AZStd::make_shared<Material>(m_defaultMaterialConfiguration);
        }

        return m_defaultMaterial;
    }

    void MaterialsManager::ReleaseAllMaterials()
    {
        m_defaultMaterial = nullptr;
        m_materials.clear();
        Physics::PhysicsMaterialNotificationsBus::Broadcast(&Physics::PhysicsMaterialNotificationsBus::Events::MaterialsReleased);
    }

    MaterialsManager::Materials::iterator MaterialsManager::FindOrCreateMaterial(Physics::MaterialId materialId)
    {
        if (materialId.IsNull())
        {
            return m_materials.end();
        }

        if (auto it = m_materials.find(materialId.GetUuid());
            it != m_materials.end())
        {
            return it;
        }
        else
        {
            auto* materialLibrary = GetMaterialLibrary();
            if (!materialLibrary)
            {
                return m_materials.end();
            }

            Physics::MaterialFromAssetConfiguration configuration;
            if (!materialLibrary->GetDataForMaterialId(materialId, configuration))
            {
                return m_materials.end();
            }

            auto newMaterial = AZStd::make_shared<Material>(configuration.m_configuration);
            auto insertedPair = m_materials.emplace(materialId.GetUuid(), AZStd::move(newMaterial));
            return insertedPair.first;
        }
    }

    MaterialsManager::Materials::iterator MaterialsManager::FindOrCreateMaterial(const AZStd::string& materialName)
    {
        if (materialName.empty())
        {
            return m_materials.end();
        }

        auto it = AZStd::find_if(m_materials.begin(), m_materials.end(), [&materialName](const auto& data)
            {
                return AZ::StringFunc::Equal(data.second->GetSurfaceTypeName(), materialName, false/*bCaseSensitive*/);
            });
        if (it != m_materials.end())
        {
            return it;
        }
        else
        {
            auto* materialLibrary = GetMaterialLibrary();
            if (!materialLibrary)
            {
                return m_materials.end();
            }

            Physics::MaterialFromAssetConfiguration configuration;
            if (!materialLibrary->GetDataForMaterialName(materialName, configuration))
            {
                return m_materials.end();
            }

            auto newMaterial = AZStd::make_shared<Material>(configuration.m_configuration);
            auto insertedPair = m_materials.emplace(configuration.m_id.GetUuid(), AZStd::move(newMaterial));
            return insertedPair.first;
        }
    }

    Physics::MaterialLibraryAsset* MaterialsManager::GetMaterialLibrary()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (const auto* physicsConfiguration = physicsSystem->GetConfiguration())
            {
                return physicsConfiguration->m_materialLibraryAsset.Get();
            }
        }
        return nullptr;
    }

    void MaterialsManager::OnPhysicsConfigurationChanged(const AzPhysics::SystemConfiguration* config)
    {
        if (m_defaultMaterial &&
            m_defaultMaterialConfiguration != config->m_defaultMaterialConfiguration)
        {
            m_defaultMaterialConfiguration = config->m_defaultMaterialConfiguration;

            m_defaultMaterial->UpdateWithConfiguration(m_defaultMaterialConfiguration);
        }
    }

    void MaterialsManager::OnMaterialLibraryChanged([[maybe_unused]] const AZ::Data::AssetId& materialLibraryAssetId)
    {
        auto* materialLibrary = GetMaterialLibrary();
        if (!materialLibrary)
        {
            AZ_Warning("PhysX", false, "MaterialsManager: invalid material library");
            return;
        }

        AZStd::vector<AZ::Uuid> materialsToRemove;

        for (auto& idMaterialPair : m_materials)
        {
            const Physics::MaterialId materialId = Physics::MaterialId::FromUUID(idMaterialPair.first);

            // Remove null materials
            if (materialId.IsNull())
            {
                materialsToRemove.push_back(materialId.GetUuid());
                continue;
            }

            Physics::MaterialFromAssetConfiguration configuration;
            if (materialLibrary->GetDataForMaterialId(materialId, configuration))
            {
                // Update materials found in the library.
                idMaterialPair.second->UpdateWithConfiguration(configuration.m_configuration);
            }
            else
            {
                // Add for removal the materials not present in the library anymore.
                materialsToRemove.push_back(materialId.GetUuid());
            }
        }

        for (const auto& id : materialsToRemove)
        {
            m_materials.erase(id);
        }
    }
}
