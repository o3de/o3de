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
        AZ_CLASS_ALLOCATOR(ScriptEventsHandler, AZ::SystemAllocator, 0);
        virtual AZ::BehaviorValueParameter* GetBusId() = 0;

        bool IsConnected() override
        {
            return false;
        }

        bool IsConnectedId(AZ::BehaviorValueParameter*) override
        {
            return false;
        }

    };

    class DefaultBehaviorHandler : public ScriptEventsHandler
    {
    public:

        AZ_RTTI(DefaultBehaviorHandler, "{0AB58075-EE4F-49D7-83D4-E1250CC4471E}", ScriptEventsHandler);
        AZ_CLASS_ALLOCATOR(DefaultBehaviorHandler, AZ::SystemAllocator, 0);

        DefaultBehaviorHandler(AZ::BehaviorEBus* ebus, const ScriptEvents::ScriptEvent*);
        ~DefaultBehaviorHandler() override;

        //////////////////////////////////////////////////////////////////////////
        // EventsHandler
        AZ::BehaviorValueParameter* GetBusId() override { return &m_address; }

        //////////////////////////////////////////////////////////////////////////
        /// AZ::BehaviorEBusHandler
        int GetFunctionIndex(const char* name) const override;
        bool Connect(AZ::BehaviorValueParameter* address = nullptr) override;
        void Disconnect() override;

    private:
        AZ::BehaviorValueParameter m_address;

        AZ::Uuid m_busNameId;
        AZ::BehaviorEBus* m_ebus;
    };
}
