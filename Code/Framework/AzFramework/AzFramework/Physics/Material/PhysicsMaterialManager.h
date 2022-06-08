/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Asset/AssetCommon.h>

#include <AzFramework/Physics/Material/PhysicsMaterial.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>

namespace Physics
{
    //! This class manages all the creation and deletion of physics materials.
    //! To use it call AZ::Interface<Physics::MaterialManager>::Get().
    class MaterialManager
    {
    public:
        AZ_RTTI(Physics::MaterialManager, "{39EF1222-BE2E-461D-B517-0395CF82C156}");

        MaterialManager() = default;
        virtual ~MaterialManager();

        void Init();

        //! Finds or creates a physics material instance with the given id.
        //! @param id Material id used to identify the material.
        //! @param materialAsset Material asset to create the material from.
        //! @return Material instance created or found. It can return nullptr if the creation failed.
        AZStd::shared_ptr<Material> FindOrCreateMaterial(const MaterialId& id, const AZ::Data::Asset<MaterialAsset>& materialAsset);

        //! Deletes a physics material instance with the given id.
        //! Default material will not be destroyed.
        void DeleteMaterial(const MaterialId& id);

        //! Deletes all physics material instances.
        //! Default material will not be destroyed.
        void DeleteAllMaterials();

        //! Get default material.
        AZStd::shared_ptr<Material> GetDefaultMaterial();

        //! Returns a physics material instance with the given id.
        AZStd::shared_ptr<Material> GetMaterial(const MaterialId& id);

    protected:
        virtual AZStd::shared_ptr<Material> CreateDefaultMaterialInternal() = 0;
        virtual AZStd::shared_ptr<Material> CreateMaterialInternal(const MaterialId& id, const AZ::Data::Asset<MaterialAsset>& materialAsset) = 0;

        AZStd::shared_ptr<Material> m_defaultMaterial;
        AZStd::unordered_map<MaterialId, AZStd::shared_ptr<Material>> m_materials;
    };
} // namespace Physics
