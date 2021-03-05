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

#include "precompiled.h"
#include "ScriptEventSystem.h"
#include "ScriptEventDefinition.h"
#include "ScriptEventsAsset.h"

namespace ScriptEvents
{
    AZStd::intrusive_ptr<Internal::ScriptEvent> ScriptEventsSystemComponentImpl::RegisterScriptEvent(const AZ::Data::AssetId& assetId, [[maybe_unused]] AZ::u32 version)
    {
        AZ_Assert(assetId.IsValid(), "Unable to register Script Event with invalid asset Id");
        if (!assetId.IsValid())
        {
            return nullptr;
        }

        ScriptEventKey key(assetId, 0);

        if (m_scriptEvents.find(key) == m_scriptEvents.end())
        {
            m_scriptEvents[key] = AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>(aznew ScriptEvents::Internal::ScriptEvent(assetId));
        }

        return m_scriptEvents[key];
    }


    void ScriptEventsSystemComponentImpl::RegisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);

        const AZStd::string& busName = definition.GetName();
        const AZ::Uuid& assetId = AZ::Uuid::CreateName(busName.c_str());
        ScriptEventKey key(assetId, 0);

        const auto& ebusIterator = behaviorContext->m_ebuses.find(busName);
        if (ebusIterator != behaviorContext->m_ebuses.end() && m_scriptEvents.find(key) != m_scriptEvents.end())
        {
            // We have already registered this Script Event, so we don't need to do anything further
            return;
        }


        if (m_scriptEvents.find(key) == m_scriptEvents.end())
        {
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().CreateAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);

            // Install the definition that's coming from Lua
            ScriptEvents::ScriptEventsAsset* scriptAsset = assetData.Get();
            scriptAsset->m_definition = definition;

            m_scriptEvents[key] = AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEvent>(aznew ScriptEvents::Internal::ScriptEvent(assetId));
            m_scriptEvents[key]->CompleteRegistration(assetData);

        }
    }

    void ScriptEventsSystemComponentImpl::UnregisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition)
    {
        const AZStd::string& busName = definition.GetName();
        const AZ::Uuid& assetId = AZ::Uuid::CreateName(busName.c_str());

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().FindAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
        if (assetData)
        {
            assetData.Release();
        }
    }

    AZStd::intrusive_ptr<Internal::ScriptEvent> ScriptEventsSystemComponentImpl::GetScriptEvent(const AZ::Data::AssetId& assetId, [[maybe_unused]] AZ::u32 version)
    {
        ScriptEventKey key(assetId, 0);

        if (m_scriptEvents.find(key) != m_scriptEvents.end())
        {
            return m_scriptEvents[key];
        }

        AZ_Warning("Script Events", false, "Script event with asset Id %s was not found (version %d)", assetId.ToString<AZStd::string>().c_str(), version);

        return nullptr;
    }

    const ScriptEvents::FundamentalTypes* ScriptEventsSystemComponentImpl::GetFundamentalTypes()
    {
        return &m_fundamentalTypes;
    }

}
