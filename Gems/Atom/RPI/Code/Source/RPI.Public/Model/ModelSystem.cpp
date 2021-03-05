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

#include <Atom/RPI.Public/Model/ModelSystem.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Model/ModelLod.h>

#include <AtomCore/Instance/InstanceDatabase.h>

namespace AZ
{
    namespace RPI
    {
        void ModelSystem::Reflect(AZ::ReflectContext* context)
        {
            ModelLodAsset::Reflect(context);
            ModelAsset::Reflect(context);
        }

        void ModelSystem::GetAssetHandlers(AssetHandlerPtrList& assetHandlers)
        {
            assetHandlers.emplace_back(MakeAssetHandler<ModelLodAssetHandler>());
            assetHandlers.emplace_back(MakeAssetHandler<ModelAssetHandler>());
        }

        void ModelSystem::Init()
        {
            //Create Lod Database
            AZ::Data::InstanceHandler<ModelLod> lodInstanceHandler;
            lodInstanceHandler.m_createFunction = [](Data::AssetData* modelLodAsset)
            {
                return ModelLod::CreateInternal(*(azrtti_cast<ModelLodAsset*>(modelLodAsset)));
            };
            Data::InstanceDatabase<ModelLod>::Create(azrtti_typeid<ModelLodAsset>(), lodInstanceHandler);

            //Create Model Database
            AZ::Data::InstanceHandler<Model> modelInstanceHandler;
            modelInstanceHandler.m_createFunction = [](Data::AssetData* modelAsset)
            {
                return Model::CreateInternal(*(azrtti_cast<ModelAsset*>(modelAsset)));
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
