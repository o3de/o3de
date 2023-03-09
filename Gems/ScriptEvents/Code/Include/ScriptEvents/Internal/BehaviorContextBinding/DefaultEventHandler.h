/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptEvents
{
    class ScriptEvent;

    class ScriptEventsHandler
        : public AZ::BehaviorEBusHandler
    {
    public:

        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        AZ_RTTI(ScriptEventsHandler, "{4272AECB-F1A7-4B22-8537-2EE6492EC132}", AZ::BehaviorEBusHandler);
        AZ_CLASS_ALLOCATOR(ScriptEventsHandler, AZ::SystemAllocator);
        virtual AZ::BehaviorArgument* GetBusId() = 0;

        bool IsConnected() override
        {
            return false;
        }

        bool IsConnectedId(AZ::BehaviorArgument*) override
        {
            return false;
        }

    };

    class DefaultBehaviorHandler : public ScriptEventsHandler
    {
    public:

        AZ_RTTI(DefaultBehaviorHandler, "{0AB58075-EE4F-49D7-83D4-E1250CC4471E}", ScriptEventsHandler);
        AZ_CLASS_ALLOCATOR(DefaultBehaviorHandler, AZ::SystemAllocator);

        DefaultBehaviorHandler(AZ::BehaviorEBus* ebus, const ScriptEvents::ScriptEvent*);
        ~DefaultBehaviorHandler() override;

        //////////////////////////////////////////////////////////////////////////
        // EventsHandler
        AZ::BehaviorArgument* GetBusId() override { return &m_address; }

        //////////////////////////////////////////////////////////////////////////
        /// AZ::BehaviorEBusHandler
        int GetFunctionIndex(const char* name) const override;
        bool Connect(AZ::BehaviorArgument* address = nullptr) override;
        void Disconnect(AZ::BehaviorArgument* address = nullptr) override;

    private:
        AZ::BehaviorArgument m_address;

        AZ::Uuid m_busNameId;
        AZ::BehaviorEBus* m_ebus;
    };
}
