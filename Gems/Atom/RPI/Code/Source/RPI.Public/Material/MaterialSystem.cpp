/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Material/MaterialSystem.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/LuaMaterialFunctor.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        void MaterialSystem::Reflect(AZ::ReflectContext* context)
        {
            MaterialPropertyValue::Reflect(context);
            MaterialTypeAsset::Reflect(context);
            MaterialAsset::Reflect(context);
            MaterialPropertiesLayout::Reflect(context);
            MaterialFunctor::Reflect(context);
            LuaMaterialFunctor::Reflect(context);
            ReflectMaterialDynamicMetadata(context);
        }

        void MaterialSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<MaterialTypeAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<MaterialAssetHandler>());
        }

        void MaterialSystem::Init()
        {
            AZ::Data::InstanceHandler<Material> handler;
            handler.m_createFunction = [](Data::AssetData* materialAsset)
            {
                return Material::CreateInternal(*(azrtti_cast<MaterialAsset*>(materialAsset)));
            };
            Data::InstanceDatabase<Material>::Create(azrtti_typeid<MaterialAsset>(), handler);
        }

        void MaterialSystem::Shutdown()
        {
            Data::InstanceDatabase<Material>::Destroy();
        }

    } // namespace RPI
} // namespace AZ
