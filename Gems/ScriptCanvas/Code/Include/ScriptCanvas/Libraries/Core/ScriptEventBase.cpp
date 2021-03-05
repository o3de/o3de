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

#include "ScriptEventBase.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TickBus.h>

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                bool ScriptEventBaseVersionConverter([[maybe_unused]] AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
                {
                    if (rootElement.GetVersion() < 6)
                    {
                        rootElement.RemoveElementByName(AZ_CRC_CE("m_asset"));
                    }

                    return true;
                }

                void ScriptEventEntry::Reflect(AZ::ReflectContext* context)
                {
                    if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
                    {
                        if (auto genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_map<AZ::Crc32, ScriptEventEntry> >::GetGenericInfo())
                        {
                            genericClassInfo->Reflect(serialize);
                        }

                        serialize->Class<ScriptEventEntry>()
                            ->Version(1)
                            ->Field("m_scriptEventAssetId", &ScriptEventEntry::m_scriptEventAssetId)
                            ->Field("m_eventName", &ScriptEventEntry::m_eventName)
                            ->Field("m_eventSlotId", &ScriptEventEntry::m_eventSlotId)
                            ->Field("m_resultSlotId", &ScriptEventEntry::m_resultSlotId)
                            ->Field("m_parameterSlotIds", &ScriptEventEntry::m_parameterSlotIds)
                            ->Field("m_numExpectedArguments", &ScriptEventEntry::m_numExpectedArguments)
                            ->Field("m_resultEvaluated", &ScriptEventEntry::m_resultEvaluated)
                            ;
                    }
                }

                ScriptEventBase::ScriptEventBase()
                    : m_version(0)
                    , m_scriptEventAssetId(0)
                    , m_asset(AZ::Data::AssetLoadBehavior::PreLoad)
                {
                }

                ScriptEventBase::~ScriptEventBase()
                {
                    ScriptCanvas::ScriptEventNodeRequestBus::Handler::BusDisconnect();
                    AZ::Data::AssetBus::Handler::BusDisconnect();
                }

                void ScriptEventBase::OnInit()
                {
                    ScriptCanvas::ScriptEventNodeRequestBus::Handler::BusConnect(GetEntityId());

                    if (m_scriptEventAssetId.IsValid())
                    {
                        Initialize(m_scriptEventAssetId);
                    }
                }

                void ScriptEventBase::Initialize(const AZ::Data::AssetId assetId)
                {
                    m_asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<ScriptEvents::ScriptEventsAsset>(assetId, m_asset.GetAutoLoadBehavior());

                    if (assetId.IsValid())
                    {
                        m_scriptEventAssetId = assetId;

                        GraphRequestBus::Event(GetOwningScriptCanvasId(), &GraphRequests::AddDependentAsset, GetEntityId(), azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), assetId);

                        AZ::Data::AssetBus::Handler::BusConnect(assetId);
                    }
                }

                void ScriptEventBase::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(asset.GetId(), AZ::Data::AssetLoadBehavior::Default);
                    m_definition = assetData.Get()->m_definition;
                    OnScriptEventReady(asset);
                }

                void ScriptEventBase::OnAssetChanged(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&, void* userData)
                {
                    ScriptEventBase* node = static_cast<ScriptEventBase*>(userData);
                    (void)node;
                }

                void ScriptEventBase::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(asset.GetId(), AZ::Data::AssetLoadBehavior::Default);
                    ScriptEvents::ScriptEvent& definition = assetData.Get()->m_definition;
                    if (definition.GetVersion() > m_version)
                    {
                        // The asset has changed.
                        ScriptEvents::ScriptEventBus::BroadcastResult(m_scriptEvent, &ScriptEvents::ScriptEventRequests::RegisterScriptEvent, m_scriptEventAssetId, definition.GetVersion());
                    }

                    m_definition = definition;
                 }

                void ScriptEventBase::OnDeactivate()
                {
                    ScriptCanvas::ScriptEventNodeRequestBus::Handler::BusDisconnect();
                    AZ::Data::AssetBus::Handler::BusDisconnect();
                }
            } // namespace Internal
        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas
