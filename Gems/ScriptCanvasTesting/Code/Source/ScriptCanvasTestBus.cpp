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

#include "ScriptCanvasTestBus.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Core/Attributes.h>

namespace ScriptCanvasTesting
{
    void Reflect(AZ::ReflectContext* context)
    {
        ScriptCanvasTesting::GlobalBusTraits::Reflect(context);
        ScriptCanvasTesting::LocalBusTraits::Reflect(context);
        ScriptCanvasTesting::NativeHandlingOnlyBusTraits::Reflect(context);
        ScriptCanvasTesting::TestTupleMethods::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EnumProperty<(AZ::u32)TestEnum::Alpha>("ALPHA");
            behaviorContext->EnumProperty<(AZ::u32)TestEnum::Bravo>("BRAVO");
            behaviorContext->EnumProperty<(AZ::u32)TestEnum::Charlie>("CHARLIE");
        }
    }

    class GlobalEBusHandler
        : public GlobalEBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            GlobalEBusHandler, "{CF167F12-0685-4347-A2DE-8D40186E7332}", AZ::SystemAllocator, AppendSweet, Increment, Not, Sum, Void,
            GetZeroParamEvent, GetByReferenceEvent, GetByValueEvent);

        AZStd::string AppendSweet(AZStd::string_view value) override
        {
            AZStd::string result;
            CallResult(result, FN_AppendSweet, value);
            return result;
        }

        int Increment(int value) override
        {
            int result;
            CallResult(result, FN_Increment, value);
            return result;
        }

        bool Not(bool value) override
        {
            bool result;
            CallResult(result, FN_Not, value);
            return result;
        }

        int Sum(int numberA, int numberB) override
        {
            int result(0);
            CallResult(result, FN_Sum, numberA, numberB);
            return result;
        }

        void Void(AZStd::string_view value) override
        {
            Call(FN_Void, value);
        }

        AZ::Event<>* GetZeroParamEvent() override
        {
            AZ::Event<>* azEvent = nullptr;
            CallResult(azEvent, FN_GetZeroParamEvent);
            return azEvent;
        }

        AZ::Event<AZStd::vector<AZStd::string>&>* GetByReferenceEvent() override
        {
            AZ::Event<AZStd::vector<AZStd::string>&>* azEvent = nullptr;
            CallResult(azEvent, FN_GetByReferenceEvent);
            return azEvent;
        }

        AZ::Event<int, bool, AZStd::string>* GetByValueEvent() override
        {
            AZ::Event<int, bool, AZStd::string>* azEvent = nullptr;
            CallResult(azEvent, FN_GetByValueEvent);
            return azEvent;
        }
    };

    void GlobalBusTraits::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            AZ::BehaviorAzEventDescription modObjectDesc, modValueDesc, modVoidDesc;

            modObjectDesc.m_eventName = "OnEvent-Reference";
            modObjectDesc.m_parameterNames = {"Object List"};

            modValueDesc.m_eventName = "OnEvent-Value";
            modValueDesc.m_parameterNames = {"A", "BB", "CCC"};

            modVoidDesc.m_eventName = "OnEvent-ZeroParam";

            behaviorContext->EBus<GlobalEBus>("GlobalEBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Handler<GlobalEBusHandler>()
                ->Event("AppendSweet", &GlobalEBus::Events::AppendSweet)
                ->Event("Increment", &GlobalEBus::Events::Increment)
                ->Event("Not", &GlobalEBus::Events::Not)
                ->Event("Sum", &GlobalEBus::Events::Sum)
                ->Event("Void", &GlobalEBus::Events::Void)
                ->Event("GetZeroParamEvent", &GlobalEBus::Events::GetZeroParamEvent)
                ->Attribute(AZ::Script::Attributes::AzEventDescription, modVoidDesc)
                ->Event("GetByReferenceEvent", &GlobalEBus::Events::GetByReferenceEvent)
                ->Attribute(AZ::Script::Attributes::AzEventDescription, modObjectDesc)
                ->Event("GetByValueEvent", &GlobalEBus::Events::GetByValueEvent)
                ->Attribute(AZ::Script::Attributes::AzEventDescription, modValueDesc);
        }
    }

    class LocalEBusHandler
        : public LocalEBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            LocalEBusHandler, "{308650EE-061D-4090-A7FB-471885C8B6A5}", AZ::SystemAllocator, AppendSweet, Increment, Not, Void);

        AZStd::string AppendSweet(AZStd::string_view value) override
        {
            AZStd::string result;
            CallResult(result, FN_AppendSweet, value);
            return result;
        }

        int Increment(int value) override
        {
            int result;
            CallResult(result, FN_Increment, value);
            return result;
        }

        bool Not(bool value) override
        {
            bool result;
            CallResult(result, FN_Not, value);
            return result;
        }

        void Void(AZStd::string_view value)
        {
            Call(FN_Void, value);
        }
    };

    void LocalBusTraits::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LocalEBus>("LocalEBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Handler<LocalEBusHandler>()
                ->Event("AppendSweet", &LocalEBus::Events::AppendSweet)
                ->Event("Increment", &LocalEBus::Events::Increment)
                ->Event("Not", &LocalEBus::Events::Not)
                ->Event("Void", &LocalEBus::Events::Void);
        }
    }

    void NativeHandlingOnlyBusTraits::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<NativeHandlingOnlyEBus>("NativeHandlingOnlyEBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("AppendSweet", &NativeHandlingOnlyEBus::Events::AppendSweet)
                ->Event("Increment", &NativeHandlingOnlyEBus::Events::Increment)
                ->Event("Not", &NativeHandlingOnlyEBus::Events::Not)
                ->Event("TwistTypeEntityId", &NativeHandlingOnlyEBus::Events::TwistTypeEntityId)
                ->Event("TwistTypeVector3", &NativeHandlingOnlyEBus::Events::TwistTypeVector3)
                ->Event("TwistTupleEntityId", &NativeHandlingOnlyEBus::Events::TwistTupleEntityId)
                ->Event("TwistTupleVector3", &NativeHandlingOnlyEBus::Events::TwistTupleVector3)
                ->Event("Void", &NativeHandlingOnlyEBus::Events::Void);
        }
    }
}
