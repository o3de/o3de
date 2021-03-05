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

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Asset/AssetBundler.h>

namespace AzToolsFramework
{
    //! AssetBundleCommands
    //! This bus handles messages related to modifying asset bundles on disk.
    //! These commands are currently implemented as blocking operations.
    class AssetBundleCommands
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<AssetBundleCommands>;

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        static const bool LocklessDispatch = true;
        virtual ~AssetBundleCommands() {}

        //! Create a Delta Asset Catalog for the bundle at the given path based on the currently loaded asset catalog,
        //! and inject that catalog into the bundle. 
        //! By default, it will not regenerate or re-inject a Delta Asset Catalog into a bundle that already has one, unless
        //! regenerate is true.
        //! Returns true once the bundle contains a Delta Asset Catalog.
        virtual bool CreateDeltaCatalog(const AZStd::string& sourcePak, bool regenerate) = 0;
        
        //! Creates an Asset Bundle from the given a bundleSettings object.
        //! AssetBundleSettings struct will contain all the information related to the bundles, including asset file info list path and output path.
        //! Please note that if the size of the bundle exceeds the max size specified than the bundles will get split into multiple bundles
        //! and the base bundle manifest will contain information of the rollover bundles.
        virtual bool CreateAssetBundle(const AssetBundleSettings& assetBundleSettings) = 0;

        //! Creates an Asset Bundle from the given asset file info list.
        //! AssetBundleSettings struct will contain all the information related to the bundles, but asset file info list path will be ignored in favor of the AssetFileInfoList provided.
        //! Please note that if the size of the bundle exceeds the max size specified than the bundles will get split into multiple bundles
        //! and the base bundle manifest will contain information of the rollover bundles.
        virtual bool CreateAssetBundleFromList(const AssetBundleSettings& assetBundleSettings, const AssetFileInfoList& assetFileInfoList) = 0;
    };
    using AssetBundleCommandsBus = AZ::EBus<AssetBundleCommands>;
}
