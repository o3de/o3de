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

            bool TryToCompileAsset(const AZStd::string& assetFilePath, TraceLevel reporting)
            {
                AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
                AzFramework::AssetSystemRequestBus::BroadcastResult(
                        status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, assetFilePath);

                if ((status != AzFramework::AssetSystem::AssetStatus_Compiled) && (status != AzFramework::AssetSystem::AssetStatus_Unknown))
                {
                    AssetUtilsInternal::ReportIssue(
                        reporting,
                        AZStd::string::format(
                            "Could not compile asset '%s', status = %u.", assetFilePath.c_str(), static_cast<uint32_t>(status))
                            .c_str());

                    return false;
                }

                return true;
            }

            Data::AssetId GetAssetIdForProductPath(const char* productPath, TraceLevel reporting, Data::AssetType assetType)
            {
                // Don't create a new entry in the asset catalog for this asset if it doesn't exist.
                // Since we only have a product path and not an asset id, any entry we create will have an incorrect id,
                // incorrect size and dependency information, and will point to a file that doesn't exist. Any attempt to use
                // that id will fail.
                constexpr bool AutoGenerateId = false;

                Data::AssetId assetId;
                Data::AssetCatalogRequestBus::BroadcastResult(
                    assetId,
                    &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                    productPath,
                    assetType,
                    AutoGenerateId);

                if (!assetId.IsValid())
                {
                    AZStd::string errorMessage = AZStd::string::format(
                        "Unable to find product asset '%s'. Has the source asset finished building?", productPath);
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
                Data::AssetBus::Handler::BusDisconnect();
                SystemTickBus::Handler::BusDisconnect();
            }

            void AsyncAssetLoader::OnAssetReady(Data::Asset<Data::AssetData> asset)
            {
                Data::AssetBus::Handler::BusDisconnect();
                m_asset = asset;
                SystemTickBus::Handler::BusConnect();

            }

            void AsyncAssetLoader::OnAssetError(Data::Asset<Data::AssetData> asset)
            {
                Data::AssetBus::Handler::BusDisconnect();
                m_asset = asset;
                SystemTickBus::Handler::BusConnect();
            }

            void AsyncAssetLoader::HandleCallback(Data::Asset<Data::AssetData> asset)
            {
                if (m_callback)
                {
                    m_callback(asset);
                }

                m_callback = {}; // Release the callback to avoid holding references to anything captured in the lambda.
                m_asset = {}; // Release the asset in case this AsyncAssetLoader hangs around longer than the asset needs to.
            }

            // SystemTickBus::Handler overrides..
            void AsyncAssetLoader::OnSystemTick()
            {
                SystemTickBus::Handler::BusDisconnect();
                HandleCallback(m_asset);
            }

        } // namespace AssetUtils
    } // namespace RPI
} // namespace AZ
