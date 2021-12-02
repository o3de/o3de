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
    //! AssetServerHandler is implementing asset server using network share.
    class AssetServerHandler
        : public AssetServerBus::Handler
    {
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
    protected:
        //! Source files intended to be copied into the cache don't go through out temp folder so they need
        //! to be added to the Archive in an additional step
        bool AddSourceFilesToArchive(const AssetProcessor::BuilderParams& builderParams, const QString& archivePath, AZStd::vector<AZStd::string>& sourceFileList);
        
        //////////////////////////////////////////////////////////////////////////
    };
} //namespace AssetProcessor
