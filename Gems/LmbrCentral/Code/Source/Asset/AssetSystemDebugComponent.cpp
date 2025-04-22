/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AssetSystemDebugComponent.h"
#include "ISystem.h"

#include "AzCore/Asset/AssetManager.h"
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    AZ_CVAR(bool, cl_assetStatusDebugActiveAssets, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Show debug stats about loading assets."
        " Data is not collected while disabled so it is recommended to enable this via command line or config");

    AZ_CVAR(bool, cl_assetStatusDebugLoadedAssets, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Show debug stats about loaded assets."
        " Data is not collected while disabled so it is recommended to enable this via command line or config");

    AZ_CVAR(std::uint8_t, cl_assetStatusDebugDisplayCount, 20, nullptr, AZ::ConsoleFunctorFlags::Null,
        "Sets the max number of assets to record and display in debug stats.  This will only update after more assets have loaded.");

    void AssetSystemDebugComponent::Activate()
    {
        BusConnect();
        AZ::Interface<IDebugAssetEvent>::Register(this);
    }

    void AssetSystemDebugComponent::Deactivate()
    {
        AZ::Interface<IDebugAssetEvent>::Unregister(this);
        BusDisconnect();
    }

    AZStd::string StatusToString(AZ::Data::AssetData::AssetStatus status)
    {
        switch (status)
        {
            case AZ::Data::AssetData::AssetStatus::NotLoaded:
                return "Not Loaded";
            case AZ::Data::AssetData::AssetStatus::Queued:
                return "Queued";
            case AZ::Data::AssetData::AssetStatus::StreamReady:
                return "Stream Ready";
            case AZ::Data::AssetData::AssetStatus::Loading:
                return "Loading";
            case AZ::Data::AssetData::AssetStatus::LoadedPreReady:
                return "Loaded Pre-Ready";
            case AZ::Data::AssetData::AssetStatus::ReadyPreNotify:
                return "Ready Pre-Notify";
            case AZ::Data::AssetData::AssetStatus::Ready:
                return "Ready";
            case AZ::Data::AssetData::AssetStatus::Error:
                return "Error";
            default:
                return "Unknown State";
        }
    }

    void AssetSystemDebugComponent::DrawGlobalDebugInfo()
    {
        if(!cl_assetStatusDebugActiveAssets && !cl_assetStatusDebugLoadedAssets)
        {
            return;
        }

        // ToDo: Remove class or update to work with Atom? LYN-3672
        /*float x = 10;
        float y = 15;
        ColorF color(1, 1, 1);
        constexpr bool center = false;
        constexpr float ySpacing = 14;
        constexpr float fontSize = 1.3;
        const int maxCount = cl_assetStatusDebugDisplayCount;

        if (cl_assetStatusDebugActiveAssets)
        {
            // Make a local copy of the "oldest active" list.  We don't want to hold the eventMutex while calling FindAsset below
            // or else we open up the possibility of lock inversion deadlocks.  This routines would lock eventMutex then AssetManager
            // locks, but modifications to the load data history can happen from multiple loading threads that lock AssetManager mutexes
            // before locking the eventMutex.
            AZStd::vector<AZ::Data::AssetId> oldestActive(m_oldestActive.size());
            {
                AZStd::scoped_lock lock(m_eventMutex);
                for (auto& info : m_oldestActive)
                {
                    oldestActive.emplace_back(info->m_id);
                }
            }

            auxGeom->Draw2dLabel(x, y, fontSize, color, center, "Last %d assets to start loading, oldest to newest:", maxCount);

            y += ySpacing; // Extra space after the "title"

            for (auto& oldestActiveId : oldestActive)
            {
                y += ySpacing;

                auto asset = AZ::Data::AssetManager::Instance().FindAsset(oldestActiveId, AZ::Data::AssetLoadBehavior::Default);

                auxGeom->Draw2dLabel(x, y, fontSize, color, center, "%s - %s",
                    asset.GetHint().length() > 0 ? asset.GetHint().c_str() : oldestActiveId.ToString<AZStd::string>().c_str(),
                    StatusToString(asset.GetStatus()).c_str());
            }
        }
        if (cl_assetStatusDebugLoadedAssets)
        {
            // Same as above - make a local copy of the list to avoid lock inversion deadlocks.
            AZStd::vector<AZStd::pair<AZ::Data::AssetId, AZStd::chrono::duration<float, AZStd::milli>>> activeAssets(m_recentlyCompleted.size());
            {
                AZStd::scoped_lock lock(m_eventMutex);
                for (auto& info : m_recentlyCompleted)
                {
                    activeAssets.emplace_back(info->m_id, info->m_loadFinish - info->m_loadStart);
                }
            }

            if(cl_assetStatusDebugActiveAssets)
            {
                y = 15;
                x = 1200;
            }

            auxGeom->Draw2dLabelCustom(x, y, fontSize, color, eDrawText_Right, "Last %d assets loaded, most recent first:", maxCount);

            y += ySpacing; // Extra space after the "title"

            for (auto& [activeAssetId, activeAssetLoadTime] : activeAssets)
            {
                y += ySpacing;

                auto asset = AZ::Data::AssetManager::Instance().FindAsset(activeAssetId, AZ::Data::AssetLoadBehavior::Default);

                auxGeom->Draw2dLabelCustom(x, y, fontSize, color, eDrawText_Right, "%s - %s - %fms",
                    asset.GetHint().length() > 0 ? asset.GetHint().c_str() : activeAssetId.ToString<AZStd::string>().c_str(),
                    StatusToString(asset.GetStatus()).c_str(),
                    activeAssetLoadTime.count());
            }
        }*/
    }

    void AssetSystemDebugComponent::AssetStatusUpdate(AZ::Data::AssetId id, AZ::Data::AssetData::AssetStatus status)
    {
        using namespace AZ::Data;

        if (!cl_assetStatusDebugActiveAssets && !cl_assetStatusDebugLoadedAssets)
        {
            return;
        }

        AZStd::scoped_lock lock(m_eventMutex);

        auto itr = m_events.find(id);
        bool exists = itr != m_events.end();

        if (exists)
        {
            itr->second.m_status = status;
        }

        switch (status)
        {
            case AssetData::AssetStatus::Queued:
            {
                EventInfo info;
                info.m_id = id;
                info.m_status = status;
                info.m_loadStart = AZStd::chrono::steady_clock::now();

                m_events[id] = info;
                m_oldestActive.insert(&m_events[id]);

                while (m_oldestActive.size() > cl_assetStatusDebugDisplayCount)
                {
                    m_oldestActive.erase(--(m_oldestActive.end()));
                }
            }
            break;
            case AssetData::AssetStatus::Error:
            case AssetData::AssetStatus::ReadyPreNotify:
            {
                if(exists)
                {
                    itr->second.m_loadFinish = AZStd::chrono::steady_clock::now();

                    m_recentlyCompleted.insert(&itr->second);
                    m_oldestActive.erase(&itr->second);

                    while (m_recentlyCompleted.size() > cl_assetStatusDebugDisplayCount)
                    {
                        auto last = --(m_recentlyCompleted.end());
                        m_events.erase((*last)->m_id);
                        m_recentlyCompleted.erase(last);
                    }
                }
            }
            break;

        }
    }

    void AssetSystemDebugComponent::ReleaseAsset(AZ::Data::AssetId id)
    {
        AZStd::scoped_lock lock(m_eventMutex);

        auto itr = m_events.find(id);

        if(itr != m_events.end())
        {
            m_oldestActive.erase(&itr->second);
            m_recentlyCompleted.erase(&itr->second);
            m_events.erase(itr);
        }
    }

    void AssetSystemDebugComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AssetSystemDebug"));
    }

    void AssetSystemDebugComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AssetSystemDebug"));
    }

    void AssetSystemDebugComponent::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serialize->Class<AssetSystemDebugComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

}
