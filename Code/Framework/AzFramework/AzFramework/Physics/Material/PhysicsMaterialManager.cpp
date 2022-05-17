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
        m_materials.clear();
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

        // Take a reference so we can mutate it.
        AZ::Data::Asset<AZ::Data::AssetData> assetLocal = materialAsset;
        if (!assetLocal.IsReady())
        {
            assetLocal.QueueLoad();
            assetLocal.BlockUntilLoadComplete();

            // Failed to load the asset
            if (!assetLocal.IsReady())
            {
                return nullptr;
            }
        }

        auto newMaterial = CreateMaterialInternal(id, assetLocal);
        if (newMaterial)
        {
            m_materials.emplace(newMaterial->GetId(), newMaterial);
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
