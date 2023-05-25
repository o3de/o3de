/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/set.h>
#include <ScriptEvents/Internal/BehaviorContextBinding/ScriptEventsBindingBus.h>

namespace ScriptEvents
{
    class ScriptEventsHandler;

    class ScriptEventBinding
        : Internal::BindingRequestBus::Handler
    {
    public:

        AZ_TYPE_INFO(ScriptEventBinding, "{E0DDA446-656D-41D6-8BEC-42B6EA57DD7D}");
        AZ_CLASS_ALLOCATOR(ScriptEventBinding, AZ::SystemAllocator);

        ScriptEventBinding(AZ::BehaviorContext* context, AZStd::string_view scriptEventName, const AZ::Uuid& addressType);

        AZStd::string_view GetScriptEventName() const { return m_scriptEventName; }

        virtual ~ScriptEventBinding();

    protected:

        // Internal::BindingRequestBus
        void Bind(const BindingParameters&) override;
        void Connect(const AZ::BehaviorArgument* address, ScriptEventsHandler* handler) override;
        void Disconnect(const AZ::BehaviorArgument* address, ScriptEventsHandler* handler) override;
        void RemoveHandler(ScriptEventsHandler* handler) override;
        ////

        // Behavior classes have a hash identifier that we will use to bind script events
        size_t GetAddressHash(const AZ::BehaviorArgument* address);

        // The equality operator method for the script event's address type (type must provide this operator to be used as script event address)
        AZ::BehaviorMethod* m_equalityOperatorMethod;

        // Script Events without a specified address will be broadcast to
        using EventSet = AZStd::set<ScriptEventsHandler*>;
        EventSet m_broadcasts;

        using EventBindingEntry = AZStd::pair<size_t, AZStd::set<ScriptEventsHandler*>>;
        using EventMap = AZStd::unordered_map<size_t, AZStd::set<ScriptEventsHandler*>>;
        EventMap m_events;

        AZStd::string_view m_scriptEventName;

        AZ::BehaviorContext* m_context;

        AZ::Uuid m_busBindingAddress;
    };

}
