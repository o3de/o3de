/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/utilities/AssetUtilEBusHelper.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

namespace AssetProcessor
{
    inline constexpr const char* AssetCacheServerModeKey{ "assetCacheServerMode" };
    inline constexpr const char* CacheServerAddressKey{ "cacheServerAddress" };

    //! AssetServerHandler is implementing asset server using network share.
    class AssetServerHandler
        : public AssetServerBus::Handler
    {
    public:
        static const char* GetAssetServerModeText(AssetServerMode mode);

    public:
        AssetServerHandler();
        virtual ~AssetServerHandler();
        //////////////////////////////////////////////////////////////////////////
        // AssetServerBus::Handler overrides
        bool IsServerAddressValid() override;
        //! StoreJobResult will store all the files in the the temp folder provided by AP to a zip file on the network drive 
        //! whose file name will be based on the server key
        bool StoreJobResult(const AssetProcessor::BuilderParams& builderParams, AZStd::vector<AZStd::string>& sourceFileList)  override;
        //! RetrieveJobResult will retrieve the zip file from the network share associated with the server key and unzip it to the temporary directory provided by AP.
        bool RetrieveJobResult(const AssetProcessor::BuilderParams& builderParams) override;
        //! HandleRemoteConfiguration will attempt to set or get the remote configuration for the cache server
        void HandleRemoteConfiguration();
        //! Retrieve the current mode for shared caching
        AssetServerMode GetRemoteCachingMode() const override;
        //! Store the shared caching mode
        void SetRemoteCachingMode(AssetServerMode mode) override;
        //! Retrieve the remote folder location for the shared cache 
        const AZStd::string& GetServerAddress() const override;
        //! Store the remote folder location for the shared cache 
        bool SetServerAddress(const AZStd::string& address) override;
    protected:
        //! Source files intended to be copied into the cache don't go through out temp folder so they need
        //! to be added to the Archive in an additional step
        bool AddSourceFilesToArchive(const AssetProcessor::BuilderParams& builderParams, const QString& archivePath, AZStd::vector<AZStd::string>& sourceFileList);
        QString ComputeArchiveFilePath(const AssetProcessor::BuilderParams& builderParams);
        
    private:
        AssetServerMode m_assetCachingMode = AssetServerMode::Inactive;
        AZStd::string m_serverAddress;
    };
} //namespace AssetProcessor
