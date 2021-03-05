/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
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

#include <AzCore/Console/IConsole.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AzFramework::AssetBenchmark
{
    //! Given a list of assets, load them and time the results.
    //! @param assetList The list of assets to load (list ownership is handed off to this function)
    //! @param loadBlocking true=load synchronously and sequentially, false=load asynchronously
    void BenchmarkLoadAssetList(AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetType>>&& assetList,
                                        bool loadBlocking = false);

    //! Clear the list of assets to use with BenchmarkLoadAssetList.
    //! @param parameters The set of console command parameters that were passed in
    void BenchmarkClearAssetList(const AZ::ConsoleCommandContainer& parameters);

    //! Add one or more asset names to the list of assets to use with BenchmarkLoadAssetList.
    //! @param parameters The set of console command parameters that were passed in
    void BenchmarkAddAssetsToList(const AZ::ConsoleCommandContainer& parameters);

    //! Given a list of assets, load them and time the results.
    //! @param parameters The set of console command parameters that were passed in
    void BenchmarkLoadAssetList(const AZ::ConsoleCommandContainer& parameters);

    //! Load all assets that exist in the asset catalog and time the results.
    //! @param parameters The set of console command parameters that were passed in
    void BenchmarkLoadAllAssets(const AZ::ConsoleCommandContainer& parameters);

    //! Synchronously load all assets that exist in the asset catalog and time the results.
    //! @param parameters The set of console command parameters that were passed in
    void BenchmarkLoadAllAssetsSynchronous(const AZ::ConsoleCommandContainer& parameters);

}// namespace AzFramework::AssetBenchmark
