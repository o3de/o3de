/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <OpenParticleSystem/Asset/ParticleAsset.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Asset/GenericAssetHandler.h>

namespace OpenParticle
{
    template<typename AssetDataT>
    class AssetHandler
        : public AzFramework::GenericAssetHandler<AssetDataT>
    {
        using Base = AzFramework::GenericAssetHandler<AssetDataT>;

    public:
        AssetHandler()
            : Base(AssetDataT::DisplayName, AssetDataT::Group, AssetDataT::Extension)
        {
        }

        virtual ~AssetHandler()
        {
            Base::Unregister();
        }

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            return Base::LoadAssetData(asset, stream, assetLoadFilterCB);
        }
    };

    class ParticleAssetHandler
        : public AssetHandler<ParticleAsset>
    {
        using Base = AssetHandler<ParticleAsset>;

    public:
        AZ_RTTI(ParticleAssetHandler, "{C6ED598C-D838-4134-89E0-96804EF7A326}", Base);

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
    };

    template<class T, class... Args>
    AZStd::unique_ptr<T> MakeAssetHandler(Args&&... args)
    {
        auto assetHandler = AZStd::make_unique<T>(AZStd::forward<Args&&>(args)...);
        assetHandler->Register();
        return AZStd::move(assetHandler);
    }

    using AssetHandlerPtrList = AZStd::vector<AZStd::unique_ptr<AZ::Data::AssetHandler>>;

    template<class T, class... Args>
    AZStd::shared_ptr<T> MakeSharedAssetHandler(Args&&... args)
    {
        auto assetHandler = AZStd::make_shared<T>(AZStd::forward<Args&&>(args)...);
        assetHandler->Register();
        return AZStd::move(assetHandler);
    }

    using AssetHandlerSharedPtrList = AZStd::vector<AZStd::shared_ptr<AZ::Data::AssetHandler>>;
} // namespace OpenParticle
