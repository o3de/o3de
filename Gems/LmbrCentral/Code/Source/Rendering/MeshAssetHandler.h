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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

struct ICVar;

namespace LmbrCentral
{
    static const char* s_meshAssetHandler_AsyncCvar = "az_Asset_EnableAsyncMeshLoading";

    /**
     * Base class for mesh asset handlers. Contains shared utilities and functionality.
     */
    class MeshAssetHandlerHelper
    {
    public:

        MeshAssetHandlerHelper();

    protected:

        /**
         * Removes the asset alias from a string
         *
         * StatObjs, CharacterInstances and GeometryCaches are stored in 
         * dictionaries by their path which is not aliased like the new AZ systems.
         * To look up mesh instances we need the un-aliased path. 
         *
         * This method assumes that the alias will be at the beginning of the string.
         *
         * @param assetPath The asset path string to remove the alias from
         */
        void StripAssetAlias(const char*& assetPath);

        static const AZStd::string s_assetAliasToken; //< The token used to strip the asset alias in StripAssetAlias

        ICVar* GetAsyncLoadCVar();
        ICVar* m_asyncLoadCvar;
    };

    /**
     * Handler for static mesh assets (cgf).
     */
    class MeshAssetHandler
        : public AZ::Data::AssetHandler
        , public AZ::AssetTypeInfoBus::Handler
        , private MeshAssetHandlerHelper
    {
    public:

        AZ_CLASS_ALLOCATOR(MeshAssetHandler, AZ::SystemAllocator, 0);

        ~MeshAssetHandler() override;

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        void GetCustomAssetStreamInfoForLoad(AZ::Data::AssetStreamInfo& streamInfo) override;
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        AZ::Data::AssetId AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        AZ::Uuid GetComponentTypeId() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
        //////////////////////////////////////////////////////////////////////////////////////////////

        void Register();
        void Unregister();

        AZ::Data::AssetId m_missingMeshAssetId;
    };
} // namespace LmbrCentral
