/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        ScriptCanvasTesting::PerformanceStressBusTraits::Reflect(context);
        ScriptCanvasTesting::NativeHandlingOnlyBusTraits::Reflect(context);
        ScriptCanvasTesting::TestTupleMethods::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EnumProperty<(AZ::u32)TestEnum::Alpha>("ALPHA")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
            behaviorContext->EnumProperty<(AZ::u32)TestEnum::Bravo>("BRAVO")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
            behaviorContext->EnumProperty<(AZ::u32)TestEnum::Charlie>("CHARLIE")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All);
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
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
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

    class PerformanceStressEBusHandler
        : public PerformanceStressEBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            PerformanceStressEBusHandler, "{EAE36675-F06B-4755-B3A5-CEC9495DC92E}", AZ::SystemAllocator
            , ForceStringCompare0
            , ForceStringCompare1
            , ForceStringCompare2
            , ForceStringCompare3
            , ForceStringCompare4
            , ForceStringCompare5
            , ForceStringCompare6
            , ForceStringCompare7
            , ForceStringCompare8
            , ForceStringCompare9
        );

        void ForceStringCompare0() override
        {
            Call(FN_ForceStringCompare0);
        }
        void ForceStringCompare1() override
        {
            Call(FN_ForceStringCompare1);
        }
        void ForceStringCompare2() override
        {
            Call(FN_ForceStringCompare2);
        }
        void ForceStringCompare3() override
        {
            Call(FN_ForceStringCompare3);
        }
        void ForceStringCompare4() override
        {
            Call(FN_ForceStringCompare4);
        }
        void ForceStringCompare5() override
        {
            Call(FN_ForceStringCompare5);
        }
        void ForceStringCompare6() override
        {
            Call(FN_ForceStringCompare6);
        }
        void ForceStringCompare7() override
        {
            Call(FN_ForceStringCompare7);
        }
        void ForceStringCompare8() override
        {
            Call(FN_ForceStringCompare8);
        }
        void ForceStringCompare9() override
        {
            Call(FN_ForceStringCompare9);
        }
    };

    void PerformanceStressBusTraits::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PerformanceStressEBus>("PerformanceStressEBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Handler<PerformanceStressEBusHandler>()
                ->Event("ForceStringCompare0", &PerformanceStressEBus::Events::ForceStringCompare0)
                ->Event("ForceStringCompare1", &PerformanceStressEBus::Events::ForceStringCompare1)
                ->Event("ForceStringCompare2", &PerformanceStressEBus::Events::ForceStringCompare2)
                ->Event("ForceStringCompare3", &PerformanceStressEBus::Events::ForceStringCompare3)
                ->Event("ForceStringCompare4", &PerformanceStressEBus::Events::ForceStringCompare4)
                ->Event("ForceStringCompare5", &PerformanceStressEBus::Events::ForceStringCompare5)
                ->Event("ForceStringCompare6", &PerformanceStressEBus::Events::ForceStringCompare6)
                ->Event("ForceStringCompare7", &PerformanceStressEBus::Events::ForceStringCompare7)
                ->Event("ForceStringCompare8", &PerformanceStressEBus::Events::ForceStringCompare8)
                ->Event("ForceStringCompare9", &PerformanceStressEBus::Events::ForceStringCompare9)
                ;
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

        void Void(AZStd::string_view value) override
        {
            Call(FN_Void, value);
        }
    };

    void LocalBusTraits::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<LocalEBus>("LocalEBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
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
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
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
