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

#include <PhysX_precompiled.h>

#include "Material.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <PxPhysicsAPI.h>

namespace PhysX
{
    Material::Material(Material&& material) 
        : m_pxMaterial(AZStd::move(material.m_pxMaterial))
        , m_surfaceType(material.m_surfaceType)
        , m_surfaceString(AZStd::move(material.m_surfaceString))
    {
        m_pxMaterial->userData = this;
    }

    Material& Material::operator=(Material&& material)
    {
        m_pxMaterial = AZStd::move(material.m_pxMaterial);
        m_surfaceType = material.m_surfaceType;
        m_surfaceString = AZStd::move(material.m_surfaceString);

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
        m_surfaceType = AZ::Crc32(materialConfiguration.m_surfaceType.c_str());
        m_surfaceString = materialConfiguration.m_surfaceType;

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

        m_surfaceType = AZ::Crc32(configuration.m_surfaceType.c_str());
        m_surfaceString = configuration.m_surfaceType;

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

    void Material::SetSurfaceType(AZ::Crc32 surfaceType)
    {
        m_surfaceType = surfaceType;
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

    AZ::u32 Material::GetCryEngineSurfaceId() const
    {
        return m_cryEngineSurfaceId;
    }

    void* Material::GetNativePointer()
    {
        return m_pxMaterial.get();
    }

    MaterialsManager::MaterialsManager()
    {
    }

    MaterialsManager::~MaterialsManager()
    {
    }

    void MaterialsManager::Connect()
    {
        Physics::PhysicsMaterialRequestBus::Handler::BusConnect();
        MaterialManagerRequestsBus::Handler::BusConnect();
    }

    void MaterialsManager::Disconnect()
    {
        MaterialManagerRequestsBus::Handler::BusDisconnect();
        Physics::PhysicsMaterialRequestBus::Handler::BusDisconnect();
    }

    void MaterialsManager::GetMaterials(const Physics::MaterialSelection& materialSelection
        , AZStd::vector<AZStd::weak_ptr<Physics::Material>>& outMaterials)
    {
        outMaterials.clear();
        outMaterials.reserve(materialSelection.GetMaterialIdsAssignedToSlots().size());

        // Ensure PxMaterial instances are initialized if possible.
        InitializeMaterials(materialSelection);

        for (const auto& id : materialSelection.GetMaterialIdsAssignedToSlots())
        {
            Physics::MaterialFromAssetConfiguration configuration;
            if (materialSelection.GetMaterialConfiguration(configuration, id))
            {
                auto iterator = m_materialsFromAssets.find(id.GetUuid());
                if (iterator != m_materialsFromAssets.end())
                {
                    outMaterials.push_back(iterator->second);
                }
                else
                {
                    outMaterials.push_back(GetDefaultMaterial());
                }
            }
            else
            {
                // It is important to return exactly the amount of materials specified in materialSelection
                // If a number of materials different to what was cooked is assigned on a physx mesh it will lead to undefined 
                // behavior and subtle bugs. Unfortunately, there's no warning or assertion on physx side at the shape creation time, 
                // nor mention of this in the documentation
                outMaterials.push_back(GetDefaultMaterial());
            }
        }
    }

    AZStd::weak_ptr<Physics::Material> MaterialsManager::GetMaterialByName(const AZStd::string& name)
    {
        auto it = AZStd::find_if(m_materialsFromAssets.begin(), m_materialsFromAssets.end(),
            [&name](const AZStd::pair<AZ::Uuid, AZStd::shared_ptr<Material>>& elem)
        {
            return elem.second.get()->GetSurfaceTypeName() == name;
        });

        if (it != m_materialsFromAssets.end())
        {
            return it->second;
        }
        return {};
    }

    AZ::u32 MaterialsManager::GetFirstSelectedMaterialIndex(const Physics::MaterialSelection& materialSelection)
    {
        const AZ::u32 defaultMaterialIndex = 0;

        if (!materialSelection.IsMaterialLibraryValid())
        {
            return defaultMaterialIndex;
        }

        auto materialAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(materialSelection.GetMaterialLibraryAssetId(), AZ::Data::AssetLoadBehavior::Default);

        materialAsset.BlockUntilLoadComplete();

        AZStd::vector<Physics::MaterialFromAssetConfiguration> materialList = materialAsset.Get()->GetMaterialsData();
        
        const AZStd::vector<Physics::MaterialId>& selectedMaterials = materialSelection.GetMaterialIdsAssignedToSlots();
        if (selectedMaterials.size() == 0)
        {
            return defaultMaterialIndex;
        }
        for (AZ::u32 i=0; i < materialList.size(); ++i)
        {
            if (materialList[i].m_id == selectedMaterials[0])
            {
                return i + 1; // Index 0 is reserved for Default material.
            }
        }
        
        return defaultMaterialIndex;
    }

    void MaterialsManager::GetPxMaterials(const Physics::MaterialSelection& materialSelection
        , AZStd::vector<physx::PxMaterial*>& outMaterials)
    {
        outMaterials.clear();
        if (materialSelection.GetMaterialIdsAssignedToSlots().empty())
        {
            // if the materialSelection is invalid we still
            // return a default material as a fallback behavior
            outMaterials.push_back(GetDefaultMaterial()->GetPxMaterial());
            return;
        }
        outMaterials.reserve(materialSelection.GetMaterialIdsAssignedToSlots().size());

        // Ensure PxMaterial instances are initialized if possible.
        InitializeMaterials(materialSelection);

        for (const auto& id : materialSelection.GetMaterialIdsAssignedToSlots())
        {
            Physics::MaterialFromAssetConfiguration configuration;
            if (materialSelection.GetMaterialConfiguration(configuration, id))
            {
                auto iterator = m_materialsFromAssets.find(id.GetUuid());
                if (iterator != m_materialsFromAssets.end())
                {
                    outMaterials.push_back(iterator->second->GetPxMaterial());
                }
                else
                {
                    outMaterials.push_back(GetDefaultMaterial()->GetPxMaterial());
                }
            }
            else
            {
                // It is important to return exactly the amount of materials specified in materialSelection
                // If a number of materials different to what was cooked is assigned on a physx mesh it will lead to undefined 
                // behavior and subtle bugs. Unfortunately, there's no warning or assertion on physx side at the shape creation time, 
                // nor mention of this in the documentation
                outMaterials.push_back(GetDefaultMaterial()->GetPxMaterial());
            }
        }
    }

    AZStd::shared_ptr<Physics::Material> MaterialsManager::GetGenericDefaultMaterial()
    {
        return GetDefaultMaterial();
    }

    const AZStd::shared_ptr<Material>& MaterialsManager::GetDefaultMaterial()
    {
        if (!m_defaultMaterial)
        {
            m_defaultMaterial = AZStd::make_shared<Material>(Physics::MaterialConfiguration());
        }

        return m_defaultMaterial;
    }

    void MaterialsManager::ReleaseAllMaterials()
    {
        m_defaultMaterial = nullptr;
        m_materialsFromAssets.clear();
        Physics::PhysicsMaterialNotificationsBus::Broadcast(&Physics::PhysicsMaterialNotificationsBus::Events::MaterialsReleased);
    }

    void MaterialsManager::InitializeMaterials(const Physics::MaterialSelection& materialSelection)
    {
        const AZStd::vector<Physics::MaterialId>& materialIds = materialSelection.GetMaterialIdsAssignedToSlots();
        for (const auto& id : materialIds)
        {
            Physics::MaterialFromAssetConfiguration configuration;
            if (!materialSelection.GetMaterialConfiguration(configuration, id))
            {
                continue; // Default material skips code below.
            }
            
            auto materialId = configuration.m_id;

            if (materialId.IsNull())
            {
                materialId = Physics::MaterialId::Create();
            }

            auto iterator = m_materialsFromAssets.find(materialId.GetUuid());
            if (iterator != m_materialsFromAssets.end())
            {
                iterator->second->UpdateWithConfiguration(configuration.m_configuration);
            }
            else
            {
                auto newMaterial = AZStd::make_shared<Material>(configuration.m_configuration);
                m_materialsFromAssets.emplace(materialId.GetUuid(), newMaterial);
            }
        }
    }
}
