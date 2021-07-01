/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
