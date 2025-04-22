/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetSystemTestCommands.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Jobs/JobFunction.h>

#include <AzCore/Math/Random.h>

#include <AzCore/std/string/conversions.h>

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/NetworkAssetNotification_private.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AssetValidation
{
    void AssetValidation::TestChangeAssets(const AZ::ConsoleCommandContainer& someStrings)
    {
        static AZStd::atomic_bool forceStop{ false };

        AZStd::chrono::milliseconds runTime{ 60000 }; // Total test run time in milliseconds
        AZStd::chrono::milliseconds changeFreq{ 1000 }; // How often to perform the update cycle
        unsigned long changePercent = 10; // what percent of the assets should we simulate receiving AssetChanged messages for
        unsigned long holdPercent = 20; // What percent of assets should we get a reference to and hold each update
        unsigned long loadBlocking = 0; // What percent of root asset loads should be requested as blocking
        AZ::u64 seedValue = 1234;
        for (auto thisIndex = 0; thisIndex < someStrings.size(); ++thisIndex)
        {
            auto& thisString = someStrings[thisIndex];
            if (thisString == "stop")
            {
                AZ_TracePrintf("TestChangeAssets", "Stopping tests.");
                forceStop = true;
            }
            else if (thisString == "start")
            {
                AZ_TracePrintf("TestChangeAssets", "Enabling tests.");
                forceStop = false;
            }
            else if (thisIndex < someStrings.size() - 1)
            {
                if (thisString == "runtime")
                {
                    runTime = AZStd::chrono::milliseconds(AZStd::stoull(AZStd::string(someStrings[++thisIndex])));
                }
                else if (thisString == "frequency")
                {
                    changeFreq = AZStd::chrono::milliseconds(AZStd::stoull(AZStd::string(someStrings[++thisIndex])));
                }
                else if (thisString == "change")
                {
                    changePercent = AZStd::stoul(AZStd::string(someStrings[++thisIndex]));
                }
                else if (thisString == "hold")
                {
                    holdPercent = AZStd::stoul(AZStd::string(someStrings[++thisIndex]));
                }
                else if (thisString == "loadblocking")
                {
                    loadBlocking = AZStd::stoul(AZStd::string(someStrings[++thisIndex]));
                }
                else if (thisString == "seed")
                {
                    seedValue = AZStd::stoull(AZStd::string(someStrings[++thisIndex]));
                }
            }
        }

        AZ_TracePrintf("TestChangeAssets", "%s: Running for %llu ms freq %llu holding %u percent changing %u percent blocking %u percent seed value %llu\n", forceStop ? "STOPPED" : "START", runTime, changeFreq, holdPercent, changePercent, loadBlocking,seedValue);
        auto runJob = AZ::CreateJobFunction([runTime, changeFreq, changePercent, holdPercent, loadBlocking, seedValue]()
        {
            AZStd::vector<AZStd::pair<AZ::Data::AssetId, AzFramework::AssetSystem::AssetNotificationMessage>> assetList;

            AZ::Data::AssetCatalogRequests::AssetEnumerationCB collectAssetsCb = [&](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
            {
                if (AZ::Data::AssetManager::Instance().GetHandler(info.m_assetType))
                {
                    AZ::Outcome<AZStd::vector<AZ::Data::ProductDependency>, AZStd::string> result;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(result, &AZ::Data::AssetCatalogRequestBus::Events::GetDirectProductDependencies, id);

                    AzFramework::AssetSystem::AssetNotificationMessage message;
                    message.m_assetId = id;
                    message.m_data = info.m_relativePath;
                    message.m_sizeBytes = info.m_sizeBytes;
                    message.m_assetType = info.m_assetType;
                    // Assets with no dependencies won't return a successful response
                    if (result.IsSuccess())
                    {
                        message.m_dependencies = result.TakeValue();
                    }
                    auto somePair = AZStd::make_pair(id, message);
                    assetList.push_back(somePair);
                }
            };

            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, collectAssetsCb, nullptr);

            auto start = AZStd::chrono::steady_clock::now();

            AZStd::chrono::milliseconds runMs{ 0 };

            AzFramework::AssetSystem::NetworkAssetUpdateInterface* notificationInterface = AZ::Interface<AzFramework::AssetSystem::NetworkAssetUpdateInterface>::Get();
            if (!notificationInterface)
            {
                AZ_Warning("TestChangeAssets", false, "Couldn't get notification interface to send change messages\n");
                return;
            }
            AZ::SimpleLcgRandom randomizer(seedValue);
            int lastTick = 0;
            AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> heldAssets;
            [[maybe_unused]] AZStd::size_t heldCount{ 0 };
            [[maybe_unused]] AZ::u64 changeCount{ 0 };
            [[maybe_unused]] AZ::u64 blockCount{ 0 };
            AZ_TracePrintf("TestChangeAssets", "Beginning run with %zu assets\n", assetList.size());
            while (!forceStop && runMs < runTime)
            {
                runMs = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - start);
                int thisTick = aznumeric_cast<int>(runMs / changeFreq) + 1;
                if (thisTick > lastTick)
                {
                    heldAssets.clear();
                    lastTick = thisTick;
                    AZ::u64 thisChangeCount{ 0 };
                    AZ::u64 thisBlockCount{ 0 };
                    for (const auto& thisElement : assetList)
                    {
                        bool blockLoad = (randomizer.GetRandom() % 100 < loadBlocking);
                        if (randomizer.GetRandom() % 100 < holdPercent)
                        {
                            heldAssets.emplace_back(AZ::Data::AssetManager::Instance().GetAsset(thisElement.first, thisElement.second.m_assetType, AZ::Data::AssetLoadBehavior::Default));
                            if (blockLoad)
                            {
                                heldAssets.back().BlockUntilLoadComplete();

                                ++thisBlockCount;
                            }
                        }
                        if (randomizer.GetRandom() % 100 < changePercent)
                        {
                            ++thisChangeCount;
                            notificationInterface->AssetChanged({ thisElement.second });
                        }
                    }

                    AZ_TracePrintf("TestChangeAssets", "On Tick %d held %zu assets, block loaded %llu and queued changes for %llu\n", lastTick, heldAssets.size(), thisBlockCount, thisChangeCount);
                    heldCount += heldAssets.size();
                    blockCount += thisBlockCount;
                    changeCount += thisChangeCount;
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(300));
                }
            }
            AZ_TracePrintf("TestChangeAssets", "DONE: After %llu ms held %zu assets, block loaded %llu and queued changes for %llu\n", runMs, heldCount, blockCount, changeCount);

        }, true);
        runJob->Start();
    }
}

