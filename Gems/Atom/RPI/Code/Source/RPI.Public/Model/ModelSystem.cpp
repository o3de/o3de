/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Model/ModelSystem.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Model/ModelLod.h>

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>
#include <Atom/RPI.Reflect/Model/SkinMetaAsset.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        void ModelSystem::Reflect(AZ::ReflectContext* context)
        {
            ModelLodAsset::Reflect(context);
            ModelAsset::Reflect(context);
            ModelMaterialSlot::Reflect(context);
            MorphTargetMetaAsset::Reflect(context);
            SkinMetaAsset::Reflect(context);
        }

        void ModelSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<ModelLodAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ModelAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<MorphTargetMetaAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<SkinMetaAssetHandler>());
        }

        void ModelSystem::Init()
        {
            //Create Lod Database
            AZ::Data::InstanceHandler<ModelLod> lodInstanceHandler;
            lodInstanceHandler.m_createFunctionWithParam = [](Data::AssetData* modelLodAsset, const AZStd::any* modelAsset)
            {
                return ModelLod::CreateInternal(Data::Asset<ModelLodAsset>{modelLodAsset, AZ::Data::AssetLoadBehavior::PreLoad}, modelAsset);
            };
            Data::InstanceDatabase<ModelLod>::Create(azrtti_typeid<ModelLodAsset>(), lodInstanceHandler);

            //Create Model Database
            AZ::Data::InstanceHandler<Model> modelInstanceHandler;
            modelInstanceHandler.m_createFunction = [](Data::AssetData* modelAsset)
            {
                return Model::CreateInternal(Data::Asset<ModelAsset>{modelAsset, AZ::Data::AssetLoadBehavior::PreLoad});
            };
            Data::InstanceDatabase<Model>::Create(azrtti_typeid<ModelAsset>(), modelInstanceHandler);
        }

        void ModelSystem::Shutdown()
        {
            Data::InstanceDatabase<Model>::Destroy();
            Data::InstanceDatabase<ModelLod>::Destroy();
        }
    } // namespace RPI
} // namespace AZ
