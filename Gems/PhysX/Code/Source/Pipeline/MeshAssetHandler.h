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


#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzFramework/Physics/Material.h>

namespace PhysX
{
    namespace Pipeline
    {


        /// Asset handler for loading and initializing PhysXMeshAsset assets.
        class MeshAssetHandler
            : public AZ::Data::AssetHandler
            , private AZ::AssetTypeInfoBus::Handler
        {
        public:
            static const char* s_assetFileExtension;

            AZ_CLASS_ALLOCATOR(MeshAssetHandler, AZ::SystemAllocator, 0);

            MeshAssetHandler();
            ~MeshAssetHandler();

            void Register();
            void Unregister();

            // AZ::Data::AssetHandler
            AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            void DestroyAsset(AZ::Data::AssetPtr ptr) override;
            void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

            // AZ::AssetTypeInfoBus
            AZ::Data::AssetType GetAssetType() const override;
            void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
            const char* GetAssetTypeDisplayName() const;
            const char* GetBrowserIcon() const override;
            const char* GetGroup() const override;
            AZ::Uuid GetComponentTypeId() const override;
            bool CanCreateComponent(const AZ::Data::AssetId& assetId) const override;
        };
    } // namespace Pipeline
} // namespace PhysX
