/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventRegistration.h"
#include "ScriptEvent.h"
#include "ScriptEventsBus.h"

namespace ScriptEvents
{
    namespace Internal
    {

        ScriptEventRegistration::~ScriptEventRegistration()
        {
            for (auto ebusPair : m_behaviorEBus)
            {
                Utils::DestroyScriptEventBehaviorEBus(ebusPair.second->m_name);
            }

            m_scriptEventBindings.clear();

            AZ::SystemTickBus::Handler::BusDisconnect();
        }

        void ScriptEventRegistration::Init(AZ::Data::AssetId scriptEventAssetId)
        {
            AZ_Assert(scriptEventAssetId.IsValid(), "Script Event requires a valid Asset Id");

            m_assetId = scriptEventAssetId;

            m_asset = AZ::Data::AssetManager::Instance().FindAsset<ScriptEvents::ScriptEventsAsset>(m_assetId, AZ::Data::AssetLoadBehavior::PreLoad);
            if (m_asset && m_asset.IsReady())
            {
                CompleteRegistration(m_asset);
            }

            // Connect *after* checking to see if we can complete registration. Connections can potentially trigger
            // an immediate call to OnAssetReady(). If this happens, we'd like to make sure it doesn't try to connect
            // to the SystemTickBus. In part, it's because it would be redundant work - we've already completed the registration.
            // But also, the SystemTickBus can only be connected to from the main thread, and this Init() call might be
            // on a job thread, so we also want to avoid the unsafe connection to SystemTickBus in that scenario.
            AZ::Data::AssetBus::Handler::BusConnect(scriptEventAssetId);
        }

        void ScriptEventRegistration::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            // If m_isReady is true, we've already called CompleteRegistration for this asset.
            if (!m_isReady)
            {
                m_asset = asset;
                AZ::SystemTickBus::Handler::BusConnect();
            }
        }

        void ScriptEventRegistration::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            m_asset = asset;
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void ScriptEventRegistration::OnSystemTick()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();

            CompleteRegistration(m_asset);
        }

        void ScriptEventRegistration::CompleteRegistration(AZ::Data::Asset<AZ::Data::AssetData> asset)
        {
            OnScopeEnd clearAsset([&]() { this->m_asset = {}; });

            if (!asset)
            {
                return;
            }

            m_assetId = asset.GetId();
            const ScriptEvents::ScriptEvent& definition = asset.GetAs<ScriptEvents::ScriptEventsAsset>()->m_definition;

            if (m_behaviorEBus.find(definition.GetVersion()) != m_behaviorEBus.end())
            {
                return;
            }

            AZ::BehaviorContext* behaviorContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationBus::Events::GetBehaviorContext);
            AZ_Assert(behaviorContext, "Script Events require a valid Behavior Context");

            m_busName = definition.GetName();

            auto behaviorEbusEntry = behaviorContext->m_ebuses.find(definition.GetBehaviorContextName());
            if (behaviorEbusEntry != behaviorContext->m_ebuses.end())
            {
                m_behaviorEBus[definition.GetVersion()] = behaviorEbusEntry->second;

                if (m_maxVersion < definition.GetVersion())
                {
                    m_maxVersion = definition.GetVersion();
                }

                m_scriptEventBindings[m_assetId] = AZStd::make_unique<ScriptEventBinding>(behaviorContext, m_busName.c_str(), definition.GetAddressType());

                ScriptEventNotificationBus::Broadcast(&ScriptEventNotifications::OnRegistered, definition);

                return;
            }

            AZ::BehaviorEBus* bus = Utils::ConstructAndRegisterScriptEventBehaviorEBus(definition);

            if (bus == nullptr)
            {
                return;
            }

            m_behaviorEBus[definition.GetVersion()] = bus;

            if (m_maxVersion < definition.GetVersion())
            {
                m_maxVersion = definition.GetVersion();
            }

            AZ::BehaviorContextBus::Event(behaviorContext, &AZ::BehaviorContextBus::Events::OnAddEBus, m_busName.c_str(), bus);
            m_scriptEventBindings[m_assetId] = AZStd::make_unique<ScriptEventBinding>(behaviorContext, m_busName.c_str(), definition.GetAddressType());

            ScriptEventNotificationBus::Event(m_assetId, &ScriptEventNotifications::OnRegistered, definition);
            m_isReady = true;
        }

        bool ScriptEventRegistration::GetMethod(AZStd::string_view eventName, AZ::BehaviorMethod*& outMethod)
        {
            AZ::BehaviorEBus* ebus = GetBehaviorBus();
            AZ_Assert(ebus, "BehaviorEBus is invalid: %s", m_busName.c_str());

            const auto& method = ebus->m_events.find(eventName);
            if (method == ebus->m_events.end())
            {
                AZ_Error("Script Events", false, "No method by name of %s found in the script event: %s", eventName.data(), m_busName.c_str());
                return false;
            }

            AZ::EBusAddressPolicy addressPolicy
                = (ebus->m_idParam.m_typeId.IsNull() || ebus->m_idParam.m_typeId == AZ::AzTypeInfo<void>::Uuid())
                ? AZ::EBusAddressPolicy::Single
                : AZ::EBusAddressPolicy::ById;

            AZ::BehaviorMethod* behaviorMethod
                = ebus->m_queueFunction
                ? (addressPolicy == AZ::EBusAddressPolicy::ById ? method->second.m_queueEvent : method->second.m_queueBroadcast)
                : (addressPolicy == AZ::EBusAddressPolicy::ById ? method->second.m_event : method->second.m_broadcast);

            if (!behaviorMethod)
            {
                AZ_Error("Script Canvas", false, "Queue function mismatch in %s-%s", eventName.data(), m_busName.c_str());
                return false;
            }

            outMethod = behaviorMethod;
            return true;
        }
    }
}
