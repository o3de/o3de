/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Asset/AssetManager.h>

#include <PhysX/Material/PhysXMaterial.h>
#include <Source/Material/PhysXMaterialManager.h>

namespace PhysX
{
    AZStd::shared_ptr<Physics::Material> MaterialManager::CreateDefaultMaterialInternal()
    {
        AZ::Data::Asset<Physics::MaterialAsset> defaultMaterialAsset =
            AZ::Data::AssetManager::Instance().CreateAsset<Physics::MaterialAsset>(
                AZ::Data::AssetId(AZ::Uuid::CreateRandom()));

        defaultMaterialAsset->SetData(Physics::MaterialConfiguration{});

        return CreateMaterialInternal(
            Physics::MaterialId::CreateFromAssetId(defaultMaterialAsset.GetId()),
            defaultMaterialAsset);
    }

    AZStd::shared_ptr<Physics::Material> MaterialManager::CreateMaterialInternal(
        const Physics::MaterialId& id,
        const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset)
    {
        return AZStd::shared_ptr<Physics::Material>(
            aznew Material(id, materialAsset));
    }
} // namespace PhysX