void TestCreateContainers([[maybe_unused]] const AZ::ConsoleCommandContainer& someStrings)
{
    auto runJob = AZ::CreateJobFunction([]()
    {
        AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetInfo>> assetList;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB collectAssetsCb = [&](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
        {
            if (AZ::Data::AssetManager::Instance().GetHandler(info.m_assetType))
            {
                auto somePair = AZStd::make_pair(id, info);
                assetList.push_back(somePair);
            }
        };

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, collectAssetsCb, nullptr);

        auto start = AZStd::chrono::steady_clock::now();

        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> loadingContainers;
        for (const auto& thisElement : assetList)
        {
            loadingContainers.push_back(AZ::Data::AssetManager::Instance().GetAsset(thisElement.first, thisElement.second.m_assetType, AZ::Data::AssetLoadBehavior::Default));
        }

        AZStd::chrono::milliseconds maxWait{ 5000 };
        AZStd::chrono::milliseconds runMS{ 0 };
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> readyContainers;
        size_t totalContainers = loadingContainers.size();
        while(readyContainers.size() < totalContainers && runMS < maxWait)
        {
            auto containerIter = loadingContainers.begin();
            while (containerIter != loadingContainers.end())
            {
                if ((*containerIter)->IsReady())
                {
                    readyContainers.push_back(*containerIter);
                    containerIter = loadingContainers.erase(containerIter);
                }
                else
                {
                    ++containerIter;
                }
            }
            runMS = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - start);
            if (readyContainers.size() == totalContainers)
            {
                AZ_TracePrintf("TestCreateContainers", "All assets (%d) ready after %ld ms\n", readyContainers.size(), runMS);
                return;
            }
            AZ_TracePrintf("TestCreateContainers", "%d / %d ready after %ld ms\n", readyContainers.size(), totalContainers, runMS);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        }
#if defined(AZ_ENABLE_TRACING)
        if (loadingContainers.size())
        {
            AZ_TracePrintf("TestCreateContainers", "Failed to load %d / %d containers\n", loadingContainers.size(), totalContainers);
            for (auto& thisContainer : loadingContainers)
            {
                AZ_TracePrintf("TestCreateContainers", "Couldn't load container for %s\n", thisContainer.ToString<AZStd::string>().c_str());
            }
        }
#endif
    }, true);
    runJob->Start();
}

