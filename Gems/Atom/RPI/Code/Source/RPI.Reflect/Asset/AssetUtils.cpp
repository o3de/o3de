/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AzCore/Asset/AssetManagerBus.h>

namespace AZ
{
    namespace RPI
    {
        namespace AssetUtils
        {
            void AssetUtilsInternal::ReportIssue(TraceLevel traceLevel, [[maybe_unused]] const char* message)
            {
                switch (traceLevel)
                {
                case TraceLevel::None:
                    break;
                case TraceLevel::Warning:
                    AZ_Warning("AssetUtils", false, "%s", message);
                    break;
                case TraceLevel::Error:
                    AZ_Error("AssetUtils", false, "%s", message);
                    break;
                case TraceLevel::Assert:
                    AZ_Assert(false, "%s", message);
                    break;
                }
            }

            Data::AssetId GetAssetIdForProductPath(const char* productPath, TraceLevel reporting, Data::AssetType assetType, bool autoGenerateId)
            {
                Data::AssetId assetId;
                Data::AssetCatalogRequestBus::BroadcastResult(
                    assetId,
                    &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                    productPath,
                    assetType,
                    autoGenerateId);

                if (!assetId.IsValid())
                {
                    AZStd::string errorMessage = AZStd::string::format("Unable to find product asset '%s'. Has the source asset finished building?", productPath);
                    AssetUtilsInternal::ReportIssue(reporting, errorMessage.c_str());
                }

                return assetId;
            }

            // AsyncAssetLoader //

            AsyncAssetLoader::AsyncAssetLoader(AssetCallback callback)
                : m_callback(callback)
            {
            }

            AsyncAssetLoader::~AsyncAssetLoader()
            {
                Data::AssetBus::MultiHandler::BusDisconnect();
            }

            void AsyncAssetLoader::OnAssetReady(Data::Asset<Data::AssetData> asset)
            {
                HandleCallback(asset, true);
            }

            void AsyncAssetLoader::OnAssetError(Data::Asset<Data::AssetData> asset)
            {
                HandleCallback(asset, false);
            }

            void AsyncAssetLoader::HandleCallback(Data::Asset<Data::AssetData> asset, bool isSuccess)
            {
                Data::AssetBus::MultiHandler::BusDisconnect();
                m_callback(asset, isSuccess);

                m_callback = {}; // Release the callback to avoid holding references to anything captured in the lambda.
                m_asset = {}; // Release the asset in case this AsyncAssetLoader hangs around longer than the asset needs to.
            }

        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ
