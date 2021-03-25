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

#include "Nodeable.h"

struct lua_State;

namespace AZ
{
    class BehaviorContext;
    class SerializeContext;

    struct BehaviorValueParameter;
}

namespace ScriptCanvas
{
    class EBusHandler final
        : public Nodeable
    {
    public:
        AZ_RTTI(EBusHandler, "{38E3448F-1876-41DF-A26F-EF873AF5EE14}", Nodeable);
        AZ_CLASS_ALLOCATOR(EBusHandler, AZ::SystemAllocator, 0);      
        
        static EBusHandler* Create(ExecutionStateWeakPtr executionState, AZStd::string_view busName);
        
        static void Reflect(AZ::ReflectContext* reflectContext);

        EBusHandler(ExecutionStateWeakPtr executionState, AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);

        EBusHandler(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);

        ~EBusHandler() override;
                
        bool Connect();

        bool ConnectTo(AZ::BehaviorValueParameter& busId);
        
        void Disconnect();
        
        const AZStd::string& GetEBusName() const;
        
        int GetEventIndex(AZStd::string_view eventName) const;
                
        bool IsActive() const override;

        bool IsConnected() const;
        
        bool IsConnectedTo(AZ::BehaviorValueParameter& busId) const;

        void HandleEvent(int eventIndex);

    protected:
        void OnDeactivate() override;

    private:
        static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters);

        AZ::BehaviorEBusHandler* m_handler = nullptr;
        AZ::BehaviorEBus* m_ebus = nullptr;
        
        EBusHandler() = default;

        void CreateHandler(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);
        
        void InitializeEBusHandling(AZStd::string_view busName, AZ::BehaviorContext* behaviorContext);

        void OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters);
    };
}
