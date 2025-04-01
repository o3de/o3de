/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Asset/WhiteBoxMeshAsset.h"
#include "Asset/WhiteBoxMeshAssetHandler.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>

namespace WhiteBox
{
    namespace Pipeline
    {
        WhiteBoxMeshAssetHandler::WhiteBoxMeshAssetHandler()
        {
            Register();
        }

        WhiteBoxMeshAssetHandler::~WhiteBoxMeshAssetHandler()
        {
            Unregister();
        }

        void WhiteBoxMeshAssetHandler::Register()
        {
            const bool assetManagerReady = AZ::Data::AssetManager::IsReady();
            AZ_Error("WhiteBoxMesh Asset", assetManagerReady, "Asset manager isn't ready.");

            if (assetManagerReady)
            {
                AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid());
            }

            AZ::AssetTypeInfoBus::Handler::BusConnect(AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid());
        }

        void WhiteBoxMeshAssetHandler::Unregister()
        {
            AZ::AssetTypeInfoBus::Handler::BusDisconnect();

            if (AZ::Data::AssetManager::IsReady())
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(this);
            }
        }

        AZ::Data::AssetType WhiteBoxMeshAssetHandler::GetAssetType() const
        {
            return AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid();
        }

        void WhiteBoxMeshAssetHandler::GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions)
        {
            extensions.emplace_back(AssetFileExtension);
        }

        const char* WhiteBoxMeshAssetHandler::GetAssetTypeDisplayName() const
        {
            return "WhiteBoxMesh";
        }

        const char* WhiteBoxMeshAssetHandler::GetBrowserIcon() const
        {
            return "Editor/Icons/Components/WhiteBox.svg";
        }

        const char* WhiteBoxMeshAssetHandler::GetGroup() const
        {
            return "WhiteBox";
        }

        AZ::Uuid WhiteBoxMeshAssetHandler::GetComponentTypeId() const
        {
            return AZ::Uuid("{C9F2D913-E275-49BB-AB4F-2D221C16170A}"); // EditorWhiteBoxComponent
        }

        AZ::Data::AssetPtr WhiteBoxMeshAssetHandler::CreateAsset(
            [[maybe_unused]] const AZ::Data::AssetId& id, const AZ::Data::AssetType& type)
        {
            if (type == AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid())
            {
                return aznew WhiteBoxMeshAsset();
            }

            AZ_Error("WhiteBoxMesh", false, "This handler deals only with WhiteBoxMeshAsset type.");
            return nullptr;
        }

        AZ::Data::AssetHandler::LoadResult WhiteBoxMeshAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            WhiteBoxMeshAsset* whiteBoxMeshAsset = asset.GetAs<WhiteBoxMeshAsset>();
            if (!whiteBoxMeshAsset)
            {
                AZ_Error(
                    "WhiteBoxMesh Asset", false,
                    "This should be a WhiteBoxMesh Asset, as this is the only type we process.");
                return AZ::Data::AssetHandler::LoadResult::Error;
            }

            const auto size = stream->GetLength();

            Api::WhiteBoxMeshStream whiteBoxData;
            whiteBoxData.resize(size);

            stream->Read(size, whiteBoxData.data());

            auto whiteBoxMesh = WhiteBox::Api::CreateWhiteBoxMesh();
            const auto result = WhiteBox::Api::ReadMesh(*whiteBoxMesh, whiteBoxData);

            // if result is not 'Full', then whiteBoxMeshAsset could be empty which is most likely an error
            // as no data was loaded from the asset, or it was not correctly read in stream->Read(..)
            const auto success = result == Api::ReadResult::Full;
            if (success)
            {
                whiteBoxMeshAsset->SetMesh(AZStd::move(whiteBoxMesh));
                whiteBoxMeshAsset->SetWhiteBoxData(AZStd::move(whiteBoxData));
            }

            return success ? AZ::Data::AssetHandler::LoadResult::LoadComplete
                           : AZ::Data::AssetHandler::LoadResult::Error;
        }

        bool WhiteBoxMeshAssetHandler::SaveAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream)
        {
            WhiteBoxMeshAsset* whiteBoxMeshAsset = asset.GetAs<WhiteBoxMeshAsset>();
            if (!whiteBoxMeshAsset)
            {
                AZ_Error(
                    "WhiteBoxMesh Asset", false,
                    "This should be a WhiteBoxMesh Asset. WhiteBoxMeshAssetHandler doesn't handle any other asset "
                    "type.");
                return false;
            }

            WhiteBox::WhiteBoxMesh* mesh = whiteBoxMeshAsset->GetMesh();
            if (!mesh)
            {
                AZ_Warning("WhiteBoxMesh Asset", false, "There is no WhiteBoxMesh to save.");
                return false;
            }

            const bool success = WhiteBox::Api::SaveToWbm(*mesh, *stream);

            if (!success)
            {
                AZ_Warning("", false, "Failed to write WhiteBoxMesh Asset:", asset.GetHint().c_str());
            }

            return success;
        }

        void WhiteBoxMeshAssetHandler::DestroyAsset(AZ::Data::AssetPtr ptr)
        {
            delete ptr;
        }

        void WhiteBoxMeshAssetHandler::GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes)
        {
            assetTypes.push_back(AZ::AzTypeInfo<WhiteBoxMeshAsset>::Uuid());
        }
    } // namespace Pipeline
} // namespace WhiteBox
