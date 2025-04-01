/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

namespace Physics
{
    void MaterialManager::Init()
    {
        m_defaultMaterial = CreateDefaultMaterialInternal();
        AZ_Assert(m_defaultMaterial.get() != nullptr, "Failed to create default material");
        AZ_Assert(m_defaultMaterial->GetId().IsValid(), "Default material created with invalid id");

        // Add default material to the list
        m_materials.emplace(m_defaultMaterial->GetId(), m_defaultMaterial);
    }

    MaterialManager::~MaterialManager()
    {
        for (const auto& material : m_materials)
        {
            if (material.first != m_defaultMaterial->GetId())
            {
                AZ_Warning(
                    "PhysX Material Manager", !(material.second.use_count() > 1),
                    "Leak: Material '%s' (from asset '%s') is still being referenced by %ld objects.",
                    material.second->GetId().ToString<AZStd::string>().c_str(),
                    material.second->GetMaterialAsset().GetHint().c_str(),
                    material.second.use_count() - 1);
            }
        }
        m_materials.clear();

        AZ_Warning(
            "PhysX Material Manager", !(m_defaultMaterial.use_count() > 1),
            "Leak: Default material '%s' is still being referenced by %ld objects.",
            m_defaultMaterial->GetId().ToString<AZStd::string>().c_str(),
            m_defaultMaterial.use_count() - 1);
        m_defaultMaterial.reset();
    }

    AZStd::shared_ptr<Material> MaterialManager::FindOrCreateMaterial(const MaterialId& id, const AZ::Data::Asset<MaterialAsset>& materialAsset)
    {
        if (!id.IsValid() || !materialAsset.GetId().IsValid())
        {
            return nullptr;
        }

        if (auto materialFound = GetMaterial(id);
            materialFound)
        {
            AZ_Error( "MaterialManager", materialAsset.GetId() == materialFound->GetMaterialAsset().GetId(),
                "MaterialManager::FindOrCreateMaterial found the requested material, but a different asset was used to create it. "
                "Materials of a specific id should be acquired using the same asset. Either make sure the material id "
                "is actually unique, or that you are using the same asset each time for that particular id.");

            return materialFound;
        }

        auto newMaterial = CreateMaterialInternal(id, materialAsset);
        if (newMaterial)
        {
            m_materials.emplace(newMaterial->GetId(), newMaterial);
        }

        // Block until the material asset is loaded.
        // It's important that the loading of physics material assets is not delayed,
        // otherwise objects would react differently during simulation for a few
        // frames until the material is loaded, causing unexpected behaviors.
        AZ::Data::Asset<AZ::Data::AssetData> assetLocal = materialAsset; // Take a reference so we can mutate it for loading.
        if (!assetLocal.IsReady())
        {
            assetLocal.QueueLoad();

            // The call to BlockUntilLoadComplete must happen after the new material
            // has been added to m_materials container. This is because BlockUntilLoadComplete
            // will dispatch events for other assets while waiting, and if other OnAssetReady
            // code ends up trying find/create the same material it needs to find it in the
            // container and return it.
            assetLocal.BlockUntilLoadComplete();
        }

        return newMaterial;
    }

    void MaterialManager::DeleteMaterial(const MaterialId& id)
    {
        if (!id.IsValid() ||
            id == m_defaultMaterial->GetId()) // Do not remove the default material from the list
        {
            return;
        }
        m_materials.erase(id);
    }

    void MaterialManager::DeleteAllMaterials()
    {
        m_materials.clear();
        m_materials.emplace(m_defaultMaterial->GetId(), m_defaultMaterial); // Put back the default material.
    }

    //! Get default material.
    AZStd::shared_ptr<Material> MaterialManager::GetDefaultMaterial()
    {
        return m_defaultMaterial;
    }

    //! Returns a weak pointer to physics material with the given id.
    AZStd::shared_ptr<Material> MaterialManager::GetMaterial(const MaterialId& id)
    {
        if (!id.IsValid())
        {
            return nullptr;
        }
        auto it = m_materials.find(id);
        return it != m_materials.end()
            ? it->second
            : nullptr;
    }
} // namespace Physics
