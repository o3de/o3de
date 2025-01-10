/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! Any asset can be used for storage any az serialization class data. So user don't need to create their own 
        //! builder and handler.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API AnyAsset
            : public AZ::Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class AnyAssetHandler;
            friend class AnyAssetBuilder;
            friend class AnyAssetCreator;

        public:
            AZ_RTTI(AnyAsset, "{2643D686-3E7C-450C-BB61-427EDEBF13D5}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(AnyAsset, SystemAllocator);

            static constexpr const char* DisplayName{ "AnyAsset" };
            static constexpr const char* Group{ "Common" };
            static constexpr const char* Extension{ "azasset" };
            
            template <typename T>
            const T* GetDataAs() const
            {
                return AZStd::any_cast<T>(&m_data);
            }

            const AZStd::any& GetAny() const
            {
                return m_data;
            }

        private:
            // Called by asset creators to assign the asset to a ready state.
            void SetReady();

            AZStd::any m_data;
        };

        template<typename T>
        const T* GetDataFromAnyAsset(Data::Asset<AnyAsset> anyAsset)
        {
            // If the asset wasn't loaded, load it first
            if (!anyAsset.IsReady())
            {
                anyAsset = AZ::Data::AssetManager::Instance().GetAsset<AnyAsset>(anyAsset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                anyAsset.BlockUntilLoadComplete();
            }

            if (!anyAsset.IsReady())
            {
                AZ_Error("AnyAsset", false, "Failed to load asset [%s]", anyAsset.GetHint().c_str());
                return nullptr;
            }

            const T* dataT = anyAsset->GetDataAs<T>();
            AZ_Error("AnyAsset", dataT, "Asset [%s] doesn't have expected data", anyAsset.GetHint().c_str());
            return dataT;
        }

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API AnyAssetHandler
            : public AssetHandler<AnyAsset>
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            using Base = AssetHandler<AnyAsset>;
        public:
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;
            bool SaveAssetData(const Data::Asset<Data::AssetData>& asset, IO::GenericStream* stream) override;
        };

        class ATOM_RPI_REFLECT_API AnyAssetCreator
        {
        public:
            static bool CreateAnyAsset(const AZStd::any& anyData, const Data::AssetId& assetId, Data::Asset<AnyAsset>& result);
            static void SetAnyAssetData(const AZStd::any& anyData, AnyAsset& result);
        };

    } // namespace RPI
} // namespace AZ