AZ_CONSOLEFREEFUNC(TestCreateContainers, AZ::ConsoleFunctorFlags::Null, "Time the creation of all assets in the catalog as containers");

void TestSingleContainer(const AZ::ConsoleCommandContainer& someStrings)
{
    if (someStrings.empty())
    {
        AZ_Warning("TestSingleContainer", false, "Need a valid id to test against");
        return;
    }

    auto uuidString = someStrings[0];
    AZ::u32 subId{0};
    auto subidPos = uuidString.rfind(':');
    if (subidPos != AZStd::string::npos)
    {
        auto subidstr = uuidString.substr(subidPos + 1);
        subId = atoi(subidstr.data());
        uuidString = uuidString.substr(0, subidPos);
    }
    AZ::Data::AssetId assetId(AZ::Uuid::CreateStringPermissive(uuidString.data()), subId);
    auto runJob = AZ::CreateJobFunction([assetId]()
    {
        AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZ::Data::AssetInfo>> assetList;

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);

        if (!assetInfo.m_assetId.IsValid())
        {
            AZ_Warning("TestSingleContainer", false, "Couldn't get asset info for %s", assetId.ToString<AZStd::string>().c_str());
        }
        auto start = AZStd::chrono::steady_clock::now();

        auto thisContainer = AZ::Data::AssetManager::Instance().GetAsset(assetId, assetInfo.m_assetType, AZ::Data::AssetLoadBehavior::Default);

        AZStd::chrono::milliseconds maxWait{ 2000 };
        AZStd::chrono::milliseconds runMS{ 0 };

        while(thisContainer->IsLoading() && runMS < maxWait)
        {
            runMS = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - start);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
        }
        if (thisContainer->IsReady())
        {
            AZ_TracePrintf("TestCreateContainers", "Container for %s loaded\n", assetId.ToString<AZStd::string>().c_str());
        }
        else
        {
            AZ_TracePrintf("TestCreateContainers", "Failed to load container for %s\n", assetId.ToString<AZStd::string>().c_str());
        }
    }, true);
    runJob->Start();
}

AZ_CONSOLEFREEFUNC(TestSingleContainer, AZ::ConsoleFunctorFlags::Null, "Test the creation of a single container");

void ShowAsset(const AZ::ConsoleCommandContainer& someStrings)
{
    if (someStrings.empty())
    {
        AZ_Warning("TestSingleContainer", false, "Need a valid id to show");
        return;
    }

    auto uuidString = someStrings[0];
    AZ::u32 subId{ 0 };
    auto subidPos = uuidString.rfind(':');
    if (subidPos != AZStd::string::npos)
    {
        auto subidstr = uuidString.substr(subidPos + 1);
        subId = atoi(subidstr.data());
        uuidString = uuidString.substr(0, subidPos);
    }
    AZ::Data::AssetId assetId(AZ::Uuid::CreateStringPermissive(uuidString.data()), subId);
    AZ::Data::AssetInfo assetInfo;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, assetId);

    if (!assetInfo.m_assetId.IsValid())
    {
        AZ_Warning("TestSingleContainer", false, "Couldn't get asset info for %s", assetId.ToString<AZStd::string>().c_str());
        return;
    }
    AZ_TracePrintf("ShowAsset", "Asset %s : Type %s Size %zu RelativePath %s\n", assetId.ToString<AZStd::string>().c_str(), assetInfo.m_assetType.ToString<AZStd::string>().c_str(), assetInfo.m_sizeBytes, assetInfo.m_relativePath.c_str());

}

AZ_CONSOLEFREEFUNC(ShowAsset, AZ::ConsoleFunctorFlags::Null, "Print info on a single asset");
