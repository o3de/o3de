/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>

#include <ScriptCanvas/Data/Data.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasTesting
{
    void Reflect(AZ::ReflectContext* context);

    class GlobalBusTraits : public AZ::EBusTraits
    {
    public:
        AZ_TYPE_INFO(GlobalBusTraits, "{DED849D7-CF17-408B-8D87-E31FC7D3CEC4}");

        static void Reflect(AZ::ReflectContext* context);

        virtual AZStd::string AppendSweet(AZStd::string_view) = 0;
        virtual int Increment(int value) = 0;
        virtual bool Not(bool value) = 0;
        virtual int Sum(int numberA, int numberB) = 0;
        virtual void Void(AZStd::string_view) = 0;

        virtual AZ::Event<>* GetZeroParamEvent() = 0;
        virtual AZ::Event<AZStd::vector<AZStd::string>&>* GetByReferenceEvent() = 0;
        virtual AZ::Event<int, bool, AZStd::string>* GetByValueEvent() = 0;
    };
    using GlobalEBus = AZ::EBus<GlobalBusTraits>;

    class PerformanceStressBusTraits : public AZ::EBusTraits
    {
    public:
        AZ_TYPE_INFO(PerformanceStressBusTraits, "{68AF0B81-70F4-4822-8127-AAC442D924C7}");

        static void Reflect(AZ::ReflectContext* context);

        virtual void ForceStringCompare0() = 0;
        virtual void ForceStringCompare1() = 0;
        virtual void ForceStringCompare2() = 0;
        virtual void ForceStringCompare3() = 0;
        virtual void ForceStringCompare4() = 0;
        virtual void ForceStringCompare5() = 0;
        virtual void ForceStringCompare6() = 0;
        virtual void ForceStringCompare7() = 0;
        virtual void ForceStringCompare8() = 0;
        virtual void ForceStringCompare9() = 0;
    };
    using PerformanceStressEBus = AZ::EBus<PerformanceStressBusTraits>;


    class LocalBusTraits : public AZ::EBusTraits
    {
    public:
        AZ_TYPE_INFO(LocalBusTraits, "{749B6949-CBBB-44D9-A57D-9973DC88E639}");

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        using BusIdType = double;

        static void Reflect(AZ::ReflectContext* context);

        virtual AZStd::string AppendSweet(AZStd::string_view) = 0;
        virtual int Increment(int value) = 0;
        virtual bool Not(bool value) = 0;
        virtual void Void(AZStd::string_view) = 0;
    };
    using LocalEBus = AZ::EBus<LocalBusTraits>;

    class NativeHandlingOnlyBusTraits : public AZ::EBusTraits
    {
    public:
        AZ_TYPE_INFO(NativeHandlingOnlyBusTraits, "{5AED48A7-3E16-41F6-B1C0-4E1FBBA84F83}");

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        using BusIdType = AZ::EntityId;

        static void Reflect(AZ::ReflectContext* context);

        virtual AZStd::string AppendSweet(AZStd::string_view) = 0;
        virtual int Increment(int value) = 0;
        virtual AZ::EntityId TwistTypeEntityId() = 0;
        virtual AZ::Vector3 TwistTypeVector3() = 0;
        virtual AZStd::tuple<AZ::EntityId, bool> TwistTupleEntityId() = 0;
        virtual AZStd::tuple<AZ::Vector3, bool> TwistTupleVector3() = 0;
        virtual bool Not(bool value) = 0;
        virtual void Void(AZStd::string_view) = 0;
    };
    using NativeHandlingOnlyEBus = AZ::EBus<NativeHandlingOnlyBusTraits>;

    class TestTupleMethods
    {
    public:
        AZ_TYPE_INFO(TestTupleMethods, "{E794CE93-7AC6-476E-BF10-B107A2E4D807}");

        static AZStd::tuple<ScriptCanvas::Data::Vector3Type, ScriptCanvas::Data::StringType, ScriptCanvas::Data::BooleanType> Three(
            const ScriptCanvas::Data::Vector3Type& v, const ScriptCanvas::Data::StringType& s, const ScriptCanvas::Data::BooleanType& b)
        {
            return AZStd::make_tuple(v, s, b);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<TestTupleMethods>("TestTupleMethods")
                    ->Attribute(AZ::Script::Attributes::Category, "Tests")
                    ->Method("Three", &TestTupleMethods::Three);

                behaviorContext->Method("ScriptCanvasTesting_TestTupleMethods_GlobalThree", &TestTupleMethods::Three)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Tests");

                auto GlobalThreeSameType = [](const ScriptCanvas::Data::StringType& s1, const ScriptCanvas::Data::StringType& s2,
                                              const ScriptCanvas::Data::StringType& s3)
                    -> AZStd::tuple<ScriptCanvas::Data::StringType, ScriptCanvas::Data::StringType, ScriptCanvas::Data::StringType>
                {
                    return AZStd::make_tuple(s1, s2, s3);
                };
                behaviorContext->Method("ScriptCanvasTesting_TestTupleMethods_GlobalThreeSameType", GlobalThreeSameType)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Tests");
            }
        }
    };

    class TestGlobalMethods
    {
    public:

        static void CanNotAcceptNull(AZStd::vector<AZStd::string>& strings)
        {
            AZ_TracePrintf("ScriptCanvas", "Used for testing parse errors");
            strings.push_back("Cannot accept null input");
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Method("ScriptCanvasTesting_TestGlobalMethods_CanNotAcceptNull", &CanNotAcceptNull)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Tests")
                    ;

                auto IsPositive = [](int input) -> bool
                {
                    return input > 0;
                };

                behaviorContext->Method("ScriptCanvasTesting_TestGlobalMethods_IsPositive", IsPositive)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Tests");

                auto DivideByPreCheck = [](int input) -> int
                {
                    return 10 / input;
                };

                AZ::CheckedOperationInfo checkedInfo{ "ScriptCanvasTesting_TestGlobalMethods_IsPositive", {}, "Out", "Invalid Input" };
                behaviorContext->Method("ScriptCanvasTesting_TestGlobalMethods_DivideByPreCheck", DivideByPreCheck)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Tests")
                    ->Attribute(AZ::ScriptCanvasAttributes::CheckedOperation, checkedInfo);

                auto SumPostCheck = [](int input1, int input2) -> int
                {
                    return input1 + input2;
                };

                AZ::BranchOnResultInfo branchResult;
                branchResult.m_trueName = "Out";
                branchResult.m_falseName = "Not Positive";
                branchResult.m_nonBooleanResultCheckName = "ScriptCanvasTesting_TestGlobalMethods_IsPositive";
                branchResult.m_returnResultInBranches = true;
                behaviorContext->Method("ScriptCanvasTesting_TestGlobalMethods_SumPostCheck", SumPostCheck)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Tests")
                    ->Attribute(AZ::ScriptCanvasAttributes::BranchOnResult, branchResult);
            }
        }
    };


    enum class TestEnum : AZ::u32
    {
        Alpha = 7,
        Bravo = 15,
        Charlie = 31,
    };
}
