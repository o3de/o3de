/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"
#include "ScriptEventSystem.h"
#include "ScriptEventDefinition.h"
#include "ScriptEventsAsset.h"
#include "ScriptEvent.h"

namespace ScriptEvents
{
    AZStd::intrusive_ptr<Internal::ScriptEventRegistration> ScriptEventsSystemComponentImpl::RegisterScriptEvent(const AZ::Data::AssetId& assetId, AZ::u32 /*version*/)
    {
        AZ_Assert(assetId.IsValid(), "Unable to register Script Event with invalid asset Id");
        if (!assetId.IsValid())
        {
            return nullptr;
        }

        ScriptEventKey key(assetId, 0);

        if (m_scriptEvents.find(key) == m_scriptEvents.end())
        {
            m_scriptEvents[key] = AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration>(aznew ScriptEvents::Internal::ScriptEventRegistration(assetId));
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
            AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().CreateAsset<ScriptEvents::ScriptEventsAsset>(assetId);

            // Install the definition that's coming from Lua
            ScriptEvents::ScriptEventsAsset* scriptAsset = assetData.Get();
            scriptAsset->m_definition = definition;

            m_scriptEvents[key] = AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration>(aznew ScriptEvents::Internal::ScriptEventRegistration(assetId));
            m_scriptEvents[key]->CompleteRegistration(assetData);

        }
    }

    void ScriptEventsSystemComponentImpl::UnregisterScriptEventFromDefinition(const ScriptEvents::ScriptEvent& definition)
    {
        const AZStd::string& busName = definition.GetName();
        const AZ::Uuid& assetId = AZ::Uuid::CreateName(busName.c_str());

        AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().FindAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
        if (assetData)
        {
            assetData.Release();
        }
    }

    AZStd::intrusive_ptr<Internal::ScriptEventRegistration> ScriptEventsSystemComponentImpl::GetScriptEvent(const AZ::Data::AssetId& assetId, [[maybe_unused]] AZ::u32 version)
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
