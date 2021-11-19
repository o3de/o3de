/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventBase.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/TickBus.h>
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

                AZ::Outcome<DependencyReport, void> ScriptEventBase::GetDependencies() const
                {
                    if (!m_asset.GetId().IsValid())
                    {
                        return AZ::Failure();
                    }

                    DependencyReport report;
                    report.scriptEventsAssetIds.insert({ m_asset.GetId() });
                    return AZ::Success(AZStd::move(report));
                }

                void ScriptEventBase::Initialize(const AZ::Data::AssetId assetId)
                {
                    m_asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                    m_asset.BlockUntilLoadComplete();

                    if (assetId.IsValid())
                    {
                        m_scriptEventAssetId = assetId;

                        GraphRequestBus::Event(GetOwningScriptCanvasId(), &GraphRequests::AddDependentAsset, GetEntityId(), azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), assetId);

                        AZ::Data::AssetBus::Handler::BusConnect(assetId);
                    }
                }

                AZStd::pair<AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>, bool> ScriptEventBase::IsAssetOutOfDate() const
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(GetAssetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                    assetData.BlockUntilLoadComplete();

                    // #conversion_diagnostic
                    if (assetData)
                    {
                        ScriptEvents::ScriptEvent& definition = assetData.Get()->m_definition;
                        if (GetVersion() != definition.GetVersion())
                        {
                            AZ_TracePrintf("ScriptCanvas", "ScriptEvent Node %s version has updated. This node will be considered out of date.", GetDebugName().data());
                            return { assetData, true };
                        }
                        else
                        {
                            return { assetData, false };
                        }
                    }
                    else
                    {
                        AZ_Warning("ScriptCanvas", false, "ScriptEvent Node %s failed to load latest interface from the source asset. This node will be disabled in the graph, and the graph will not parse", GetDebugName().data());
                        return  { assetData, true };
                    }
                }

                void ScriptEventBase::OnActivate()
                {
                    m_asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_scriptEventAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                    m_asset.BlockUntilLoadComplete();
                }

                void ScriptEventBase::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
                {
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                    assetData.BlockUntilLoadComplete();

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
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
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
            }
        }
    }
}
