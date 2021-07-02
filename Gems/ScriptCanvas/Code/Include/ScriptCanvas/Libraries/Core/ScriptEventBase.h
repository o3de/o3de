/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Include/ScriptCanvas/Libraries/Core/ScriptEventBase.generated.h>

#include <AzCore/std/containers/map.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Variable/VariableBus.h>

#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventsAssetRef.h>

namespace AZ
{
    class BehaviorEBus;
    class BehaviorEBusHandler;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            namespace Internal
            {
                struct ScriptEventEntry
                {
                    AZ_TYPE_INFO(ScriptEventEntry, "{28231E8C-6F56-4A28-A19A-2931D99FB1C9}");

                    bool IsExpectingResult() const
                    {
                        return m_resultSlotId.IsValid();
                    }

                    bool ContainsSlot(SlotId slotId) const
                    {
                        if (m_eventSlotId == slotId || m_resultSlotId == slotId)
                        {
                            return true;
                        }
                        else
                        {
                            return m_parameterSlotIds.end()
                                != AZStd::find_if(m_parameterSlotIds.begin(), m_parameterSlotIds.end(), [&slotId](const SlotId& candidate) { return slotId == candidate; });
                        }
                    }

                    AZ::Data::AssetId m_scriptEventAssetId;
                    AZStd::string m_eventName;
                    SlotId m_eventSlotId;
                    SlotId m_resultSlotId;
                    AZStd::vector<SlotId> m_parameterSlotIds;
                    int m_numExpectedArguments = {};
                    bool m_resultEvaluated = {};

                    bool m_shouldHandleEvent = false;
                    bool m_isHandlingEvent = false;

                    static void Reflect(AZ::ReflectContext* context);
                };

                //! Provides a base class for nodes that handle Script Events
                class ScriptEventBase
                    : public Node
                    , public AZ::Data::AssetBus::Handler
                    , private ScriptCanvas::ScriptEventNodeRequestBus::Handler
                {
                public:

                    SCRIPTCANVAS_NODE(ScriptEventBase);

                    using Events = AZStd::vector<ScriptEventEntry>;
                    using EventMap = AZStd::map<AZ::Crc32, ScriptEventEntry>;
                    using SlotIdMapping = AZStd::unordered_map<AZ::Uuid, ScriptCanvas::SlotId>;

                    AZ::u32 m_version;
                    EventMap m_eventMap;
                    SlotIdMapping m_eventSlotMapping;
                    AZ::Data::AssetId m_scriptEventAssetId;
                    AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> m_asset;

                    ScriptEventBase();
                    ~ScriptEventBase() override;

                    void OnInit() override;
                    void OnActivate() override;
                    void OnDeactivate() override;

                    AZ::Outcome<DependencyReport, void> GetDependencies() const override;
                    AZ::u32 GetVersion() const { return m_version; }
                    const EventMap& GetEvents() const { return m_eventMap; }
                    const AZ::Data::AssetId GetAssetId() const { return m_scriptEventAssetId; }
                    ScriptEvents::ScriptEventsAssetRef& GetAssetRef() { return m_scriptEventAsset; }
                    const ScriptEvents::ScriptEvent& GetScriptEvent() const { return m_definition; }
                    ScriptEvents::ScriptEventsAssetPtr GetAsset() const { return m_asset; }

                    AZStd::pair<AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>, bool> IsAssetOutOfDate() const;

                    virtual void Initialize(const AZ::Data::AssetId assetId);

                    

                protected:

                    // AZ::Data::AssetBus::Handler...
                    void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
                    void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

                    virtual void OnScriptEventReady(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&) {}
                    static void OnAssetChanged(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>&, void* userData);

                    AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration> m_scriptEvent;
                    ScriptEvents::ScriptEventsAssetRef m_scriptEventAsset;

                    ScriptEvents::ScriptEvent m_definition; // Don't serialize.

                    AZ::BehaviorEBus* m_ebus = nullptr;
                };

            }
        }
    }
}
