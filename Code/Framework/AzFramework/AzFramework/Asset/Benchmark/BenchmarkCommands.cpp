/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Asset/Benchmark/BenchmarkCommands.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/chrono.h>

namespace AzFramework::AssetBenchmark
{
    //! Max time that the load asset benchmark is allowed to run.
    AZ_CVAR(uint32_t, benchmarkLoadAssetTimeoutMs, 20000, nullptr, AZ::ConsoleFunctorFlags::Null,
            "Timeout value for BenchmarkLoadAsset* commands, in milliseconds");

    //! Polling frequency used by the load asset benchmark to detect load completion.
    AZ_CVAR(uint32_t, benchmarkLoadAssetPollMs, 20, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Sleep value for BenchmarkLoadAsset* commands between asset completion polls, in milliseconds");

    //! Optionally show progress messages during the load.
    //! Printing these messages can affect the overall timing, so this is off by default.
    AZ_CVAR(bool, benchmarkLoadAssetDisplayProgress, false, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Controls whether or not the BenchmarkLoadAsset* commands print progress messages while running");

    //! Label to add to the BenchmarkLoadAsset log outputs.
    AZ_CVAR(AZ::CVarFixedString, benchmarkLoadAssetLogLabel, "BenchmarkLoadAsset", nullptr, AZ::ConsoleFunctorFlags::Null,
        "Provide a log label for tagging the BenchmarkLoadAsset* outputs to make it easier to distinguish different benchmark runs.");

    static AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetType>> s_benchmarkAssetList;

    // Given a list of assets, load them and time the results.
    void BenchmarkLoadAssetList(AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetType>>&& sourceAssetList,
                                bool loadBlocking)
    {
        // Run the entire benchmark on its own thread so that the main thread can continue ticking along.

        AZStd::thread benchmarkThread([assetList = AZStd::move(sourceAssetList), loadBlocking]()
        {
            // Define the set of loading stats to track
            [[maybe_unused]] const size_t initialRequests = assetList.size();
            [[maybe_unused]] size_t previouslyLoadedAssets = 0;
            size_t newlyLoadedAssets = 0;
            size_t loadErrors = 0;

            // These are used to hold onto our Asset references until we're done to ensure that we aren't
            // immediately unloading and possibly reloading any assets along the way.
            AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> requestedAssets;
            AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> processedAssets;

            // Start timing.
            auto start = AZStd::chrono::steady_clock::now();

            // Part 1:  Queue up all the requested assets.
            // If loadBlocking is true, each asset will be synchronously and serially loaded.

            for (const auto& [assetId, assetType] : assetList)
            {
                // Check to see if the requested asset is already loaded or not.
                // We'll track the stats appropriately, and queue it if it isn't already loaded.
                AZ::Data::Asset<AZ::Data::AssetData> curAsset;
                curAsset = AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::Default);
                if (curAsset.IsReady())
                {
                    // The asset is already loaded, so track it as processed.
                    previouslyLoadedAssets++;
                    processedAssets.emplace_back(AZStd::move(curAsset));
                }
                else
                {
                    // The asset is *not* already loaded.  Queue it for loading.
                    requestedAssets.emplace_back(AZ::Data::AssetManager::Instance().GetAsset(assetId, assetType, AZ::Data::AssetLoadBehavior::Default));

                    if(loadBlocking)
                    {
                        requestedAssets.back().BlockUntilLoadComplete();
                    }
                }
            }

            // Part 2:  All the assets are queued, now wait for them all to complete.

            AZStd::chrono::milliseconds runMs{ 0 };
            const AZStd::chrono::milliseconds maxWaitMs{ benchmarkLoadAssetTimeoutMs };

            // Keep going until our assets are loaded or we time out.
            while (!requestedAssets.empty() && runMs < maxWaitMs)
            {
                // For all remaining assets, remove any that have finished or errored out
                // and track them in our stats.
                requestedAssets.erase(
                    AZStd::remove_if(requestedAssets.begin(), requestedAssets.end(),
                        [&loadErrors, &newlyLoadedAssets, &processedAssets](auto requestedAsset)
                    {
                        if (!requestedAsset || !requestedAsset.GetId().IsValid())
                        {
                            AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
                                "Asset %s type %s (%s) was not found",
                                requestedAsset.GetId().template ToString<AZStd::string>().c_str(),
                                requestedAsset.GetType().template ToString<AZStd::string>().c_str(),
                                requestedAsset.GetHint().c_str());
                            loadErrors++;
                            return true;
                        }
                        else if (requestedAsset.IsReady())
                        {
                            newlyLoadedAssets++;
                            processedAssets.emplace_back(AZStd::move(requestedAsset));
                            return true;
                        }
                        else if (requestedAsset.IsError())
                        {
                            AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
                                "Asset %s type %s (%s) had an error while loading",
                                requestedAsset.GetId().template ToString<AZStd::string>().c_str(),
                                requestedAsset.GetType().template ToString<AZStd::string>().c_str(),
                                requestedAsset.GetHint().c_str());
                            loadErrors++;
                            return true;
                        }

                        return false;
                    }),
                    requestedAssets.end());

                // Update our total running time so far.
                runMs = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - start);

                if (!requestedAssets.empty())
                {
                    // Only display progress messages if requested.  Beware, these can affect the benchmark timings.
                    if (benchmarkLoadAssetDisplayProgress)
                    {
                        AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
                            "%zu / %zu processed after %lld ms",
                            (previouslyLoadedAssets + newlyLoadedAssets + loadErrors), initialRequests, runMs.count());
                    }

                    // If polling produces too unstable of results, this could eventually get changed to listen
                    // for all the OnAssetReady messages.
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(benchmarkLoadAssetPollMs));
                }
            }

            // If we've timed out, provide some diagnostics on what went wrong.
            if (runMs >= maxWaitMs)
            {
                AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
                    "Request timed out, the following assets didn't load:\n");
                for (auto& requestedAsset [[maybe_unused]]: requestedAssets)
                {
                    AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
                        "%s: id=%s type=%s\n",
                        requestedAsset.GetHint().c_str(),
                        requestedAsset.GetId().template ToString<AZStd::string>().c_str(),
                        requestedAsset.GetType().template ToString<AZStd::string>().c_str()
                    );
                }
            }

            // Print out our benchmarking results.
            AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
                "Results: LoadRequests=%zu "
                "PreviouslyLoaded=%zu "
                "NewlyLoaded=%zu "
                "Errors=%zu "
                "TotalProcessed=%zu "
                "Time=%lld ms %s\n",
                initialRequests,
                previouslyLoadedAssets,
                newlyLoadedAssets,
                loadErrors,
                (previouslyLoadedAssets + newlyLoadedAssets + loadErrors),
                runMs.count(), (runMs >= maxWaitMs) ? "(request timed out)" : "");

            AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(), "Benchmark run complete.\n");

        });
        benchmarkThread.detach();

    }

    // Console command:  Clear the list of assets to use with BenchmarkLoadAssetList
    void BenchmarkClearAssetList([[maybe_unused]] const AZ::ConsoleCommandContainer& parameters)
    {
        s_benchmarkAssetList.clear();
        AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(), "Benchmark asset list cleared.\n");
    }

    // Console command:  Add the given list of assets to the list of assets to load with BenchmarkLoadAssetList
    void BenchmarkAddAssetsToList(const AZ::ConsoleCommandContainer& parameters)
    {
        [[maybe_unused]] bool allAssetsAdded = true;

        for (auto& assetName : parameters)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetInfo assetInfo;

            // For each name passed in, look up the Asset ID and Type
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                AZStd::string(assetName).c_str(), AZ::Data::s_invalidAssetType, false);
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById,
                assetId);

            // Only add the asset to our list if it's valid and there's an AssetHandler that can load it.
            if (assetId.IsValid())
            {
                if (AZ::Data::AssetManager::Instance().GetHandler(assetInfo.m_assetType))
                {
                    s_benchmarkAssetList.emplace_back(assetId, assetInfo.m_assetType);
                }
                else
                {
                    AZ_Error("AssetBenchmark", false, "No asset handler registered for asset: " AZ_STRING_FORMAT,
                        AZ_STRING_ARG(assetName));
                    allAssetsAdded = false;
                }
            }
            else
            {
                AZ_Error("AssetBenchmark", false, "Could not find asset: " AZ_STRING_FORMAT, AZ_STRING_ARG(assetName));
                allAssetsAdded = false;
            }
        }

        AZ_TracePrintf(static_cast<AZ::CVarFixedString>(benchmarkLoadAssetLogLabel).c_str(),
            allAssetsAdded ? "All requested assets added." : "One or more requested assets could not be added.");
    }


    // Console command:  Given a list of assets, asynchronously load them and time the results.
    void BenchmarkLoadAssetList(const AZ::ConsoleCommandContainer& parameters)
    {
        BenchmarkAddAssetsToList(parameters);

        if (!s_benchmarkAssetList.empty())
        {
            constexpr bool loadBlocking = false;

            // This intentionally does a 'move' of the list instead of a copy as a user convenience.  The assumed common use case
            // is that once the user has run a benchmark, they would like the asset list cleared before starting the next benchmark run.
            BenchmarkLoadAssetList(AZStd::move(s_benchmarkAssetList), loadBlocking);
        }
    }

    // Gather up all existing assets in the asset catalog and try to load them.
    static void BenchmarkLoadAllAssetsInternal(bool loadBlocking)
    {
        AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetType>> assetList;

        // For each asset in the asset catalog, only add it to our list of there's an
        // AssetHandler capable of loading it.
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB collectAssetsCb = [&]
            (const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
            {
                if (AZ::Data::AssetManager::Instance().GetHandler(info.m_assetType))
                {
                    assetList.emplace_back(id, info.m_assetType);
                }
            };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr, collectAssetsCb, nullptr);

        BenchmarkLoadAssetList(AZStd::move(assetList), loadBlocking);
    }

    // Console command:  Load all assets in the asset catalog asynchronously and time the results.
    void BenchmarkLoadAllAssets([[maybe_unused]] const AZ::ConsoleCommandContainer& parameters)
    {
        constexpr bool loadBlocking = false;
        BenchmarkLoadAllAssetsInternal(loadBlocking);
    }

    // Console command:  Load all assets in the asset catalog synchronously and time the results.
    void BenchmarkLoadAllAssetsSynchronous([[maybe_unused]] const AZ::ConsoleCommandContainer& parameters)
    {
        constexpr bool loadBlocking = true;
        BenchmarkLoadAllAssetsInternal(loadBlocking);
    }


    // Normally, the commands above would be registered by AZ_CONSOLEFREEFUNC here.  They specifically have been moved from
    // here to AssetSystemComponent.cpp to circumvent dead code stripping.  If they appear here, then the only references
    // to code within this file is self-contained to the file.  The C++ standard (3.6.2, 3.7.1) only guarantees that
    // a static variable is initialized before any other function in the compilation unit (.cpp file) is called.  If no other
    // functions are called, it's not guaranteed to get initialized at all, which leads to this entire file getting stripped
    // as unused.  By moving these lines to a different compilation unit, the file has external references and is no longer
    // unused.

} // namespace AzFramework::AssetBenchmark
