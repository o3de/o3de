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

#include <AzCore/Component/Component.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>

namespace LmbrCentral
{
    class AssetSystemDebugComponent
        : public AZ::Component
        , AzFramework::DebugDisplayEventBus::Handler
        , AZ::Data::IDebugAssetEvent
    {
    public:
        AZ_COMPONENT(AssetSystemDebugComponent, "{2DB77E66-67A5-4E56-B2FF-75C718B182A1}", AZ::Component);

        AssetSystemDebugComponent() = default;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // DebugDisplayEventBus
        void DrawGlobalDebugInfo() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // IDebugAssetEvent
        void AssetStatusUpdate(AZ::Data::AssetId id, AZ::Data::AssetData::AssetStatus status) override;
        void ReleaseAsset(AZ::Data::AssetId id) override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* reflection);

        struct EventInfo
        {
            AZ::Data::AssetId m_id;
            AZ::Data::AssetData::AssetStatus m_status;
            AZStd::chrono::system_clock::time_point m_loadStart;
            AZStd::chrono::system_clock::time_point m_loadFinish;
        };

        struct EventSortOldest
        {
            bool operator()(const EventInfo* a, const EventInfo* b) const
            {
                return a->m_loadStart > b->m_loadStart;
            }
        };

        struct EventSortMostRecentCompleted
        {
            bool operator()(const EventInfo* a, const EventInfo* b)
            {
                return a->m_loadFinish > b->m_loadFinish;
            }
        };

        AZStd::recursive_mutex m_eventMutex;

        AZStd::unordered_map<AZ::Data::AssetId, EventInfo> m_events;
        AZStd::set<EventInfo*, EventSortOldest> m_oldestActive;
        AZStd::set<EventInfo*, EventSortMostRecentCompleted> m_recentlyCompleted;
    };
}
