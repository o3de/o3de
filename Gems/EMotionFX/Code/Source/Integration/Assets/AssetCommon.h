/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <EMotionFX/Source/Allocators.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        /**
         *
         */
        class EMotionFXAsset : public AZ::Data::AssetData
        {
        public:
            AZ_RTTI(EMotionFXAsset, "{043F606A-A483-4910-8110-D8BC4B78922C}", AZ::Data::AssetData)
            AZ_CLASS_ALLOCATOR(EMotionFXAsset, EMotionFXAllocator)

            EMotionFXAsset(AZ::Data::AssetId id = AZ::Data::AssetId())
                : AZ::Data::AssetData(id)
            {}

            void ReleaseEMotionFXData()
            {
                m_emfxNativeData.clear();
                m_emfxNativeData.shrink_to_fit();
            }

            AZStd::vector<AZ::u8> m_emfxNativeData;
        };

        /**
         *
         */
        template<typename DataType>
        class EMotionFXAssetHandler
            : public AZ::Data::AssetHandler
            , protected AZ::AssetTypeInfoBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(EMotionFXAssetHandler<DataType>, EMotionFXAllocator)

            EMotionFXAssetHandler()
            {
                Register();
            }

            ~EMotionFXAssetHandler() override
            {
                Unregister();
            }

            AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
            {
                (void)type;
                return aznew DataType(id);
            }

            AZ::Data::AssetId AssetMissingInCatalog(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override
            {
                // missing assets should at least get escalated to the top of the list.  Sub-handlers could override this and do
                // additional things like substitute some default asset Id.
                AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetByUuid, asset.GetId().m_guid);

                return AZ::Data::AssetId();
            }

            AZ::Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
            {
                (void)assetLoadFilterCB;

                DataType* assetData = asset.GetAs<DataType>();

                if (stream->GetLength() > 0)
                {
                    assetData->m_emfxNativeData.resize(stream->GetLength());
                    stream->Read(stream->GetLength(), assetData->m_emfxNativeData.data());

                    return AZ::Data::AssetHandler::LoadResult::LoadComplete;
                }

                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override
            {
                (void)asset;
                (void)stream;
                AZ_Error("EMotionFX", false, "Asset handler does not support asset saving.");
                return false;
            }

            void DestroyAsset(AZ::Data::AssetPtr ptr) override
            {
                delete ptr;
            }

            void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
            {
                assetTypes.push_back(azrtti_typeid<DataType>());
            }

            void Register()
            {
                AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset database isn't ready!");
                AZ::Data::AssetManager::Instance().RegisterHandler(this, azrtti_typeid<DataType>());

                AZ::AssetTypeInfoBus::Handler::BusConnect(azrtti_typeid<DataType>());
            }

            void Unregister()
            {
                AZ::AssetTypeInfoBus::Handler::BusDisconnect(azrtti_typeid<DataType>());

                if (AZ::Data::AssetManager::IsReady())
                {
                    AZ::Data::AssetManager::Instance().UnregisterHandler(this);
                }
            }

            void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override
            {
                if (!loadStageSucceeded || !OnInitAsset(asset))
                {
                    AssetHandler::InitAsset(asset, false, isReload);
                    return;
                }

                AssetHandler::InitAsset(asset, true, isReload);
            }

            virtual bool OnInitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
            {
                (void)asset;
                return true;
            }

            const char* GetGroup() const override
            {
                return "Animation";
            }
        };
    } // namespace Integration
} // namespace EMotionFX

