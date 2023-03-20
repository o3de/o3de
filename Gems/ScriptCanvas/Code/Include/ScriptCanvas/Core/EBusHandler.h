/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "Nodeable.h"

AZ_DECLARE_BUDGET(ScriptCanvas);

struct lua_State;

namespace AZ
{
    class BehaviorContext;
    class SerializeContext;

    struct BehaviorArgument;
}

namespace ScriptCanvas
{
    class EBusHandler final
        : public Nodeable
    {
    public:
        AZ_RTTI(EBusHandler, "{38E3448F-1876-41DF-A26F-EF873AF5EE14}", Nodeable);
        AZ_CLASS_ALLOCATOR(EBusHandler, AZ::SystemAllocator);      
        
        static EBusHandler* Create(ExecutionStateWeakPtr executionState, AZStd::string_view busName);
        
        static void Reflect(AZ::ReflectContext* reflectContext);

        EBusHandler(ExecutionStateWeakPtr executionState, AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);

        EBusHandler(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);

        ~EBusHandler() override;
                
        bool Connect();

        bool ConnectTo(AZ::BehaviorArgument& busId);
        
        void Disconnect();
        
        const AZStd::string& GetEBusName() const;
        
        int GetEventIndex(AZStd::string_view eventName) const;
                
        bool IsActive() const override;

        bool IsConnected() const;
        
        bool IsConnectedTo(AZ::BehaviorArgument& busId) const;

        void HandleEvent(int eventIndex);

    protected:
        void OnDeactivate() override;

    private:
        static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorArgument* result, int numParameters, AZ::BehaviorArgument* parameters);

        AZ::BehaviorEBusHandler* m_handler = nullptr;
        AZ::BehaviorEBus* m_ebus = nullptr;
        
        EBusHandler() = default;

        void CreateHandler(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);
        
        void InitializeEBusHandling(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);

        void OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorArgument* result, const int numParameters, AZ::BehaviorArgument* parameters);
    };
}
