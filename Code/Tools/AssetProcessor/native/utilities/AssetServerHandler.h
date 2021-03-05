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
        bool IsServerAddressValid();
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
