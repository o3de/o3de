/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/Script/ScriptContextDebug.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Component/Entity.h>

#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/fixed_vector.h>

#include <math.h> // for pow

#include <AzCore/Math/MathReflection.h>
#include <AzCore/Math/Transform.h>

#include <AzCore/std/any.h>

// Components
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>

#include <Tests/BehaviorContextFixture.h>

namespace UnitTest
{
    using namespace AZ;

    enum GlobalEnum
    {
        GE_VALUE1 = 0,
        GE_VALUE2,
    };

    enum class GlobalClassEnum
    {
        Value1,
        Value2,
        Value3,
    };

    struct GlobalData
    {
        AZ_TYPE_INFO(GlobalData, "{4F35A5E6-568E-43C8-851A-4D2315E9BAD0}");

        GlobalData()
        {}

        char data[512];
    };


    int g_globalValue = 501;
    int g_globalData = 0;
    int g_globalTestClassesConstructed = 0;
    int g_globalTestClassesDestructed = 0;

    GlobalClassEnum globalClassEnumValue = GlobalClassEnum::Value3;

    int globalPropertyGetter()
    {
        return g_globalValue;
    }

    void globalPropertySetter(int value)
    {
        g_globalValue = value;
    }

    int globalMethod(int a)
    {
        return a + 3;
    }

    void globalMethod1()
    {
        g_globalData = 1001;
    }

    int globalMethod2WithDefaultArgument(int value)
    {
        return value + 1;
    }

    int globalMethodContainers(const AZStd::vector<int>& value)
    {
        return value[0];
    }

    int globalMethodPair(const AZStd::pair<int, bool>& value)
    {
        return value.second ? value.first : -1;
    }

    int globalMethodToOverride(int a)
    {
        return a * 3;
    }

    void globalMethodOverride(ScriptDataContext& dc)
    {
        (void)dc;
    }

    GlobalData globalMethodLarge()
    {
        return GlobalData();
    }

    GlobalClassEnum globalMethodGetClassEnum()
    {
        return GlobalClassEnum::Value1;
    }

    void globalMethodSetClassEnum(GlobalClassEnum value)
    {
        globalClassEnumValue = value;
    }

    // Global Enum class wrapper, till the "namespace" open/close functionallity is missing
    struct BehaviorGlobalClassEnumWrapper
    {
        AZ_TYPE_INFO(BehaviorGlobalClassEnumWrapper,"{de36dcf3-7c25-4053-b747-1148240cda68}");
    };

    class BehaviorTestClass
    {
    public:
        AZ_RTTI(BehaviorTestClass, "{26398112-E4F1-4912-80E6-D81152116D02}");
        AZ_CLASS_ALLOCATOR(BehaviorTestClass, AZ::SystemAllocator);

        BehaviorTestClass()
            : m_data(1)
            , m_data1(2)
            , m_dataReadOnly(3)
        {
            g_globalData = 1020;
            g_globalTestClassesConstructed++;
        }

        BehaviorTestClass(const BehaviorTestClass& other)
            : m_data(other.m_data)
            , m_data1(other.m_data1)
            , m_dataReadOnly(other.m_dataReadOnly)
        {
            g_globalData = 1025;
            g_globalTestClassesConstructed++;
        }

        BehaviorTestClass(int data)
            : m_data(data)
            , m_data1(3)
            , m_dataReadOnly(4)
        {
            g_globalData = 1030;
            g_globalTestClassesConstructed++;
        }

        virtual ~BehaviorTestClass()
        {
            g_globalData = 1040;
            g_globalTestClassesDestructed++;
        }

        enum MyEnum
        {
            ME_VALUE0,
            ME_VALUE1,
        };

        enum class MyClassEnum
        {
            MCE_VALUE0,
            MCE_VALUE1,
        };

        void Method1()
        {
            g_globalData = 1050;
        }

        int Method2(int* a) const
        {
            return *a + 1;
        }

        BehaviorTestClass* GetClass()
        {
            return this;
        }

        int GetData() const
        {
            return m_data;
        }
        void SetData(int data)
        {
            m_data = data;
            m_data1 = m_data + 10;
            m_dataReadOnly = m_data1 + 100;
        }

        static void StaticMethod(int newData)
        {
            g_globalData = newData;
        }

        void SpecialScriptMethod(ScriptDataContext& dc)
        {
            dc.PushResult(303);
        }

        void MethodToOverrideInScript()
        {
            g_globalData = 1060;
        }

        void Method1ScriptOverride(ScriptDataContext&)
        {
            g_globalData = 1070;
        }

        void MethodToOverrideInScriptWithANonMember() const
        {
            g_globalData = 1080;
        }

        BehaviorTestClass AddOperator(BehaviorTestClass& rhs)
        {
            BehaviorTestClass result = *this;
            result.m_data += rhs.m_data;
            result.m_data1 += rhs.m_data1;
            result.m_dataReadOnly += rhs.m_dataReadOnly;
            return result;
        }

        AZStd::string ToString()
        {
            return AZStd::string::format("%d\n", m_data);
        }

        bool BoundsCheckMethodWithDefaultValue(float value, float epsilon, float minBounds, float maxBounds)
        {
            (void)epsilon;
            return value >= minBounds && value < maxBounds;
        }

        int m_data;
        int m_data1;
        int m_dataReadOnly;
    };

    class BehaviorDerivedTestClass : public BehaviorTestClass
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorDerivedTestClass, AZ::SystemAllocator)
        AZ_RTTI(BehaviorDerivedTestClass, "{dba8a4e3-8fab-4223-94a6-874c6cff88e5}", BehaviorTestClass);
        int m_data;

    };

    void TestMethodToScriptOverride(const BehaviorTestClass* thisPtr, ScriptDataContext&)
    {
        (void)thisPtr;
        g_globalData = 1010;
    }

    void TestScriptNonIntrusiveConstructor(BehaviorTestClass* thisPtr, ScriptDataContext& dc)
    {
        new(thisPtr) BehaviorTestClass();
        if (dc.GetNumArguments() && dc.IsNumber(0))
        {
            int data = 0;
            dc.ReadArg(0, data);
            thisPtr->m_data = data;
            thisPtr->m_data1 = data * 2;
            thisPtr->m_dataReadOnly = data * 3;
        }
        else
        {
            thisPtr->m_data = 1000;
            thisPtr->m_data1 = 1005;
            thisPtr->m_dataReadOnly = 1007;
        }
    }

    class BehaviorTestBusEvents : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Configure the EBus
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        using BusIdType = int;
        static const bool EnableEventQueue = true;
        //////////////////////////////////////////////////////////////////////////

        virtual void OnEvent(int data) = 0;

        virtual int OnEventWithResult(int data)
        {
            return data + 2;
        }

        virtual int OnEventWithResultContainer(const AZStd::vector<int> values) = 0;

        virtual GlobalClassEnum OnEventWithClassEnumResult() = 0;

        virtual AZStd::string OnEventWithStringResult() { return ""; }

        virtual BehaviorTestClass OnEventWithClassResult() { return BehaviorTestClass(); }

        virtual AZStd::string OnEventWithDefaultValueAndStringResult(AZStd::string_view view1, AZStd::string_view) { return AZStd::string::format("Default Value: %s", view1.data()); }

        virtual void OnEventConst() const {};

        virtual BehaviorTestClass OnEventResultWithBehaviorClassParameter(BehaviorTestClass data) { return BehaviorTestClass(); };
    };

    typedef AZ::EBus<BehaviorTestBusEvents> BehaviorTestBus;


    class BehaviorTestBusHandler : public BehaviorTestBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorTestBusHandler, "{19F5C8C8-4260-46B1-B624-997CD3F10CBD}", AZ::SystemAllocator
            , OnEvent
            , OnEventWithResult
            , OnEventWithResultContainer
            , OnEventWithClassEnumResult
            , OnEventWithStringResult
            , OnEventWithClassResult
            , OnEventWithDefaultValueAndStringResult
            , OnEventResultWithBehaviorClassParameter
        );

        // User code
        void OnEvent(int data) override
        {
            // you can get the index yourself or use the FN_xxx enum FN_OnEvent
            static int eventIndex = GetFunctionIndex("OnEvent");
            AZ_Assert(eventIndex != -1, "We can't find event with name %s", "OnEvent");
            Call(eventIndex, data);
        }

        int OnEventWithResult(int data) override
        {
            int result = 0; // default result as a function hook might not exists
            CallResult(result, FN_OnEventWithResult, data);
            return result;
        }

        int OnEventWithResultContainer(const AZStd::vector<int> values) override
        {
            int result = 0;
            CallResult(result, FN_OnEventWithResultContainer, values);
            return result;
        }

        GlobalClassEnum OnEventWithClassEnumResult() override
        {
            GlobalClassEnum result = GlobalClassEnum::Value1;
            CallResult(result, FN_OnEventWithClassEnumResult);
            return result;
        }

        AZStd::string OnEventWithStringResult() override
        {
            AZStd::string result;
            CallResult(result, FN_OnEventWithStringResult);
            return result;
        }

        BehaviorTestClass OnEventWithClassResult() override
        {
            BehaviorTestClass result;
            CallResult(result, FN_OnEventWithClassResult);
            return result;
        }

        AZStd::string OnEventWithDefaultValueAndStringResult(AZStd::string_view view1, AZStd::string_view view2) override
        {
            return BehaviorTestBus::Handler::OnEventWithDefaultValueAndStringResult(view1, view2);
        }


        BehaviorTestClass OnEventResultWithBehaviorClassParameter(BehaviorTestClass data) override
        {
            BehaviorTestClass result = data;
            return result;
        }
    };

    class BehaviorTestBusHandlerWithDoc : public BehaviorTestBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER_WITH_DOC(BehaviorTestBusHandlerWithDoc, "{C0D4FE98-DBE1-439A-ACBA-B3767863C560}", AZ::SystemAllocator
            , OnEvent, ({"data", "Data to pass in"})
            , OnEventWithResult, ({"data", "Data to pass in"})
            , OnEventWithResultContainer, ({ "values", "Vector of integers to forward" })
            , OnEventWithClassEnumResult, ()
            , OnEventWithStringResult, ()
            , OnEventWithClassResult, ()
            , OnEventWithDefaultValueAndStringResult, ({ "defaultView", "string_view which contains literal to print by default" }, { "unusedView", "Unused test parameter" })
            , OnEventResultWithBehaviorClassParameter, ()
        );

        // User code
        void OnEvent(int data) override
        {
            // you can get the index yourself or use the FN_xxx enum FN_OnEvent
            static int eventIndex = GetFunctionIndex("OnEvent");
            AZ_Assert(eventIndex != -1, "We can't find event with name %s", "OnEvent");
            Call(eventIndex, data);
        }

        int OnEventWithResult(int data) override
        {
            int result = 0; // default result as a function hook might not exists
            CallResult(result, FN_OnEventWithResult, data);
            return result;
        }

        int OnEventWithResultContainer(const AZStd::vector<int> values) override
        {
            int result = 0;
            CallResult(result, FN_OnEventWithResultContainer, values);
            return result;
        }

        GlobalClassEnum OnEventWithClassEnumResult() override
        {
            GlobalClassEnum result = GlobalClassEnum::Value1;
            CallResult(result, FN_OnEventWithClassEnumResult);
            return result;
        }

        AZStd::string OnEventWithStringResult() override
        {
            AZStd::string result;
            CallResult(result, FN_OnEventWithStringResult);
            return result;
        }

        BehaviorTestClass OnEventWithClassResult() override
        {
            BehaviorTestClass result;
            CallResult(result, FN_OnEventWithClassResult);
            return result;
        }

        AZStd::string OnEventWithDefaultValueAndStringResult(AZStd::string_view view1, AZStd::string_view view2) override
        {
            return BehaviorTestBus::Handler::OnEventWithDefaultValueAndStringResult(view1, view2);
        }


        BehaviorTestClass OnEventResultWithBehaviorClassParameter(BehaviorTestClass data) override
        {
            BehaviorTestClass result = data;
            return result;
        }
    };

    // traditional EBus Handler
    class NativeBehaviorTestBusHandler : public BehaviorTestBus::Handler
    {
    public:
        NativeBehaviorTestBusHandler()
        {
            m_enumResult = GlobalClassEnum::Value2;
            BehaviorTestBus::Handler::BusConnect(1);
        }

        void OnEvent(int data) override
        {
            g_globalData = 2000 + data;
        }

        int OnEventWithResult(int data) override
        {
            g_globalData = 2100 + data;
            return data + 3;
        }

        int OnEventWithResultContainer(const AZStd::vector<int> values) override
        {
            g_globalData = 2100 + values[0];
            return values[0];
        }

        GlobalClassEnum OnEventWithClassEnumResult() override
        {
            return m_enumResult;
        }

        GlobalClassEnum m_enumResult;
    };

    /// Hooks
    void OnEventHook(void* userData, int data)
    {
        (void)userData;
        g_globalData = 2200 + data;
    }

    int OnEventWithResultHook(void* userData, int data)
    {
        (void)userData;
        g_globalData = 2300 + data;
        return data + 30;
    }

    void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, BehaviorArgument* result, int numParameters, BehaviorArgument* parameters)
    {
        (void)userData; (void)numParameters; (void)result; (void)eventName; (void)eventIndex;
        AZ_Assert(result == nullptr || strstr(eventName, "OnEventWithResult"), "We don't exepct result here");
        AZ_Assert(numParameters == 1, "Invalid number of parameters!");
        AZ_Assert(parameters[0].ConvertTo<int>(), "The parameter should be int, and convertable to it");
        int* intData = parameters[0].GetAsUnsafe<int>();
        g_globalData = 2400 + *intData;
        if (result)
        {
            result->StoreResult(*intData + 50);
        }
    }

    void globalGenericMethod(ScriptDataContext& dc)
    {
        if (dc.GetNumArguments())
        {
            if (dc.IsNumber(0))
            {
                int data;
                dc.ReadArg(0, data);
                dc.PushResult(data + 5);
            }
        }
    }

    class BehaviorTestSmartPtr
    {
    public:
        AZ_CLASS_ALLOCATOR(BehaviorTestSmartPtr, AZ::SystemAllocator);
        AZ_TYPE_INFO(BehaviorTestSmartPtr, "{b59d3416-a77c-4e71-b638-4212705a07e6}");

        BehaviorTestSmartPtr()
            : m_intrusiveRefCount(0)
            , m_data(303)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        // instrusive_ptr
        // TODO: Add intrusive pointers helpers (RefCounted<T> and RefCountedThreadSave<T> or as a policy RefCounter<T, ThreadSafe>
        void add_ref()
        {
            ++m_intrusiveRefCount;
        }
        void release()
        {
            if (--m_intrusiveRefCount == 0)
            {
                delete this;
            }
        }
        int m_intrusiveRefCount;
        //////////////////////////////////////////////////////////////////////////

        void TestFunction()
        {
            g_globalData = 1090;
        }

        int m_data;
    };

    void IncrementBehaviorTestSmartPtrSharedPtr(AZStd::shared_ptr<BehaviorTestSmartPtr>& sp)
    {
        if (sp)
        {
            ++sp->m_data;
        }
    };

    void IncrementBehaviorTestSmartPtrIntrusivePtr(AZStd::intrusive_ptr<BehaviorTestSmartPtr>& sp)
    {
        if (sp)
        {
            ++sp->m_data;
        }
    }

    void PointerIsNullptr(BehaviorTestSmartPtr* value)
    {
        AZ_TEST_ASSERT(value == nullptr);
    }

    void BehaviorTestTemplatedOnDemandReflection(const AZStd::vector<int>& values)
    {
        g_globalData = 2500 + static_cast<int>(values.size());
    }

    int BehaviorGlobalMethodAfterBind()
    {
        return 3030;
    }

    int BehaviorGlobalPropertyGetAfterBind()
    {
        return g_globalData + 101;
    }

    void BehaviorGlobalPropertySetAfterBind(int data)
    {
        g_globalData = data;
    }

    struct BehaviorClassAfterBind
    {
        AZ_CLASS_ALLOCATOR(BehaviorClassAfterBind, AZ::SystemAllocator);
        AZ_TYPE_INFO(BehaviorClassAfterBind, "{72de521d-5880-43b7-93e9-ab06a3770946}");
        int m_data;
    };

    class ClassRequestBusEvents :  public AZ::EBusTraits
    {
    public:
        virtual int GetData() = 0;
        virtual void SetData(int data) = 0;
    };

    using ClassRequestEBus = EBus<ClassRequestBusEvents>;

    class ClassInteractingWithEBus :  public ClassRequestEBus::Handler
    {
    public:
        AZ_TYPE_INFO(ClassInteractingWithEBus, "{28d9591f-d233-442f-af62-81096b64d703}");

        ClassInteractingWithEBus()
            : m_data(0)
        {
            BusConnect();
        }

        int GetData() override { return m_data; }
        void SetData(int data) override { m_data = data; }

        int m_data;
    };

    static void ScriptErrorAssertCB(AZ::ScriptContext*, AZ::ScriptContext::ErrorType, const char*)
    {
        AZ_TEST_ASSERT(false);
    }

    class BehaviorContextTest
        : public BehaviorContextFixture
    {
    public:

        void SetUp() override
        {
            BehaviorContextFixture::SetUp();
            ResetGlobalVars();
        }

        void ResetGlobalVars()
        {
            g_globalValue = 501;
            g_globalData = 0;
            globalClassEnumValue = GlobalClassEnum::Value3;
        }

        void run()
        {
            // Constants/Enum
            m_behaviorContext->Constant("globalConstant", []() { return 3.14f; });
            m_behaviorContext->Constant("globalConstantMacro", BehaviorConstant(3.14f));

            m_behaviorContext->Enum<(int)GE_VALUE1>("GE_VALUE1");

            m_behaviorContext->Enum<(int)GlobalClassEnum::Value1>("Value1");
            m_behaviorContext->Enum<(int)GlobalClassEnum::Value2>("Value2");

            // Property
            m_behaviorContext->Property("globalProperty", &globalPropertyGetter, &globalPropertySetter)
                    ->Attribute("GlobalPropAttr", 1);
            m_behaviorContext->Property("globalPropertyLambda", []() { return g_globalValue; }, [](int v) { g_globalValue = v; });
            m_behaviorContext->Property("globalPropertyMacro", BehaviorValueProperty(&g_globalValue));
            m_behaviorContext->Property("globalPropertyReadOnly", BehaviorValueGetter(&g_globalValue), nullptr); // read only property
            // Property by address???

            m_behaviorContext->Class<GlobalData>();

            // Method
            const int defaultIntValue = 20;
            m_behaviorContext->Method("globalMethod", &globalMethod, m_behaviorContext->MakeDefaultValues(555))
                    ->Attribute("GlobalMethodAttr", 5);
            m_behaviorContext->Method("globalMethod1", &globalMethod1);
            m_behaviorContext->Method("globalMethod2WithDefaultArgument", &globalMethod2WithDefaultArgument, { {{"Value", "An Integer argument", m_behaviorContext->MakeDefaultValue(defaultIntValue)}} });

            m_behaviorContext->Method("globalMethodContainers", &globalMethodContainers);
            m_behaviorContext->Method("globalMethodPair", &globalMethodPair);

            m_behaviorContext->Method("globalMethodToOverride", &globalMethodToOverride)
                    ->Attribute(Script::Attributes::MethodOverride, &globalMethodOverride);
            m_behaviorContext->Method("globalMethodLarge", &globalMethodLarge);
            m_behaviorContext->Method("TestTemplatedOnDemandReflection", &BehaviorTestTemplatedOnDemandReflection);
            m_behaviorContext->Method("IncrementTestSmartPtrSharedPtr", &IncrementBehaviorTestSmartPtrSharedPtr);
            m_behaviorContext->Method("IncrementTestSmartPtrIntrusivePtr", &IncrementBehaviorTestSmartPtrIntrusivePtr);

            m_behaviorContext->Method("globalMethodGetClassEnum", &globalMethodGetClassEnum);
            m_behaviorContext->Method("globalMethodSetClassEnum", &globalMethodSetClassEnum);
            m_behaviorContext->Method("PointerIsNullptr", &PointerIsNullptr);

            // Class
            m_behaviorContext->Class<BehaviorTestClass>()->
                    Constructor<int>()->
                    Attribute("ClassAttr",10)->
                    Attribute(AZ::Script::Attributes::Storage,AZ::Script::Attributes::StorageType::Value)->
                    Attribute(AZ::Script::Attributes::ConstructorOverride,&TestScriptNonIntrusiveConstructor)->
                Constant("epsilon", BehaviorConstant(0.001f))->
                Enum<(int)BehaviorTestClass::ME_VALUE0>("ME_VALUE0")->
                Enum<(int)BehaviorTestClass::MyClassEnum::MCE_VALUE0>("MCE_VALUE0")->
                Enum<(int)BehaviorTestClass::MyClassEnum::MCE_VALUE1>("MCE_VALUE1")->
                Method("Method1", &BehaviorTestClass::Method1)->
                    Attribute("MethodAttr", 20)->
                Method("Method2", &BehaviorTestClass::Method2)->
                Method("SpecialScriptMethod", &BehaviorTestClass::SpecialScriptMethod)->
                Method("OverrideMethod", &BehaviorTestClass::MethodToOverrideInScript)->
                    Attribute(Script::Attributes::MethodOverride, &BehaviorTestClass::Method1ScriptOverride)->
                Method("OverrideMethod1", &BehaviorTestClass::MethodToOverrideInScriptWithANonMember)->
                    Attribute(Script::Attributes::MethodOverride, &TestMethodToScriptOverride)->
                Method("GetClass", &BehaviorTestClass::GetClass)->
                Method("AddOperator", &BehaviorTestClass::AddOperator)->
                    Attribute(AZ::Script::Attributes::Operator,AZ::Script::Attributes::OperatorType::Add)->
                Method("ToString", &BehaviorTestClass::ToString)->
                Method("StaticMethod", &BehaviorTestClass::StaticMethod)->
                Method("MemberWithDefaultValues", &BehaviorTestClass::BoundsCheckMethodWithDefaultValue, { {{"value", "Value which will be checked to be within the two bounds arguments"}, {"delta", "The epsilon value", m_behaviorContext->MakeDefaultValue(0.1f)},
                    {"minBound", "The minimum bounds value,", m_behaviorContext->MakeDefaultValue(0.0f)}, {"maxBound", "The maximum bounds value", m_behaviorContext->MakeDefaultValue(1.0f)}} })->
                Property("data", &BehaviorTestClass::GetData, &BehaviorTestClass::SetData)->
                    Attribute("PropAttr", 30)->
                Property("data1", BehaviorValueProperty(&BehaviorTestClass::m_data1))->
                Property("dataReadOnly", BehaviorValueGetter(&BehaviorTestClass::m_dataReadOnly), nullptr);

            m_behaviorContext->Class<BehaviorDerivedTestClass>()->
                Property("derivedData", BehaviorValueProperty(&BehaviorDerivedTestClass::m_data));

            m_behaviorContext->Class<BehaviorTestSmartPtr>()->
                Method("TestFunction", &BehaviorTestSmartPtr::TestFunction)->
                Property("data", BehaviorValueProperty(&BehaviorTestSmartPtr::m_data));

            // Class Global Class enum wrapper
            m_behaviorContext->Class<BehaviorGlobalClassEnumWrapper>()
                ->Constant("VALUE1", BehaviorConstant(GlobalClassEnum::Value1))
                ->Constant("VALUE2", BehaviorConstant(GlobalClassEnum::Value2))
                ;

            // EBus
            const AZStd::string_view defaultStringViewValue = "DEFAULT!!!!";
            AZStd::string expectedDefaultValueAndStringResult = AZStd::string::format("Default Value: %s", defaultStringViewValue.data());
            BehaviorDefaultValuePtr defaultStringViewBehaviorValue = m_behaviorContext->MakeDefaultValue(defaultStringViewValue);
            BehaviorDefaultValuePtr superDefaultStringViewBehaviorValue = m_behaviorContext->MakeDefaultValue(AZStd::string_view("SUPER DEFAULT!!!!"));

            m_behaviorContext->EBus<BehaviorTestBus>("TestBus")
                    ->Attribute("EBusAttr", 40)
                ->Handler<BehaviorTestBusHandler>()
                    ->Attribute("HandlerAttr", 50)
                ->Event("OnEvent", &BehaviorTestBus::Events::OnEvent)
                    ->Attribute("EventAttr", 60)
                ->Event("OnEventWithResult", &BehaviorTestBus::Events::OnEventWithResult)
                ->Event("OnEventWithResultContainer", &BehaviorTestBus::Events::OnEventWithResultContainer)
                ->Event("OnEventWithClassEnumResult", &BehaviorTestBus::Events::OnEventWithClassEnumResult)
                ->Event("OnEventWithStringResult", &BehaviorTestBus::Events::OnEventWithStringResult)
                ->Event("OnEventResultWithBehaviorClassParameter", &BehaviorTestBus::Events::OnEventResultWithBehaviorClassParameter)
                ->Event("OnEventWithDefaultValueAndStringResult", &BehaviorTestBus::Events::OnEventWithDefaultValueAndStringResult,
                { {{"view1", "string_view without string trait", defaultStringViewBehaviorValue, AZ::BehaviorParameter::TR_NONE, AZ::BehaviorParameter::TR_STRING},
                    {"view2", "string_view with string trait", superDefaultStringViewBehaviorValue}} }) // Remove string trait from parameter
                ->Event("OnEventConst", &BehaviorTestBus::Events::OnEventConst)
                ;

            // Calls
            BehaviorMethod* method = m_behaviorContext->m_methods.find("globalMethod")->second;
            int result = 0;
            method->InvokeResult(result,3);
            AZ_TEST_ASSERT(result == 6);
            // check with default values
            method->InvokeResult(result);
            AZ_TEST_ASSERT(result == 558);

            //  Check attr
            AZ_Assert(method->m_attributes[0].first == Crc32("GlobalMethodAttr"), "Expected attribute");
            AttributeData<int>* attrData = azrtti_cast<AttributeData<int>*>(method->m_attributes[0].second);
            AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
            int attrValue = attrData->Get(nullptr);
            (void)attrValue;
            AZ_Assert(attrValue == 5, "Data should be 5");

            method = m_behaviorContext->m_methods.find("globalMethod1")->second;
            method->Invoke();

            method = m_behaviorContext->m_methods.find("globalMethod2WithDefaultArgument")->second;
            int defaultValueMethodResult = 0;
            method->InvokeResult(defaultValueMethodResult);
            EXPECT_EQ(defaultIntValue + 1, defaultValueMethodResult);

            AZStd::vector<int> values = { 10, 11, 12 };
            method = m_behaviorContext->m_methods.find("globalMethodContainers")->second;
            method->InvokeResult(result, values);

            // Global property
            BehaviorProperty* prop = m_behaviorContext->m_properties.find("globalProperty")->second;
            prop->m_setter->Invoke(3);
            prop->m_getter->InvokeResult(result);
            //  Check attr
            AZ_Assert(prop->m_attributes[0].first == Crc32("GlobalPropAttr"), "Expected attribute");
            attrData = azrtti_cast<AttributeData<int>*>(prop->m_attributes[0].second);
            AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
            attrValue = attrData->Get(nullptr);
            AZ_Assert(attrValue == 1, "Data should be 1");

            // Global read only property
            prop = m_behaviorContext->m_properties.find("globalPropertyReadOnly")->second;
            AZ_Assert(prop->m_setter == nullptr, "This property should be read only!");
            prop->m_getter->InvokeResult(result);

            // Constant
            prop = m_behaviorContext->m_properties.find("globalConstant")->second;
            float fResult = 0.0f;
            prop->m_getter->InvokeResult(fResult);

            // Enum
            GlobalClassEnum enumResult = GlobalClassEnum::Value2;
            method = m_behaviorContext->m_methods.find("globalMethodGetClassEnum")->second;
            method->InvokeResult(enumResult);
            AZ_TEST_ASSERT(enumResult == GlobalClassEnum::Value1);

            method = m_behaviorContext->m_methods.find("globalMethodSetClassEnum")->second;
            method->Invoke(GlobalClassEnum::Value2);

            // Classes
            BehaviorClass* behaviorClass = m_behaviorContext->m_classes.find("BehaviorTestClass")->second;
            BehaviorObject classInstance = behaviorClass->Create();
            {
                //  Check attr
                AZ_Assert(behaviorClass->m_attributes[0].first == Crc32("ClassAttr"), "Expected attribute");
                attrData = azrtti_cast<AttributeData<int>*>(behaviorClass->m_attributes[0].second);
                AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
                attrValue = attrData->Get(classInstance.m_address);
                AZ_Assert(attrValue == 10, "Data should be 10");
            }
            method = behaviorClass->m_methods.find("Method1")->second;
            method->Invoke(classInstance);
            {
                //  Check attr
                AZ_Assert(method->m_attributes[0].first == Crc32("MethodAttr"), "Expected attribute");
                attrData = azrtti_cast<AttributeData<int>*>(method->m_attributes[0].second);
                AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
                attrValue = attrData->Get(classInstance.m_address);
                AZ_Assert(attrValue == 20, "Data should be 20");
            }
            method = behaviorClass->m_methods.find("Method2")->second;
            EXPECT_TRUE(method->m_isConst);
            // Generic class instance
            int v = 5;
            method->InvokeResult(result, classInstance, &v);

            // Generic result
            BehaviorObject tc;
            method = behaviorClass->m_methods.find("GetClass")->second;
            method->InvokeResult(tc, classInstance);
            EXPECT_TRUE(method->IsMember());

            method = behaviorClass->m_methods.find("StaticMethod")->second;
            EXPECT_FALSE(method->IsMember());

            method = behaviorClass->m_methods.find("MemberWithDefaultValues")->second;
            EXPECT_TRUE(method->IsMember());
            bool withinBounds = false;
            method->InvokeResult(withinBounds, classInstance, 0.4f);
            EXPECT_TRUE(withinBounds);
            method->InvokeResult(withinBounds, classInstance, -1.1f);
            EXPECT_FALSE(withinBounds);

            // Class property
            prop = behaviorClass->m_properties.find("data")->second;
            EXPECT_TRUE(prop->m_getter->m_isConst);
            EXPECT_FALSE(prop->m_setter->m_isConst);
            prop->m_setter->Invoke(classInstance,12);
            prop->m_getter->InvokeResult(result,classInstance);
            {
                //  Check attr
                AZ_Assert(prop->m_attributes[0].first == Crc32("PropAttr"), "Expected attribute");
                attrData = azrtti_cast<AttributeData<int>*>(prop->m_attributes[0].second);
                AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
                attrValue = attrData->Get(classInstance.m_address);
                AZ_Assert(attrValue == 30, "Data should be 30");
            }


            // Class value property
            prop = behaviorClass->m_properties.find("data1")->second;
            prop->m_setter->Invoke(classInstance, 30);
            prop->m_getter->InvokeResult(result, classInstance);

            // Class value read only property
            prop = behaviorClass->m_properties.find("dataReadOnly")->second;
            AZ_Assert(prop->m_setter == nullptr, "Setter should be null!");
            prop->m_getter->InvokeResult(result, classInstance);

            // Test function conversion of parameters using RTTI (it this case base classes)
            method = behaviorClass->m_methods.find("Method1")->second;
            BehaviorDerivedTestClass tt;
            method->Invoke(&tt);

            //////////////////////////////////////////////////////////////////////////
            // EBus
            //////////////////////////////////////////////////////////////////////////

            // EBus - send events
            BehaviorTestBus::BusIdType testId = 1;
            BehaviorEBus* behaviorEBus = m_behaviorContext->m_ebuses.find("TestBus")->second;

            {
                //  Check attr
                AZ_Assert(behaviorEBus->m_attributes[0].first == Crc32("EBusAttr"), "Expected attribute");
                attrData = azrtti_cast<AttributeData<int>*>(behaviorEBus->m_attributes[0].second);
                AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
                attrValue = attrData->Get(nullptr);
                AZ_Assert(attrValue == 40, "Data should be 40");
            }

            // create handler for events
            BehaviorEBusHandler* testBusHandler = nullptr;
            behaviorEBus->m_createHandler->InvokeResult(testBusHandler);
            testBusHandler->InstallHook("OnEvent", &OnEventHook);
            testBusHandler->InstallHook("OnEventWithResult", &OnEventWithResultHook);

            // generic event handler
            BehaviorEBusHandler* genericTestBusHandler = nullptr;
            behaviorEBus->m_createHandler->InvokeResult(genericTestBusHandler);
            genericTestBusHandler->InstallGenericHook("OnEvent", &OnEventGenericHook);
            genericTestBusHandler->InstallGenericHook("OnEventWithResult", &OnEventGenericHook);
            {
                //  Check attr
                AZ_Assert(behaviorEBus->m_createHandler->m_attributes[0].first == Crc32("HandlerAttr"), "Expected attribute");
                attrData = azrtti_cast<AttributeData<int>*>(behaviorEBus->m_createHandler->m_attributes[0].second);
                AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
                attrValue = attrData->Get(nullptr);
                AZ_Assert(attrValue == 50, "Data should be 50");
            }

            // connect handlers to receive events
            testBusHandler->Connect(testId);
            genericTestBusHandler->Connect(testId);

            // fire some events
            BehaviorEBusEventSender* ebusSender = &behaviorEBus->m_events.find("OnEvent")->second;
            method = ebusSender->m_broadcast;
            method->Invoke(10);
            {
                //  Check attr
                AZ_Assert(ebusSender->m_attributes[0].first == Crc32("EventAttr"), "Expected attribute");
                attrData = azrtti_cast<AttributeData<int>*>(ebusSender->m_attributes[0].second);
                AZ_Assert(attrData != nullptr, "This must be a valid attribute!");
                attrValue = attrData->Get(nullptr);
                AZ_Assert(attrValue == 60, "Data should be 60");
            }
            method = behaviorEBus->m_events.find("OnEventWithResult")->second.m_broadcast;
            method->InvokeResult(result, 11);
            method = behaviorEBus->m_events.find("OnEventWithResult")->second.m_event;
            method->InvokeResult(result, testId, 15);

            method = behaviorEBus->m_events.find("OnEvent")->second.m_queueBroadcast;
            method->Invoke(1);
            method = behaviorEBus->m_events.find("OnEvent")->second.m_queueEvent;
            method->Invoke(testId, 2);

            method = behaviorEBus->m_events.find("OnEventWithDefaultValueAndStringResult")->second.m_broadcast;
            AZStd::string defaultStringResultValue;
            method->InvokeResult(defaultStringResultValue);
            EXPECT_EQ(expectedDefaultValueAndStringResult, defaultStringResultValue);

            method = behaviorEBus->m_events.find("OnEventWithDefaultValueAndStringResult")->second.m_event;
            defaultStringResultValue.clear();
            method->InvokeResult(defaultStringResultValue, testId);
            EXPECT_EQ(expectedDefaultValueAndStringResult, defaultStringResultValue);

            EXPECT_EQ(3u, method->GetNumArguments());
            EXPECT_EQ(1u, method->GetMinNumberOfArguments());
            EXPECT_TRUE(method->HasResult());
            const AZ::BehaviorParameter* stringResultParam = method->GetResult();
            EXPECT_NE(nullptr, stringResultParam);
            EXPECT_TRUE(AZ::BehaviorContextHelper::IsStringParameter(*stringResultParam));

            // The first string_view param has removed the TR_STRING trait
            const AZ::BehaviorParameter* stringView1Param = method->GetArgument(1);
            EXPECT_NE(nullptr, stringView1Param);
            EXPECT_FALSE(AZ::BehaviorContextHelper::IsStringParameter(*stringView1Param));

            // The second string_view param has the TR_STRING trait
            const AZ::BehaviorParameter* stringView2Param = method->GetArgument(2);
            EXPECT_NE(nullptr, stringView2Param);
            EXPECT_TRUE(AZ::BehaviorContextHelper::IsStringParameter(*stringView2Param));

            method = behaviorEBus->m_events.find("OnEventConst")->second.m_event;
            EXPECT_TRUE(method->m_isConst);

            method = behaviorEBus->m_events.find("OnEventConst")->second.m_broadcast;
            EXPECT_TRUE(method->m_isConst);

            method = behaviorEBus->m_events.find("OnEventConst")->second.m_queueEvent;
            EXPECT_TRUE(method->m_isConst);

            method = behaviorEBus->m_events.find("OnEventConst")->second.m_queueBroadcast;
            EXPECT_TRUE(method->m_isConst);

            BehaviorTestBus::ExecuteQueuedEvents();

            delete testBusHandler;
            delete genericTestBusHandler;

            // class enums
            {
                NativeBehaviorTestBusHandler nativeTestBusHandler;
                ebusSender = &behaviorEBus->m_events.find("OnEventWithClassEnumResult")->second;
                method = ebusSender->m_broadcast;
                GlobalClassEnum classEnumResult = GlobalClassEnum::Value1;
                method->InvokeResult(classEnumResult);
                AZ_TEST_ASSERT(classEnumResult == GlobalClassEnum::Value2);
            }

            behaviorClass->Destroy(classInstance);

            m_behaviorContext->Method("globalGenericMethod", &globalGenericMethod);
            {
                ScriptContext sc;
                sc.BindTo(m_behaviorContext);

                // global methods
                sc.Execute("value = globalMethod(12)");
                sc.Execute("value = globalGenericMethod(value)");
                sc.Execute("value = globalMethodToOverride(20)");
                sc.Execute("value = globalMethodLarge()");

                // global method with a vector (OnDemandReflection)
                sc.Execute("intVector = vector_int()");
                sc.Execute("intVector:push_back(74)");
                AZ_TEST_START_TRACE_SUPPRESSION;
                ScriptContext::ErrorHook oldHook = sc.GetErrorHook();
                sc.SetErrorHook(ScriptErrorAssertCB);
                sc.Execute("intVector.push_back(74)"); // missing self pointer
                sc.SetErrorHook(oldHook);
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);

                // test index operators
                sc.Execute("globalProperty = intVector[1]"); // read a value using custom operator
                AZ_TEST_ASSERT(g_globalValue == 74);
                //sc.Execute("intVector[6] = 128"); // write a value (set and resize) using custom operator
                //sc.Execute("globalProperty = intVector[2]");
                //AZ_TEST_ASSERT(g_globalValue == 0); // on resize we initialize with default value
                //sc.Execute("globalProperty = intVector[6]");
                //AZ_TEST_ASSERT(g_globalValue == 128);

                sc.Execute("pair = pair_int_bool()");
                sc.Execute("pair.first = 7; pair.second = true;");
                sc.Execute("globalProperty = globalMethodPair(pair)");
                EXPECT_EQ(7, g_globalValue);

                sc.Execute("TestTemplatedOnDemandReflection(intVector)");

                // global properties
                sc.Execute("value = globalProperty");
                sc.Execute("globalProperty = value + 10");

                // classes
                sc.Execute("testClass = BehaviorTestClass(11)");

                // class methods
                sc.Execute("testClass:Method1()");
                sc.Execute("testClass:Method2(12)");
                sc.Execute("value = testClass:SpecialScriptMethod(11)");
                sc.Execute("value = testClass:OverrideMethod(12)");
                sc.Execute("value = testClass:OverrideMethod1(13)");

                sc.Execute("testClass1 = testClass:GetClass()");

                // Test static methods
                sc.Execute("BehaviorTestClass.StaticMethod(1090)");
                EXPECT_EQ(1090, g_globalData);

                AZ_TEST_START_TRACE_SUPPRESSION;
                sc.Execute("testClass:StaticMethod(1090)");
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);

                AZ_TEST_START_TRACE_SUPPRESSION;
                sc.Execute("BehaviorTestClass.Method1(nil)");
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);

                // class properties
                sc.Execute("value = testClass.data");
                sc.Execute("testClass.data = value + 11");

                // class operators
                sc.Execute("testClass1 = BehaviorTestClass() testClass1.data = 203");
                sc.Execute("testClass2 = testClass1 + testClass");
                sc.Execute("testClass1.data = testClass2.data + 23");

                // derived classes
                sc.Execute("derivedTestClass = BehaviorDerivedTestClass()");
                sc.Execute("derivedTestClass.derivedData = 101");
                sc.Execute("derivedTestClass.data = 13");

                // smart ptr
                sc.Execute("smartIntrusivePtr = intrusive_ptr_BehaviorTestSmartPtr(BehaviorTestSmartPtr())");
                sc.Execute("IncrementTestSmartPtrIntrusivePtr(smartIntrusivePtr)"); // pass the smartPointer to a function as a smart pointer
                sc.Execute("rawPointer = smartIntrusivePtr:get() rawPointer:TestFunction()"); // get the raw pointer and call a function
                sc.Execute("smartIntrusivePtr:TestFunction()"); // call the wrapped function directly from the smart pointer

                sc.Execute("PointerIsNullptr(nil)");

                // ebus
                NativeBehaviorTestBusHandler myTestBusHandler;

                // broadcast
                sc.Execute("TestBus.Broadcast.OnEvent(10)");

                // class enum
                sc.Execute("eventClassResult = TestBus.Broadcast.OnEventWithClassEnumResult()");
                sc.Execute("globalMethodSetClassEnum(eventClassResult)");

                // events
                sc.Execute("TestBus.Event.OnEvent(0,20)"); // send event to ID 0 (not listener)
                sc.Execute("TestBus.Event.OnEvent(1,30)"); // send event to ID 1

                // queue broadcast
                sc.Execute("TestBus.QueueBroadcast.OnEvent(50)");

                // queue event
                sc.Execute("TestBus.QueueEvent.OnEvent(1,60)");

                // queue function
                sc.Execute("function QueuedFunctionOnTestBus(a,b,c)\
                                 globalProperty = a + b + c\
                            end\
                            TestBus.QueueFunction(QueuedFunctionOnTestBus,1,2,3)");

                BehaviorTestBus::ExecuteQueuedEvents();

                myTestBusHandler.BusDisconnect();

                //////////////////////////////////////////////////////////////////////////
                // handling ebus multiple return values
                NativeBehaviorTestBusHandler myTestBusHandler1;
                NativeBehaviorTestBusHandler myTestBusHandler2;
                myTestBusHandler1.m_enumResult = GlobalClassEnum::Value1;
                myTestBusHandler2.m_enumResult = GlobalClassEnum::Value2;

                // use a table as result container
                sc.Execute("eventClassResult = { TestBus.Broadcast.OnEventWithClassEnumResult() }");
                globalClassEnumValue = GlobalClassEnum::Value3;
                sc.Execute("globalMethodSetClassEnum(eventClassResult[1])");
                AZ_TEST_ASSERT(globalClassEnumValue == GlobalClassEnum::Value2);
                sc.Execute("globalMethodSetClassEnum(eventClassResult[2])");
                AZ_TEST_ASSERT(globalClassEnumValue == GlobalClassEnum::Value1);

                // use multiple variables as result container
                sc.Execute("result1, result2 = TestBus.Broadcast.OnEventWithClassEnumResult()");
                globalClassEnumValue = GlobalClassEnum::Value3;
                sc.Execute("globalMethodSetClassEnum(result1)");
                AZ_TEST_ASSERT(globalClassEnumValue == GlobalClassEnum::Value2);
                sc.Execute("globalMethodSetClassEnum(result2)");
                AZ_TEST_ASSERT(globalClassEnumValue == GlobalClassEnum::Value1);

                // collect garbage before running next test so any destructors get called
                sc.Execute(R"LUA(
                    collectgarbage()
                )LUA");

                g_globalTestClassesConstructed = 0;
                g_globalTestClassesDestructed = 0;

                // test whether the behavior parameters that are passed by value have their
                // constructors/destructors called equally
                sc.Execute(R"LUA(
                    local behaviorParameter = BehaviorTestClass();
                    local result = TestBus.Broadcast.OnEventResultWithBehaviorClassParameter(behaviorParameter);
                    behaviorParameter = nil;
                    result = nil;
                    collectgarbage();
                )LUA");

                AZ_TEST_ASSERT(g_globalTestClassesConstructed > 0);
                AZ_TEST_ASSERT(g_globalTestClassesDestructed > 0);
                AZ_TEST_ASSERT(g_globalTestClassesConstructed == g_globalTestClassesDestructed);

                myTestBusHandler1.BusDisconnect();
                myTestBusHandler2.BusDisconnect();
                //////////////////////////////////////////////////////////////////////////

                // create handler
                sc.Execute(R"LUA(
                    testBusHandler = {}
                    function testBusHandler:OnEvent(data)
                        globalProperty = data
                        globalProperty = TestBus.GetCurrentBusId()
                    end
                    function testBusHandler:OnEventWithClassEnumResult()
                        return BehaviorGlobalClassEnumWrapper.VALUE2
                    end
                    function testBusHandler:OnEventWithStringResult()
                        return 'success';
                    end
                    function testBusHandler:OnEventWithClassResult()
                        local result = BehaviorTestClass(100);
                        result.data = 100;
                        return result;
                    end
                    testBusHandler = TestBus.Connect(testBusHandler,1)
                )LUA");

                // check if we can handle event
                BehaviorTestBus::Broadcast(&BehaviorTestBus::Events::OnEvent, 101);

                // broadcast a class enum
                GlobalClassEnum globalClassEnumResult = GlobalClassEnum::Value1;
                BehaviorTestBus::BroadcastResult(globalClassEnumResult, &BehaviorTestBus::Events::OnEventWithClassEnumResult);
                AZ_TEST_ASSERT(globalClassEnumResult == GlobalClassEnum::Value2);

                AZStd::string stringResult;
                BehaviorTestBus::BroadcastResult(stringResult, &BehaviorTestBus::Events::OnEventWithStringResult);
                EXPECT_STREQ("success", stringResult.c_str());

                BehaviorTestClass classResult;
                BehaviorTestBus::BroadcastResult(classResult, &BehaviorTestBus::Events::OnEventWithClassResult);
                EXPECT_EQ(100, classResult.m_data);

                // disconnect
                sc.Execute("testBusHandler:Disconnect()");

                // test events
                BehaviorTestBus::Broadcast(&BehaviorTestBus::Events::OnEvent, 201); // it should have no effect

                //////////////////////////////////////////////////////////////////////////
                // Test reflecting after the bind
                m_behaviorContext->Method("BehaviorGlobalMethodAfterBind", &BehaviorGlobalMethodAfterBind);
                sc.Execute("afterBindValue = BehaviorGlobalMethodAfterBind()");
                m_behaviorContext->Property("globalPropertyAfterBind", &BehaviorGlobalPropertyGetAfterBind, &BehaviorGlobalPropertySetAfterBind);
                sc.Execute("globalPropertyAfterBind = afterBindValue");
                AZ_TEST_ASSERT(g_globalData == 3030);

                m_behaviorContext->Class<BehaviorClassAfterBind>()->
                    Property("data", BehaviorValueProperty(&BehaviorClassAfterBind::m_data));

                sc.Execute("classAfterBind = BehaviorClassAfterBind() classAfterBind.data = 1012 globalPropertyAfterBind = classAfterBind.data");
                AZ_TEST_ASSERT(g_globalData == 1012);
                //////////////////////////////////////////////////////////////////////////

                //////////////////////////////////////////////////////////////////////////
                // Classes using EBuses for communication and virtual ebus properties
                m_behaviorContext->Class<ClassInteractingWithEBus>()->
                    RequestBus("ClassRequestEBus");

                m_behaviorContext->EBus<ClassRequestEBus>("ClassRequestEBus")->
                    Attribute(AZ::Script::Attributes::DisallowBroadcast, true)->
                    Event("GetData", &ClassRequestEBus::Events::GetData)->
                    Event("SetData", &ClassRequestEBus::Events::SetData)->
                    VirtualProperty("data", "GetData", "SetData");

                BehaviorClass* classInteractingWithEBus = m_behaviorContext->m_classes.find("ClassInteractingWithEBus")->second;
                AZ_TEST_ASSERT(classInteractingWithEBus->m_requestBuses.size() == 1);
                AZ_TEST_ASSERT(classInteractingWithEBus->m_requestBuses.find("ClassRequestEBus") != classInteractingWithEBus->m_requestBuses.end());

                BehaviorEBus* classRequestBus = m_behaviorContext->m_ebuses.find("ClassRequestEBus")->second;
                AZ_TEST_ASSERT(classRequestBus->m_virtualProperties.size() == 1);
                const BehaviorEBus::VirtualProperty& virtualEBusProperty = classRequestBus->m_virtualProperties.find("data")->second;

                AZ_TEST_START_TRACE_SUPPRESSION;
                sc.Execute("ClassRequestEBus.Broadcast.GetData()");
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);

                {
                    ClassInteractingWithEBus listenerClassInstance; // setup a listener, otherwise property value can't be stored
                    listenerClassInstance.m_data = 1010;

                    // get the value of the ebus property. In practice those request buses use ID to identify a single handler, then we just use m_event instead of m_broadcast
                    int data = 0;
                    virtualEBusProperty.m_getter->m_broadcast->InvokeResult(data);
                    AZ_TEST_ASSERT(data == 1010);

                    // set the value of the ebus property.
                    virtualEBusProperty.m_setter->m_broadcast->Invoke(3030);
                    AZ_TEST_ASSERT(listenerClassInstance.m_data == 3030);
                }
                //////////////////////////////////////////////////////////////////////////
            }
        }
    };

    TEST_F(BehaviorContextTest, LuaTest)
    {
        run();
    }

    TEST_F(BehaviorContextTest, LuaBehaviorEBusHandlerWithDocMacroCompilesSuccessfully)
    {
        AZ::BehaviorContext behaviorContext;
        behaviorContext.EBus<BehaviorTestBus>("TestBusWithEbusHandlerThatSupportsNameAndTooltip")
            ->Handler<BehaviorTestBusHandlerWithDoc>()
            ;
        auto BehaviorEBusFoundIt = behaviorContext.m_ebuses.find("TestBusWithEbusHandlerThatSupportsNameAndTooltip");
        ASSERT_NE(behaviorContext.m_ebuses.end(), BehaviorEBusFoundIt);
        AZ::BehaviorEBus* behaviorEBus = BehaviorEBusFoundIt->second;
        ASSERT_NE(nullptr, behaviorEBus->m_createHandler);

        AZ::BehaviorEBusHandler* handlerResult{};
        EXPECT_TRUE(behaviorEBus->m_createHandler->InvokeResult(handlerResult));
        ASSERT_NE(nullptr, handlerResult);
        auto handlerDeleter = [behaviorEBus](AZ::BehaviorEBusHandler* handler)
        {
            behaviorEBus->m_destroyHandler->Invoke(handler);
        };
        AZStd::unique_ptr<AZ::BehaviorEBusHandler, decltype(handlerDeleter)> ebusHandler(handlerResult, AZStd::move(handlerDeleter));

        const AZ::BehaviorEBusHandler::EventArray handlerEvents = ebusHandler->GetEvents();
        auto stringViewEventIt = AZStd::find_if(handlerEvents.begin(), handlerEvents.end(), [](const AZ::BehaviorEBusHandler::BusForwarderEvent& handlerEvent)
            {
                return strcmp(handlerEvent.m_name, "OnEventWithDefaultValueAndStringResult") == 0;
            });
        ASSERT_NE(handlerEvents.end(), stringViewEventIt);
        ASSERT_EQ(AZ::eBehaviorBusForwarderEventIndices::ParameterFirst +  2, stringViewEventIt->m_metadataParameters.size());

        EXPECT_EQ("defaultView", stringViewEventIt->m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::ParameterFirst].m_name);
        EXPECT_EQ("string_view which contains literal to print by default", stringViewEventIt->m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::ParameterFirst].m_toolTip);
        EXPECT_EQ("unusedView", stringViewEventIt->m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::ParameterFirst + 1].m_name);
        EXPECT_EQ("Unused test parameter", stringViewEventIt->m_metadataParameters[AZ::eBehaviorBusForwarderEventIndices::ParameterFirst + 1].m_toolTip);
    }
} // namespace Unittest

#if !defined(AZCORE_EXCLUDE_LUA)

namespace UnitTest
{
    class IncompleteType;
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UnitTest::IncompleteType, "{53CC592A-E4B8-4F89-A293-87C7838A6108}");
    AZ_TYPE_INFO_SPECIALIZE(UnitTest::GlobalEnum, "{8A34A34C-B547-4724-8D3B-E12B8774E338}");
}

using namespace AZ;

namespace UnitTest
{
    int s_globalVar = 0;
    bool s_globalVarBool = false;
    float s_globalVar1 = 0.0f;
    float s_globalVar2ReadOnly = 10.0f;
    float s_globalVar3WriteOnly = 0.0f;

    AZStd::string s_globalVarString;

    int s_globalField = 0;
    int s_globalField1 = 12;
    int s_globalField2 = 0;

    static int s_errorCount = 0;

    IncompleteType* s_globalIncompletePtr = static_cast<IncompleteType*>(AZ_INVALID_POINTER);
    IncompleteType* s_globalIncompletePtr1 = nullptr;

    void GlobalVarSet(int v)
    {
        s_globalVar = v;
    }

    int  GlobalVarGet()
    {
        return s_globalVar;
    }

    void GlobalVarStringSet(const char* v)
    {
        s_globalVarString = v;
    }

    const char* GlobalVarStringGet()
    {
        return s_globalVarString.c_str();
    }

    void GlobalCheckString(AZStd::string globalVarString)
    {
        AZ_TEST_ASSERT(s_globalVarString == globalVarString);
    }

    void GlobalCheckStringRef(const AZStd::string& globalVarString)
    {
        AZ_TEST_ASSERT(s_globalVarString == globalVarString);
    }

    int GlobalFunc0()
    {
        return 0;
    }
    int GlobalFunc1(int)
    {
        return 1;
    }
    int GlobalFunc2(int, float)
    {
        return 2;
    }
    int GlobalFunc3(int, float, bool)
    {
        return 3;
    }
    int GlobalFunc4(int, float, bool, int)
    {
        return 4;
    }
    int GlobalFunc5(int, float, bool, int, float)
    {
        return 5;
    }
    int GlobalFunc0Override()
    {
        return 10;
    }
    void GlobalVoidFunc0()
    {
        s_globalVar = 0;
    }
    void GlobalVoidFunc1(int v)
    {
        s_globalVar = v;
    }
    void GlobalVoidFunc2(int v, float)           { s_globalVar = v; }
    void GlobalVoidFunc3(int v, float, bool)      { s_globalVar = v; }
    void GlobalVoidFunc4(int v, float, bool, int)  { s_globalVar = v; }
    void GlobalVoidFunc5(int v, float, bool, int, float) { s_globalVar = v; }

    void GlobalEnumFunc(GlobalEnum value)       { s_globalVar = static_cast<int>(value); }

    void VariadicFunc(ScriptDataContext& context)
    {
        s_globalVar = context.GetNumArguments();
        context.PushResult(s_globalVar * 2);
    }

    class ScriptClass2;
    class ScriptClass4;
    class ScriptClass5;

    class ScriptBindTest
        : public BehaviorContextFixture
    {
        static ScriptBindTest* s_scriptBindInstance;
    public:
        AZStd::fixed_vector<ScriptClass2*, 10> scriptClass2Instances;

        AZStd::fixed_vector< AZStd::shared_ptr<ScriptClass4>, 10 > scriptClass4lockedInstances;
        int scriptClass4numInstances = 0;

        AZStd::fixed_vector< AZStd::intrusive_ptr<ScriptClass5>, 10 > scriptClass5lockedInstances;
        int scriptClass5numInstances = 0;

        void SetUp() override;

        void TearDown() override;

        void ResetGlobalVars();

        static ScriptBindTest* GetScriptBindInstance()
        {
            return s_scriptBindInstance;
        }

        static bool EnumClass(const char* name, const AZ::Uuid& /*typeid*/, void* /*userData*/);

        static bool EnumMethod(const AZ::Uuid* /*type id*/, const char* name, const char* dbgParamInfo, void* /*userData*/);

        static bool EnumProperty(const AZ::Uuid* /*type id*/, const char* name, bool isRead, bool isWrite, void* /*userData*/);

        static void ScriptErrorCB(AZ::ScriptContext*, AZ::ScriptContext::ErrorType, const char* details);

        void run();
    };

    ScriptBindTest* ScriptBindTest::s_scriptBindInstance = nullptr;

    class ScriptClass* s_scriptClassInstance = nullptr;

    class ScriptClass
    {
    public:
        AZ_TYPE_INFO(ScriptClass, "{7b91cb47-a271-4031-8114-56d38efabc4f}");
        AZ_CLASS_ALLOCATOR(ScriptClass, SystemAllocator);

        enum LocalEnum
        {
            SC_ET_VALUE2 = 2,
            SC_ET_VALUE3,
        };

        ScriptClass()
            : m_data(0)
            , m_data1(0.0f)
            , m_data2ReadOnly(11.0f)
            , m_data3WriteOnly(0.0f)
        {
            if (s_scriptClassInstance == nullptr)
            {
                s_scriptClassInstance = this;
            }
        }
        ScriptClass(float dataReadOnly)
            : m_data(0)
            , m_data1(0.0f)
            , m_data2ReadOnly(dataReadOnly)
            , m_data3WriteOnly(0.0f)
        {
            if (s_scriptClassInstance == nullptr)
            {
                s_scriptClassInstance = this;
            }
        }

        ~ScriptClass()
        {
            if (s_scriptClassInstance == this)
            {
                s_scriptClassInstance = nullptr;
            }
        }

        int MemberFunc0()                           { return 0; }
        int MemberFunc1(int)                        { return 1; }
        int MemberFunc2(int, float)                  { return 2; }
        int MemberFunc3(int, float, bool)             { return 3; }
        int MemberFunc4(int, float, bool, int)         { return 4; }
        int MemberFunc5(int, float, bool, int, float)   { return 5; }
        void MemberVoidFunc0()                      { m_data = 0; }
        void MemberVoidFunc1(int v)                         { m_data = v; }
        void MemberVoidFunc2(int v, float)                   { m_data = v; }
        void MemberVoidFunc3(int v, float, bool)              { m_data = v; }
        void MemberVoidFunc4(int v, float, bool, int)          { m_data = v; }
        void MemberVoidFunc5(int v, float, bool, int, float)    { m_data = v; }

        int     GetData() const     { return m_data; }
        void    SetData(int data)   { m_data = data; }

        void    VariadicFunc(ScriptDataContext& context)
        {
            s_globalVar = context.GetNumArguments();
        }

        int m_data;
        float m_data1;
        float m_data2ReadOnly;
        float m_data3WriteOnly;

        struct ScriptClassNested
        {
            ScriptClassNested()
                : m_data(100) {}
            int m_data;
        };
    };

    void* ScriptClassAllocate(void* userData)
    {
        (void)userData;
        return azmalloc(sizeof(ScriptClass),AZStd::alignment_of<ScriptClass>::value,AZ::SystemAllocator);
    }

    void ScriptClassFree(void* obj, void* userData)
    {
        (void)userData;
        azfree(obj, AZ::SystemAllocator, sizeof(ScriptClass), AZStd::alignment_of<ScriptClass>::value);
    }

    class ScriptClass1
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptClass1, SystemAllocator);

        ScriptClass1()
            : m_data(0.0f) {}

        float m_data;
    };


    class ScriptClass2
    {
    public:
        AZ_TYPE_INFO(ScriptClass2, "{b4482151-b246-4c95-9ab3-db4589b3ffa0}");
        AZ_CLASS_ALLOCATOR(ScriptClass2, SystemAllocator);

        ScriptClass2(int data)
            : m_data(data)
        {
            ScriptBindTest::GetScriptBindInstance()->scriptClass2Instances.push_back(this);
        }
        ScriptClass2(const ScriptClass2& rhs)
        {
            m_data = rhs.m_data;
            ScriptBindTest::GetScriptBindInstance()->scriptClass2Instances.push_back(this);
        }
        ~ScriptClass2()
        {
            auto& scriptClass2Instances = ScriptBindTest::GetScriptBindInstance()->scriptClass2Instances;
            auto instanceIt = AZStd::find(scriptClass2Instances.begin(), scriptClass2Instances.end(), this);
            if (instanceIt != scriptClass2Instances.end())
            {
                scriptClass2Instances.erase(instanceIt);
            }
        }

        void    VariadicFunc(ScriptDataContext& context)
        {
            (void)context;
        }

        bool operator<(const ScriptClass2& rhs) const                   { return m_data < rhs.m_data; }
        bool operator<=(const ScriptClass2& rhs) const                  { return m_data < rhs.m_data; }
        bool operator==(const ScriptClass2& rhs) const                  { return m_data == rhs.m_data; }

        ScriptClass2 ScriptAdd(const ScriptClass2& rhs) const           { return ScriptClass2(m_data + rhs.m_data); }
        ScriptClass2 ScriptSub(const ScriptClass2& rhs) const           { return ScriptClass2(m_data - rhs.m_data); }
        ScriptClass2 ScriptMul(const ScriptClass2& rhs) const           { return ScriptClass2(m_data * rhs.m_data); }
        ScriptClass2 ScriptDiv(const ScriptClass2& rhs) const           { return ScriptClass2(m_data / rhs.m_data); }
        ScriptClass2 ScriptMod(const ScriptClass2& rhs) const           { return ScriptClass2(m_data % rhs.m_data); }
        ScriptClass2 ScriptPow(const ScriptClass2& rhs) const           { return ScriptClass2((int)pow((float)m_data, rhs.m_data)); }
        ScriptClass2 ScriptUnary() const
        {
            return ScriptClass2(-m_data);
        }
        ScriptClass2 ScriptConcat(const ScriptClass2& rhs)  const       { return ScriptClass2(m_data + rhs.m_data); }
        int         ScriptLength() const { return m_data; }

        const char* ToString() const                                    { return "This is ScriptClass2"; }

        ScriptClass2* Forward()                                         { return this; }

        int m_data;
    };

    class ScriptClass3* s_scriptClass3Instance = nullptr;
    class ScriptClass3
    {
    public:
        AZ_TYPE_INFO(ScriptClass3, "{058b255b-1abf-4831-9ea9-8b8cb6a7a634}");
        AZ_CLASS_ALLOCATOR(ScriptClass3, SystemAllocator);

        ScriptClass3()
        {
            m_bool = false;
            m_intData = 0;
            m_floatData = 0.0f;
            m_incompletePtr = nullptr;
        }

        ScriptClass3(ScriptDataContext& ctx)
        {
            m_bool = false;
            m_intData = 0;
            m_floatData = 0.0f;
            m_incompletePtr = nullptr;
            // unpack variadic parameters
            AZ_TEST_ASSERT(ctx.GetNumArguments() == 4);
            AZ_TEST_ASSERT(ctx.IsBoolean(0));
            AZ_TEST_ASSERT(ctx.IsNumber(1));
            AZ_TEST_ASSERT(ctx.IsNumber(2));
            AZ_TEST_ASSERT(ctx.IsRegisteredClass(3) == false);
            ctx.ReadArg(0, m_bool);
            ctx.ReadArg(1, m_intData);
            ctx.ReadArg(2, m_floatData);
            ctx.ReadArg(3, m_incompletePtr);

            m_scriptClass2 = nullptr;

            if (s_scriptClass3Instance == nullptr)
            {
                s_scriptClass3Instance = this;
            }
        }

        ~ScriptClass3()
        {
            if (s_scriptClass3Instance == this)
            {
                s_scriptClass3Instance = nullptr;
            }
        }

        const char* ToString() const                        { return "This is ScriptClass3"; }

        bool m_bool;
        int m_intData;
        float m_floatData;
        IncompleteType* m_incompletePtr;
        ScriptClass2* m_scriptClass2;

    private:
        // TODO: Simulate private copy constructor when AZStd::is_copy_constructible works
        //ScriptClass3(const ScriptClass3&) = delete;
    };

    class ScriptClass4
    {
    public:
        AZ_TYPE_INFO(ScriptClass4, "{4ecb714f-c73d-4a80-84d4-b55b145c3033}");
        AZ_CLASS_ALLOCATOR(ScriptClass4, SystemAllocator);

        ScriptClass4(int data)
            : m_data(data)
        {
            int& numClass4Instances = ScriptBindTest::GetScriptBindInstance()->scriptClass4numInstances;
            numClass4Instances++;
        }
        ~ScriptClass4()
        {
            int& numClass4Instances = ScriptBindTest::GetScriptBindInstance()->scriptClass4numInstances;
            numClass4Instances--;
        }

        static void HoldInstance(const AZStd::shared_ptr<ScriptClass4>& ptr)
        {
            auto& class4lockedInstances = ScriptBindTest::GetScriptBindInstance()->scriptClass4lockedInstances;
            class4lockedInstances.push_back(ptr);
        }
        void CopyData(AZStd::shared_ptr<ScriptClass4> rhs)
        {
            m_data = rhs->m_data;
        }
        int m_data;

    };

    class ScriptClass5
    {
    public:
        AZ_TYPE_INFO(ScriptClass5, "{99a1e378-d457-45d0-bd49-72b21d3368c5}");
        AZ_CLASS_ALLOCATOR(ScriptClass5, SystemAllocator);

        ScriptClass5(int data)
            : m_data(data)
            , m_refCount(0)
        {
            int& numClass5Instances = ScriptBindTest::GetScriptBindInstance()->scriptClass5numInstances;
            numClass5Instances++;
        }
        ~ScriptClass5()
        {
            int& numClass5Instances = ScriptBindTest::GetScriptBindInstance()->scriptClass5numInstances;
            numClass5Instances--;
        }
        void add_ref()          { m_refCount++; }
        void release()
        {
            m_refCount--;
            if (m_refCount == 0)
            {
                delete this;
            }
        }
        static void HoldInstance(const AZStd::intrusive_ptr<ScriptClass5>& ptr)
        {
            auto& class5lockedInstances = ScriptBindTest::GetScriptBindInstance()->scriptClass5lockedInstances;
            class5lockedInstances.push_back(ptr);
        }

        void CopyData(const AZStd::intrusive_ptr<ScriptClass5>& rhs)
        {
            m_data = rhs->m_data;
        }

        int m_data;
        int m_refCount;


    };

    struct ScriptValueClass
    {
        AZ_TYPE_INFO(ScriptValueClass, "{78b1baa9-483b-42c7-8c1e-4eba903bd529}");
        AZ_CLASS_ALLOCATOR(ScriptValueClass, SystemAllocator);

        ScriptValueClass()
            : m_data(10)
        {
        }

        ScriptValueClass(const ScriptValueClass& rhs)
        {
            (void)rhs;
            AZ_TEST_ASSERT(false); // we should not make copies
        }

        int m_data;
    };

    struct ScriptValueHolder
    {
        AZ_TYPE_INFO(ScriptValueHolder, "{86887e28-dfc6-4619-87d8-3658da0996ec}");
        AZ_CLASS_ALLOCATOR(ScriptValueHolder, SystemAllocator);

        ScriptValueHolder()
        {
        }
        ScriptValueHolder(const ScriptValueHolder& rhs)
        {
            (void)rhs;
            AZ_TEST_ASSERT(false); // we should not make copies
        }
        ScriptValueClass  m_value;
    };

    class ScriptUnregisteredBaseClass
    {
    public:
        AZ_RTTI(ScriptUnregisteredBaseClass, "{0ebe10b0-617b-4086-bff1-d51523db8cd6}");

        ScriptUnregisteredBaseClass()
            : m_baseData(1) {}
        virtual ~ScriptUnregisteredBaseClass() {}

        virtual int GetData() const { return m_baseData; }

        int m_baseData;
    };

    class ScriptUnregisteredDerivedClass
        : public ScriptUnregisteredBaseClass
    {
    public:
        AZ_RTTI(ScriptUnregisteredDerivedClass, "{5aba38c7-ae10-46d1-bc8e-3a0a00a3a97c}", ScriptUnregisteredBaseClass);

        ScriptUnregisteredDerivedClass()
            : m_derivedData(10) {}

        int GetData() const override { return m_derivedData; }

        int m_derivedData;
    };

    int   GetUnregisteredScriptBaseData(ScriptUnregisteredBaseClass* base)
    {
        return base->GetData();
    }

    int   GetUnregisteredScriptDerivedData(ScriptUnregisteredDerivedClass* derived)
    {
        return derived->m_derivedData;
    }

    ScriptUnregisteredBaseClass* s_globalUnregBaseClass = nullptr;
    ScriptUnregisteredDerivedClass* s_globalUnregDerivedClass = nullptr;

    class ScriptRegisteredBaseClass
    {
    public:
        AZ_RTTI(ScriptRegisteredBaseClass, "{73c2f822-d1ef-4abc-accb-cb36599f6b66}");
        AZ_CLASS_ALLOCATOR(ScriptRegisteredBaseClass, SystemAllocator);

        ScriptRegisteredBaseClass()
            : m_baseData(2) {}
        virtual ~ScriptRegisteredBaseClass()    {}

        virtual int VirtualFunction()
        {
            return 0;
        }

        int m_baseData;
    };

    ScriptRegisteredBaseClass* s_globalRegisteredBaseClass = nullptr;

    class ScriptRegisteredDerivedClass
        : public ScriptRegisteredBaseClass
    {
    public:
        AZ_RTTI(ScriptRegisteredDerivedClass, "{9f42df41-fdcc-44aa-909f-1f675804653c}", ScriptRegisteredBaseClass);
        AZ_CLASS_ALLOCATOR(ScriptRegisteredDerivedClass, SystemAllocator);

        ScriptRegisteredDerivedClass()
            : m_derivedData(11) {}
        ~ScriptRegisteredDerivedClass() override {}

        int VirtualFunction() override
        {
            return 1;
        }

        int m_derivedData;
    };

    class AbstractClass
    {
    public:
        AZ_RTTI(AbstractClass, "{d7082d66-108f-4177-8fb3-5cf25b510aa7}");

        virtual ~AbstractClass() {}
        virtual int PureCall() = 0;
    };

    class AbstractImplementation
        : public AbstractClass
    {
    public:
        AZ_RTTI(AbstractImplementation, "{62e1514d-57be-49c5-b829-d937f07276cd}", AbstractClass);
        AZ_CLASS_ALLOCATOR(AbstractImplementation, SystemAllocator);

        int PureCall() override { return 5; }
    };

    AZStd::shared_ptr<ScriptClass4> s_scriptMemberShared;
    AZStd::intrusive_ptr<ScriptClass5> s_scriptMemberIntrusive;


    void ScriptBindTest::SetUp()
    {
        BehaviorContextFixture::SetUp();
        ResetGlobalVars();
        s_scriptBindInstance = this;
        s_scriptMemberShared = AZStd::shared_ptr<ScriptClass4>(aznew ScriptClass4(10));
        s_scriptMemberIntrusive = AZStd::intrusive_ptr<ScriptClass5>(aznew ScriptClass5(10));
    }

    void ScriptBindTest::TearDown()
    {
        s_scriptMemberShared.reset();
        s_scriptMemberIntrusive.reset();
        s_scriptBindInstance = nullptr;

        BehaviorContextFixture::TearDown();
    }

    void ScriptBindTest::ResetGlobalVars()
    {
        s_globalVar = 0;
        s_globalVarBool = false;
        s_globalVar1 = 0.0f;
        s_globalVar2ReadOnly = 10.0f;
        s_globalVar3WriteOnly = 0.0f;

        s_globalVarString = AZStd::string();

        s_globalField = 0;
        s_globalField1 = 12;
        s_globalField2 = 0;

        s_errorCount = 0;

        s_globalIncompletePtr = static_cast<IncompleteType*>(AZ_INVALID_POINTER);
        s_globalIncompletePtr1 = nullptr;
    }

    bool ScriptBindTest::EnumClass(const char* name, const AZ::Uuid& /*typeid*/, void* /*userData*/)
    {
        AZ_TEST_ASSERT(name != nullptr);
        return true;
    }

    bool ScriptBindTest::EnumMethod(const AZ::Uuid* /*type id*/, const char* name, const char* dbgParamInfo, void* /*userData*/)
    {
        // make sure the debug info works
        if (strcmp(name, "GlobalFunc5") == 0)
        {
            AZ_TEST_ASSERT(strcmp(dbgParamInfo, "int NumberArguments (int intValue,float floatValue,bool Value,int intValue2,float floatValue2") == 0);
        }
        return true;
    }

    bool ScriptBindTest::EnumProperty(const AZ::Uuid* /*type id*/, const char* name, bool isRead, bool isWrite, void* /*userData*/)
    {
        if (strcmp(name, "globalVar") == 0)
        {
            AZ_TEST_ASSERT(isWrite && isRead);
        }
        if (strcmp(name, "globalVar2") == 0)
        {
            AZ_TEST_ASSERT(!isWrite && isRead);
        }
        if (strcmp(name, "globalVar3") == 0)
        {
            AZ_TEST_ASSERT(isWrite && !isRead);
        }
        return true;
    }

    void ScriptBindTest::ScriptErrorCB(AZ::ScriptContext*, AZ::ScriptContext::ErrorType, const char* details)
    {
        (void)details;
        s_errorCount++;
        AZ_Warning("Script", false, "%s", details);
    }

    void ScriptBindTest::run()
    {
        AZ::Uuid ch = AzTypeInfo<char>::Uuid();
        AZ::Uuid ch1 = AzTypeInfo<const char*>::Uuid();
        (void)ch; (void)ch1;

        // enum
        m_behaviorContext->Enum<GE_VALUE1>("GE_VALUE1")->
            Enum<GE_VALUE2>("GE_VALUE2");

        // constant
        m_behaviorContext->Constant("PI", BehaviorConstant(3.14f));

        // property
        m_behaviorContext->Property("globalVar", &GlobalVarGet, &GlobalVarSet);
        m_behaviorContext->Property("globalVar1", BehaviorValueProperty(&s_globalVar1));
        m_behaviorContext->Property("globalVarBool", BehaviorValueProperty(&s_globalVarBool));
        m_behaviorContext->Property("globalVar2", BehaviorValueGetter(&s_globalVar2ReadOnly), nullptr);
        m_behaviorContext->Property("globalVar3", nullptr, BehaviorValueSetter(&s_globalVar3WriteOnly));
        m_behaviorContext->Property("globalVarString", &GlobalVarStringGet, &GlobalVarStringSet);

        // field
        m_behaviorContext->Property("globalField", BehaviorValueProperty(&s_globalField));
        m_behaviorContext->Property("globalField1", BehaviorValueGetter(&s_globalField1), nullptr);
        m_behaviorContext->Property("globalField2", nullptr, BehaviorValueSetter(&s_globalField2));
        m_behaviorContext->Property("globalIncomplete", BehaviorValueProperty(&s_globalIncompletePtr)); // unregistered type (passed as light userdata)
        m_behaviorContext->Property("globalIncomplete1", BehaviorValueProperty(&s_globalIncompletePtr1)); // unregistered type (passed as light userdata)

        // method
        m_behaviorContext->Method("GlobalFunc0", &GlobalFunc0);
        m_behaviorContext->Method("GlobalFunc1", &GlobalFunc1);
        m_behaviorContext->Method("GlobalFunc2", &GlobalFunc2);
        m_behaviorContext->Method("GlobalFunc3", &GlobalFunc3);
        m_behaviorContext->Method("GlobalFunc4", &GlobalFunc4);
        m_behaviorContext->Method("GlobalFunc5", &GlobalFunc5, nullptr, "int NumberArguments (int intValue,float floatValue,bool Value,int intValue2,float floatValue2");
        m_behaviorContext->Method("GlobalVoidFunc0", &GlobalVoidFunc0);
        m_behaviorContext->Method("GlobalVoidFunc1", &GlobalVoidFunc1);
        m_behaviorContext->Method("GlobalVoidFunc2", &GlobalVoidFunc2);
        m_behaviorContext->Method("GlobalVoidFunc3", &GlobalVoidFunc3);
        m_behaviorContext->Method("GlobalVoidFunc4", &GlobalVoidFunc4);
        m_behaviorContext->Method("GlobalVoidFunc5", &GlobalVoidFunc5);
        m_behaviorContext->Method("GlobalEnumFunc", &GlobalEnumFunc);
        m_behaviorContext->Method("GlobalCheckString", &GlobalCheckString);
        m_behaviorContext->Method("GlobalCheckStringRef", &GlobalCheckStringRef);

        // clases
        m_behaviorContext->Class<ScriptClass>()->
            Allocator(&ScriptClassAllocate, &ScriptClassFree)->
            Enum<ScriptClass::SC_ET_VALUE2>("SC_ET_VALUE2")->
            Enum<ScriptClass::SC_ET_VALUE3>("SC_ET_VALUE3")->
            Constant("EPSILON", BehaviorConstant(0.001f))->
            Method("MemberFunc0", &ScriptClass::MemberFunc0)->
            Method("MemberFunc1", &ScriptClass::MemberFunc1)->
            Method("MemberFunc2", &ScriptClass::MemberFunc2)->
            Method("MemberFunc3", &ScriptClass::MemberFunc3)->
            Method("MemberFunc4", &ScriptClass::MemberFunc4)->
            Method("MemberFunc5", &ScriptClass::MemberFunc5)->
            Method("MemberVoidFunc0", &ScriptClass::MemberVoidFunc0)->
            Method("MemberVoidFunc1", &ScriptClass::MemberVoidFunc1)->
            Method("MemberVoidFunc2", &ScriptClass::MemberVoidFunc2)->
            Method("MemberVoidFunc3", &ScriptClass::MemberVoidFunc3)->
            Method("MemberVoidFunc4", &ScriptClass::MemberVoidFunc4)->
            Method("MemberVoidFunc5", &ScriptClass::MemberVoidFunc5)->
            Method("VariadicFunc", &ScriptClass::VariadicFunc)->
            Property("data", &ScriptClass::GetData, &ScriptClass::SetData)->
            Property("data1", BehaviorValueProperty(&ScriptClass::m_data1))->
            Property("data2", BehaviorValueGetter(&ScriptClass::m_data2ReadOnly), nullptr)->
            Property("data3", nullptr, BehaviorValueSetter(&ScriptClass::m_data3WriteOnly));//->
        //Class<ScriptClass::ScriptClassNested,ScriptContext::SP_RAW_SCRIPT_OWN>()->
        //  Field("data",&ScriptClass::ScriptClassNested::m_data)->
        //ClassEnd();

        m_behaviorContext->Class<ScriptClass2>()->
            Constructor<int>()->
            Method("Add", &ScriptClass2::ScriptAdd)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Add)->
            Method("Sub", &ScriptClass2::ScriptSub)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Sub)->
            Method("Mul", &ScriptClass2::ScriptMul)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mul)->
            Method("Div", &ScriptClass2::ScriptDiv)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Div)->
            Method("Mod", &ScriptClass2::ScriptMod)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Mod)->
            Method("Pow", &ScriptClass2::ScriptPow)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Pow)->
            Method("Unary", &ScriptClass2::ScriptUnary)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Unary)->
            Method("Concat", &ScriptClass2::ScriptConcat)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Concat)->
            Method("Length", &ScriptClass2::ScriptLength)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Length)->
            Method("Equal", &ScriptClass2::operator==)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)->
            Method("LessEqual", &ScriptClass2::operator<=)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessEqualThan)->
            Method("LessThan", &ScriptClass2::operator<)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::LessThan)->
            Method("ToString", &ScriptClass2::ToString)->
            Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
            Method("Forward", &ScriptClass2::Forward)->
            Property("data", BehaviorValueProperty(&ScriptClass2::m_data));

        m_behaviorContext->Class<ScriptClass3>()->
            Constructor<ScriptDataContext&>()->
            Property("boolData", BehaviorValueProperty(&ScriptClass3::m_bool))->
            Property("intData", BehaviorValueProperty(&ScriptClass3::m_intData))->
            Property("floatData", BehaviorValueProperty(&ScriptClass3::m_floatData))->
            Property("scriptClass2", BehaviorValueProperty(&ScriptClass3::m_scriptClass2));

        m_behaviorContext->Class<ScriptClass4>()->
            Constructor<int>()->
            Method("CopyData", &ScriptClass4::CopyData)->
            Property("data", BehaviorValueProperty(&ScriptClass4::m_data));

        m_behaviorContext->Property("scriptClass4Member", BehaviorValueProperty(&s_scriptMemberShared));
        m_behaviorContext->Method("HoldScriptClass4", &ScriptClass4::HoldInstance);

        m_behaviorContext->Class<ScriptClass5>()->
            Constructor<int>()->
            Method("CopyData", &ScriptClass5::CopyData)->
            Property("data", BehaviorValueProperty(&ScriptClass5::m_data));

        m_behaviorContext->Property("scriptClass5Member", BehaviorValueProperty(&s_scriptMemberIntrusive));
        m_behaviorContext->Method("HoldScriptClass5", &ScriptClass5::HoldInstance);

        // Variadic method
        m_behaviorContext->Method("VariadicFunc", &VariadicFunc);

        // Check nested classes
        m_behaviorContext->Class<ScriptValueClass>()->
            Property("data", BehaviorValueProperty(&ScriptValueClass::m_data));

        m_behaviorContext->Class<ScriptValueHolder>()->
            Property("value", BehaviorValueProperty(&ScriptValueHolder::m_value));

        // test class pointer casting for unregistered classes
        m_behaviorContext->Property("globalUnregBaseClass", BehaviorValueProperty(&s_globalUnregBaseClass));
        m_behaviorContext->Property("globalUnregDerivedClass", BehaviorValueProperty(&s_globalUnregDerivedClass));

        m_behaviorContext->Method("GetUnregisteredScriptBaseData", &GetUnregisteredScriptBaseData);
        m_behaviorContext->Method("GetUnregisteredScriptDerivedData", &GetUnregisteredScriptDerivedData);

        // register a base and derived class test
        m_behaviorContext->Class<ScriptRegisteredBaseClass>()->
            Property("baseData", BehaviorValueProperty(&ScriptRegisteredBaseClass::m_baseData))->
            Method("VirtualFunction", &ScriptRegisteredBaseClass::VirtualFunction);

        // when using AZ_RTTI the bind will discover the 'ScriptRegisteredBaseClass' and import it's bindings
        // into 'ScriptRegisteredDerivedClass' class so we access it (like in C++)
        m_behaviorContext->Class<ScriptRegisteredDerivedClass>()->
            Property("derivedData", BehaviorValueProperty(&ScriptRegisteredDerivedClass::m_derivedData));

        // add a function that makes a derived class (can change at runtime) and return pointer to a base class.
        m_behaviorContext->Property("globalRegisteredBaseClass", BehaviorValueProperty(&s_globalRegisteredBaseClass));

        // abstract classes
        m_behaviorContext->Class<AbstractClass>()->
            Method("PureCall", &AbstractClass::PureCall);

        m_behaviorContext->Class<AbstractImplementation>();

        // create the lua script context and Bind it the behavior
        ScriptContext script;
        script.BindTo(m_behaviorContext);

        // test globalVar with functions get and set
        AZ_TEST_ASSERT(s_globalVar == 0);
        script.Execute("globalVar = 5");
        AZ_TEST_ASSERT(s_globalVar == 5);
        script.Execute("globalVar = globalVar + 1");
        AZ_TEST_ASSERT(s_globalVar == 6);

        // test globalVar1 with variable address only
        AZ_TEST_ASSERT(s_globalVar1 == 0.0f);
        script.Execute("globalVar1 = 5");
        AZ_TEST_ASSERT(s_globalVar1 == 5.0f);
        script.Execute("globalVar1 = globalVar1 + 1.0");
        AZ_TEST_ASSERT(s_globalVar1 == 6.0f);

        // test globalVar2ReadOnly
        s_globalVar1 = 0.0f;
        AZ_TEST_ASSERT(s_globalVar2ReadOnly == 10.0f);
        script.Execute("globalVar1 = globalVar2");
        AZ_TEST_ASSERT(s_globalVar1 == 10.0f);
        //script.Execute("globalVar2 = 5.0"); //- this will cause script error (we can improve the message by providing a fake set function);

        // test globalVar2WriteOnly
        AZ_TEST_ASSERT(s_globalVar3WriteOnly == 0.0f);
        script.Execute("globalVar3 = 100");
        AZ_TEST_ASSERT(s_globalVar3WriteOnly == 100.0f);
        //script.Execute("print(\"globalVar3 is \",globalVar3)"); //- this will cause script error (we can improve the message by providing a fake set function);

        // globalField with functions get and set
        AZ_TEST_ASSERT(s_globalField == 0);
        script.Execute("globalField = 5");
        AZ_TEST_ASSERT(s_globalField == 5);
        script.Execute("globalField = globalField +1");
        AZ_TEST_ASSERT(s_globalField == 6);

        // globalField1 read only
        s_globalVar1 = 0.0f;
        AZ_TEST_ASSERT(s_globalField1 == 12.0f);
        script.Execute("globalVar1 = globalField1");
        AZ_TEST_ASSERT(s_globalVar1 == 12.0f);
        //script.Execute("globalField1 = 10.0"); //- this will cause script error (we can improve the message by providing a fake set function);

        // globalField2 write only
        AZ_TEST_ASSERT(s_globalField2 == 0.0f);
        script.Execute("globalField2 = 100");
        AZ_TEST_ASSERT(s_globalField2 == 100.0f);
        //script.Execute("print(\"globalField2 is \",globalField2)"); //- this will cause script error (we can improve the message by providing a fake set function);

        // s_globalVarString test
        AZ_TEST_ASSERT(s_globalVarString.empty());
        script.Execute("globalVarString = \"Hello\"");
        AZ_TEST_ASSERT(s_globalVarString.compare("Hello") == 0);

        // incomplete types passed by a light-user data (pointer reference)
        AZ_TEST_ASSERT(s_globalIncompletePtr == reinterpret_cast<IncompleteType*>(AZ_INVALID_POINTER));
        AZ_TEST_ASSERT(s_globalIncompletePtr1 == nullptr);

        script.Execute("globalIncomplete1 = globalIncomplete");
        AZ_TEST_ASSERT(s_globalIncompletePtr1 == s_globalIncompletePtr);

        // enum
        s_globalVar = 100;
        script.Execute("globalVar = 0");
        AZ_TEST_ASSERT(s_globalVar == 0);
        s_globalVar = 100;
        script.Execute("globalVar = GE_VALUE1");
        AZ_TEST_ASSERT(s_globalVar == GE_VALUE1);
        s_globalVar = 100;
        script.Execute("globalVar = GE_VALUE2");
        AZ_TEST_ASSERT(s_globalVar == GE_VALUE2);
        s_globalVar = 100;
        script.Execute("GlobalEnumFunc(GE_VALUE2)");
        AZ_TEST_ASSERT(s_globalVar == GE_VALUE2);

        // constant
        s_globalVar1 = 0.0f;
        script.Execute("globalVar1 = PI");
        AZ_TEST_ASSERT_FLOAT_CLOSE(s_globalVar1, 3.14f);

        // methods
        s_globalVar = 5;
        script.Execute("globalVar = GlobalFunc0()");
        AZ_TEST_ASSERT(s_globalVar == 0);
        s_globalVar = 0;
        script.Execute("globalVar = GlobalFunc1(2)");
        AZ_TEST_ASSERT(s_globalVar == 1);
        s_globalVar = 0;
        script.Execute("globalVar = GlobalFunc2(2,3)");
        AZ_TEST_ASSERT(s_globalVar == 2);
        s_globalVar = 0;
        script.Execute("globalVar = GlobalFunc3(2,3,true)");
        AZ_TEST_ASSERT(s_globalVar == 3);
        s_globalVar = 0;
        script.Execute("globalVar = GlobalFunc4(2,3,false,4)");
        AZ_TEST_ASSERT(s_globalVar == 4);
        s_globalVar = 0;
        script.Execute("globalVar = GlobalFunc5(2,3,true,5,2)");
        AZ_TEST_ASSERT(s_globalVar == 5);

        s_globalVar = 5;
        script.Execute("GlobalVoidFunc0()");
        AZ_TEST_ASSERT(s_globalVar == 0);
        s_globalVar = 0;
        script.Execute("GlobalVoidFunc1(2)");
        AZ_TEST_ASSERT(s_globalVar == 2);
        s_globalVar = 0;
        script.Execute("GlobalVoidFunc2(10,1)");
        AZ_TEST_ASSERT(s_globalVar == 10);
        s_globalVar = 0;
        script.Execute("GlobalVoidFunc3(15,3,true)");
        AZ_TEST_ASSERT(s_globalVar == 15);
        s_globalVar = 0;
        script.Execute("GlobalVoidFunc4(21,4,false,2)");
        AZ_TEST_ASSERT(s_globalVar == 21);
        s_globalVar = 0;
        script.Execute("GlobalVoidFunc5(101,102,true,2,1)");
        AZ_TEST_ASSERT(s_globalVar == 101);

        script.Execute("GlobalCheckString(globalVarString)");
        script.Execute("GlobalCheckStringRef(globalVarString)");

        // test a variadic function
        s_globalVar = 5;
        script.Execute("globalVar1 = VariadicFunc()");
        AZ_TEST_ASSERT(s_globalVar == 0);
        AZ_TEST_ASSERT_FLOAT_CLOSE(s_globalVar1, 0.0f);

        s_globalVar = 0;
        script.Execute("globalVar1 = VariadicFunc(101,102,true,2,1)");
        AZ_TEST_ASSERT(s_globalVar == 5);
        AZ_TEST_ASSERT_FLOAT_CLOSE(s_globalVar1, 10.0f);

        s_globalVar = 0;
        script.Execute("globalVar1 = VariadicFunc(101,102)");
        AZ_TEST_ASSERT(s_globalVar == 2);
        AZ_TEST_ASSERT_FLOAT_CLOSE(s_globalVar1, 4.0f);

        //////////////////////////////////////////////////////////////////////////
        // Classes
        s_globalVar = 0;
        script.Execute("globalVar = scriptClass4Member.data");
        AZ_TEST_ASSERT(s_globalVar == 10);
        script.Execute("scriptClass4Member = nil");
        AZ_TEST_ASSERT(s_scriptMemberShared == nullptr);
        script.Execute("scriptClass4Member = shared_ptr_ScriptClass4(ScriptClass4(12))");
        AZ_TEST_ASSERT(s_scriptMemberShared->m_data == 12);

        // test invalid assignment
        {
            s_errorCount = 0;
            auto originalErrorCB = script.GetErrorHook();
            script.SetErrorHook(&ScriptErrorCB);
            script.Execute("scriptClass4Member = ScriptClass5(12)");
            script.SetErrorHook(originalErrorCB);
            AZ_TEST_ASSERT(s_errorCount == 1);
        }

        script.Execute("scriptClass4Member = nil");
        script.GarbageCollect();
        AZ_TEST_ASSERT(scriptClass4numInstances == 0)

            // test assignments of nil to intrusive pointer
            s_globalVar = 0;
        script.Execute("globalVar = scriptClass5Member.data");
        AZ_TEST_ASSERT(s_globalVar == 10);
        script.Execute("scriptClass5Member = nil");
        AZ_TEST_ASSERT(s_scriptMemberShared == nullptr);
        script.Execute("scriptClass5Member = intrusive_ptr_ScriptClass5(ScriptClass5(12))");
        AZ_TEST_ASSERT(s_scriptMemberIntrusive->m_data == 12);

        // test invalid assignment
        {
            s_errorCount = 0;
            auto originalErrorCB = script.GetErrorHook();
            script.SetErrorHook(&ScriptErrorCB);
            script.Execute("scriptClass5Member = ScriptClass4(12)");
            script.SetErrorHook(originalErrorCB);
            AZ_TEST_ASSERT(s_errorCount == 1);
        }

        AZ_TEST_ASSERT(s_scriptMemberIntrusive->m_data == 12);
        script.Execute("scriptClass5Member = nil");
        script.GarbageCollect();
        AZ_TEST_ASSERT(scriptClass5numInstances == 0)

            // debug print
            AZ_TEST_ASSERT(script.GetDebugContext() == nullptr);
        script.EnableDebug();
        if (script.GetDebugContext() != nullptr) // debug is NOT supported in release builds
        {
            script.GetDebugContext()->EnumRegisteredGlobals(&EnumMethod, &EnumProperty);
            script.GetDebugContext()->EnumRegisteredClasses(&EnumClass, &EnumMethod, &EnumProperty);
            script.DisableDebug();
            AZ_TEST_ASSERT(script.GetDebugContext() == nullptr);
        }

        // create a ScriptClass instance from the script
        AZ_TEST_ASSERT(s_scriptClassInstance == nullptr);
        script.Execute("sc = ScriptClass()");
        AZ_TEST_ASSERT(s_scriptClassInstance != nullptr);

        // enum
        s_globalVar = 0;
        script.Execute("globalVar = sc.SC_ET_VALUE2");
        AZ_TEST_ASSERT(s_globalVar == 2);

        s_globalVar = 0;
        script.Execute("globalVar = sc.SC_ET_VALUE3");
        AZ_TEST_ASSERT(s_globalVar == 3);

        // constant
        s_globalVar1 = 0.0f;
        script.Execute("globalVar1 = sc.EPSILON");
        AZ_TEST_ASSERT_FLOAT_CLOSE(s_globalVar1, 0.001f);

        // methods
        s_globalVar = 1;
        script.Execute("globalVar = sc:MemberFunc0()");
        AZ_TEST_ASSERT(s_globalVar == 0);

        s_globalVar = 0;
        script.Execute("globalVar = sc:MemberFunc1(1)");
        AZ_TEST_ASSERT(s_globalVar == 1);

        s_globalVar = 0;
        script.Execute("globalVar = sc:MemberFunc2(2,2.0)");
        AZ_TEST_ASSERT(s_globalVar == 2);

        s_globalVar = 0;
        script.Execute("globalVar = sc:MemberFunc3(3,3.0,false)");
        AZ_TEST_ASSERT(s_globalVar == 3);

        s_globalVar = 0;
        script.Execute("globalVar = sc:MemberFunc4(4,4.0,true,40)");
        AZ_TEST_ASSERT(s_globalVar == 4);

        s_globalVar = 0;
        script.Execute("globalVar = sc:MemberFunc5(5,5.0,false,50,50.0)");
        AZ_TEST_ASSERT(s_globalVar == 5);

        s_scriptClassInstance->m_data = 10;
        script.Execute("sc:MemberVoidFunc0()");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data == 0);

        s_scriptClassInstance->m_data = 0;
        script.Execute("sc:MemberVoidFunc1(11)");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data == 11);

        s_scriptClassInstance->m_data = 0;
        script.Execute("sc:MemberVoidFunc2(7,11)");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data == 7);

        s_scriptClassInstance->m_data = 0;
        script.Execute("sc:MemberVoidFunc3(13,11,false)");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data == 13);

        s_scriptClassInstance->m_data = 0;
        script.Execute("sc:MemberVoidFunc4(21,11,false,40)");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data == 21);

        s_scriptClassInstance->m_data = 0;
        script.Execute("sc:MemberVoidFunc5(30,11,false,50,50.0)");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data == 30);

        // test number of arguments in a variadic function
        s_globalVar = 0;
        script.Execute("sc:VariadicFunc(30,11,false)");
        AZ_TEST_ASSERT(s_globalVar == 3);

        s_scriptClassInstance->m_data = 0;
        s_globalVar = 1;
        script.Execute("globalVar = sc.data");
        AZ_TEST_ASSERT(s_globalVar == 0);

        script.Execute("sc.data = 10");
        script.Execute("globalVar = sc.data");
        AZ_TEST_ASSERT(s_globalVar == 10);

        s_globalVar1 = 3.0f;
        script.Execute("globalVar1 = sc.data1");
        AZ_TEST_ASSERT(s_globalVar1 == 0.0f);

        script.Execute("sc.data1 = 11 ");
        script.Execute("globalVar1 = sc.data1");
        AZ_TEST_ASSERT(s_globalVar1 == 11.0f);

        s_globalVar1 = 0.0f;
        script.Execute("globalVar1 = sc.data2");
        AZ_TEST_ASSERT(s_globalVar1 == 11.0f);

        AZ_TEST_ASSERT(s_scriptClassInstance->m_data3WriteOnly == 0.0f);
        script.Execute("sc.data3 = 101");
        AZ_TEST_ASSERT(s_scriptClassInstance->m_data3WriteOnly == 101.0f);

        s_globalVarString.clear();
        script.Execute("globalVarString = tostring(sc)");
        AZ_TEST_ASSERT(s_globalVarString.find("ScriptClass") != AZStd::string::npos);
        AZ_TEST_ASSERT(s_globalVarString.find("LuaUserData") != AZStd::string::npos); 

        // release test
        script.Execute("sc = nil");
        script.GarbageCollect();
        AZ_TEST_ASSERT(s_scriptClassInstance == nullptr);

        //////////////////////////////////////////////////////////////////////////
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 0);
        script.Execute("sc2 = ScriptClass2(10)");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 1);
        AZ_TEST_ASSERT(scriptClass2Instances[0]->m_data == 10);

        script.Execute("sc21 = ScriptClass2(20)");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);
        AZ_TEST_ASSERT(scriptClass2Instances[1]->m_data == 20);

        // addition
        script.Execute("sc22 = sc2 + sc21");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == 30);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // sub
        script.Execute("sc22 = sc2 - sc21");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == -10);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // mul
        script.Execute("sc22 = sc2 * sc21");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == 200);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // div
        script.Execute("sc22 = sc21 / sc2");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == 2);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // mod
        script.Execute("sc21.data = 3 sc22 = sc2 % sc21 sc21.data = 20");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == 1);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // pow
        script.Execute("sc21.data = 2 sc22 = sc2 ^ sc21 sc21.data = 20");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == 100);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // unary
        script.Execute("sc22 = -sc2");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == -10);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // concat
        script.Execute("sc22 = sc2 .. sc21");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 3);
        AZ_TEST_ASSERT(scriptClass2Instances[2]->m_data == 30);
        script.Execute("sc22 = nil collectgarbage(\"collect\")");
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 2);

        // compare
        s_globalVarBool = true;
        script.Execute("globalVarBool = (sc2 == sc21)");
        AZ_TEST_ASSERT(!s_globalVarBool);
        script.Execute("sc21.data = 10 globalVarBool = (sc2 == sc21) sc21.data = 20");
        AZ_TEST_ASSERT(s_globalVarBool);

        // compare less than
        s_globalVarBool = false;
        script.Execute("globalVarBool = (sc2 < sc21)");
        AZ_TEST_ASSERT(s_globalVarBool);
        script.Execute("globalVarBool = (sc21 < sc2)");
        AZ_TEST_ASSERT(!s_globalVarBool);

        // compare less than
        s_globalVarBool = false;
        script.Execute("globalVarBool = (sc2 <= sc21)");
        AZ_TEST_ASSERT(s_globalVarBool);
        script.Execute("globalVarBool = (sc21 <= sc2)");
        AZ_TEST_ASSERT(!s_globalVarBool);

        // len
        s_globalVar = 0;
        script.Execute("globalVar = #sc2");
        AZ_TEST_ASSERT(s_globalVar == 10);

        // len
        s_globalVarString.clear();
        script.Execute("globalVarString = tostring(sc2)");
        AZ_TEST_ASSERT(s_globalVarString.compare("This is ScriptClass2") == 0);

        // test pass object by pointer (when it's script owned) back to script
        // we should make sure that we find the original instance
        script.Execute("sc2F = sc2:Forward()");

        // release test
        script.Execute("sc2 = nil sc21 = nil");
        script.GarbageCollect();
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 1);
        script.Execute("sc2F = nil");
        script.GarbageCollect();
        AZ_TEST_ASSERT(scriptClass2Instances.size() == 0);

        // test class with variadic constructor
        AZ_TEST_ASSERT(s_scriptClass3Instance == nullptr);
        script.Execute("sc3 = ScriptClass3(true,10,20.0,globalIncomplete)");
        AZ_TEST_ASSERT(s_scriptClass3Instance != nullptr);
        AZ_TEST_ASSERT(s_scriptClass3Instance->m_bool == true)
            AZ_TEST_ASSERT(s_scriptClass3Instance->m_intData == 10)
            AZ_TEST_ASSERT(s_scriptClass3Instance->m_floatData == 20.0f)
            AZ_TEST_ASSERT(s_scriptClass3Instance->m_incompletePtr == s_globalIncompletePtr);
        AZ_TEST_ASSERT(s_scriptClass3Instance->m_scriptClass2 == nullptr);

        // we need to transfer the ownership of the object
        script.Execute("sc3.scriptClass2 = ScriptClass2(333):ReleaseOwnership()");
        AZ_TEST_ASSERT(s_scriptClass3Instance->m_scriptClass2 != nullptr);
        AZ_TEST_ASSERT(s_scriptClass3Instance->m_scriptClass2->m_data == 333);
        script.Execute("sc3.scriptClass2.data = 444");
        AZ_TEST_ASSERT(s_scriptClass3Instance->m_scriptClass2->m_data == 444);
        ScriptClass2* scriptClass2ToDelete = s_scriptClass3Instance->m_scriptClass2; // we Lua released the ownership
        script.Execute("sc3.scriptClass2 = nil");
        AZ_TEST_ASSERT(s_scriptClass3Instance->m_scriptClass2 == nullptr);

        script.Execute("sc3 = nil collectgarbage(\"collect\")");
        delete scriptClass2ToDelete;
        AZ_TEST_ASSERT(s_scriptClass3Instance == nullptr);

        // test a class with shared_ptr owner policy
        AZ_TEST_ASSERT(scriptClass4numInstances == 0);
        script.Execute("sc4 = shared_ptr_ScriptClass4(ScriptClass4(101))"); // create an instance and DON'T HOLD A SHARED PTR
        AZ_TEST_ASSERT(scriptClass4numInstances == 1);
        AZ_TEST_ASSERT(scriptClass4lockedInstances.empty());
        script.Execute("sc41 = shared_ptr_ScriptClass4(ScriptClass4(201)) HoldScriptClass4(sc41)"); // create an instance and HOLD A SHARED PTR
        AZ_TEST_ASSERT(scriptClass4numInstances == 2);
        AZ_TEST_ASSERT(scriptClass4lockedInstances.size() == 1);
        AZ_TEST_ASSERT(scriptClass4lockedInstances[0]->m_data == 201);
        script.Execute("sc41:CopyData(sc4)");
        AZ_TEST_ASSERT(scriptClass4lockedInstances[0]->m_data == 101);
        script.Execute("sc4 = nil sc41 = nil collectgarbage(\"collect\")"); // this should release and delete sc4 but NOT sc41
        AZ_TEST_ASSERT(scriptClass4numInstances == 1);
        AZ_TEST_ASSERT(scriptClass4lockedInstances.size() == 1);
        scriptClass4lockedInstances.clear();
        AZ_TEST_ASSERT(scriptClass4numInstances == 0);

        // test a class with intrusive_ptr owner policy
        AZ_TEST_ASSERT(scriptClass5numInstances == 0);
        script.Execute("sc5 = intrusive_ptr_ScriptClass5(ScriptClass5(101))"); // create an instance and DON'T HOLD A INTRUSIVE PTR (check constructor)
        AZ_TEST_ASSERT(scriptClass5numInstances == 1);
        AZ_TEST_ASSERT(scriptClass5lockedInstances.empty());
        script.Execute("sc51 = intrusive_ptr_ScriptClass5(ScriptClass5(201)) HoldScriptClass5(sc51)"); // create an instance and HOLD A INTRUSIVE PTR (check constructor)
        AZ_TEST_ASSERT(scriptClass5numInstances == 2);
        AZ_TEST_ASSERT(scriptClass5lockedInstances.size() == 1);
        AZ_TEST_ASSERT(scriptClass5lockedInstances[0]->m_data == 201);
        script.Execute("sc51:CopyData(sc5)");
        AZ_TEST_ASSERT(scriptClass5lockedInstances[0]->m_data == 101);
        script.Execute("sc5 = nil sc51 = nil collectgarbage(\"collect\")"); // this should release and delete sc4 but NOT sc41
        AZ_TEST_ASSERT(scriptClass5numInstances == 1);
        AZ_TEST_ASSERT(scriptClass5lockedInstances.size() == 1);
        scriptClass5lockedInstances.clear();
        AZ_TEST_ASSERT(scriptClass5numInstances == 0);

        // Check nested classes

        // create valueHolder
        script.Execute("valueHolder = ScriptValueHolder()");

        // test reading user value data type
        script.Execute("globalVar1 = valueHolder.value.data");
        AZ_TEST_ASSERT(s_globalVar1 == 10.0f);

        // test writing user value data type
        script.Execute("valueHolder.value.data = 5");
        script.Execute("globalVar1 = valueHolder.value.data");
        AZ_TEST_ASSERT(s_globalVar1 == 5.0f);

        // create user value type
        script.Execute("userValueType = ScriptValueClass()");

        // test reading user value data type
        s_globalVar1 = 0.0f;
        script.Execute("globalVar1 = userValueType.data");
        AZ_TEST_ASSERT(s_globalVar1 == 10.0f);

        // test assignment of value types
        s_globalVar1 = 0.0f;
        script.Execute("valueHolder.value = userValueType");
        script.Execute("globalVar1 = valueHolder.value.data");
        AZ_TEST_ASSERT(s_globalVar1 == 10.0f);

        // make sure we copy the userValueType, not store a reference to it!
        script.Execute("userValueType.data = 100");
        script.Execute("globalVar1 = valueHolder.value.data");
        AZ_TEST_ASSERT(s_globalVar1 == 10.0f);

        // now store a reference and make sure it works
        script.Execute("userValueTypeRef = valueHolder.value");
        script.Execute("userValueTypeRef.data = 100");
        script.Execute("globalVar1 = valueHolder.value.data");
        AZ_TEST_ASSERT(s_globalVar1 == 100.0f);

        script.Execute("valueHolder = nil userValueType = nil");

        // test class pointer casting for unregistered classes
        ScriptUnregisteredBaseClass ubc;
        ScriptUnregisteredDerivedClass udc;

        s_globalUnregBaseClass = &udc;
        s_globalUnregDerivedClass = &udc;
        s_globalVar = 0;
        script.Execute("globalVar = GetUnregisteredScriptBaseData(globalUnregDerivedClass)"); // using downcast
        AZ_TEST_ASSERT(s_globalVar == 10);
        s_globalVar = 0;
        script.Execute("globalVar = GetUnregisteredScriptBaseData(globalUnregBaseClass)");
        AZ_TEST_ASSERT(s_globalVar == 10);
        s_globalVar = 0;
        script.Execute("globalVar = GetUnregisteredScriptDerivedData(globalUnregBaseClass)"); // using RTTI upcast
        AZ_TEST_ASSERT(s_globalVar == 10);

        // register a base and derived class test

        // test that derived classes copy the base class properties
        s_globalVar = 0;
        script.Execute("derivedWithBase = ScriptRegisteredDerivedClass()");
        script.Execute("globalVar = derivedWithBase.baseData"); // access base class data (RTTI is needed to be discovered and imported into top class metatable)
        AZ_TEST_ASSERT(s_globalVar == 2);
        // check that virtual functions work properly.
        script.Execute("globalRegisteredBaseClass = derivedWithBase"); // this will cast ScriptRegisteredDerivedClass to ScriptRegisteredBaseClass
        script.Execute("registeredBase = globalRegisteredBaseClass"); // this should cast back to the top known/registered class using AZRTTI.
        script.Execute("globalVar = registeredBase:VirtualFunction()");
        AZ_TEST_ASSERT(s_globalVar == 1);
        script.Execute("globalVar = derivedWithBase:VirtualFunction()");
        AZ_TEST_ASSERT(s_globalVar == 1);
        script.Execute("globalRegisteredBaseClass = nil registeredBase = nil derivedWithBase = nil");

        // abstract classes
        s_globalVar = 0;
        script.Execute("abstract = AbstractImplementation()");
        script.Execute("globalVar = abstract:PureCall()");
        AZ_TEST_ASSERT(s_globalVar == 5);
        script.Execute("abstract = nil");

        s_globalVarString.set_capacity(0); // free all memory
    }

    TEST_F(ScriptBindTest, LuaTest)
    {
        run();
    }

    class ScriptContextTest
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            ScriptContext script;

            const char code[] = "\
                        GlobalValueA = 6\
                        GlobalValueB = 12.4\
                        GlobalValueC = \"GlobalTest\"\
                        GlobalTable = {\
                            valueA = 5,\
                            valueB = 11.3,\
                            valueC = \"TableTest\",\
                            SubTable = {\
                                valueA = 6,\
                                valueB = 12.4,\
                                SubTable = {\
                                    valueA = 7,\
                                },\
                            },\
                        }\
                        function GlobalTable.TableFunction (a)\
                            return a\
                        end\
                        function GlobalFunction ()\
                            return 25\
                        end\
                        function GlobalSquare (a)\
                            return a,a*a\
                        end\
                        function GlobalFast (a)\
                            return a*2\
                        end\
                        GlobalTableWithMeta = {}\
                        setmetatable(GlobalTableWithMeta,GlobalTable)\
                        ";

            script.Execute(code);

            ScriptDataContext dc;
            bool status;
            int intResult = 0;
            float floatResult = 0.f;
            const char* stringResult = nullptr;

            // inspect test

            // global int
            status = script.FindGlobal("GlobalValueA", dc);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(dc.IsNumber(0));
            status = dc.ReadValue(0, intResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(intResult == 6);

            // global float
            status = script.FindGlobal("GlobalValueB", dc);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(dc.IsNumber(0));
            status = dc.ReadValue(0, floatResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT_FLOAT_CLOSE(floatResult, 12.4f);

            // string value
            status = script.FindGlobal("GlobalValueC", dc);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(dc.IsString(0));
            status = dc.ReadValue(0, stringResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(strcmp(stringResult, "GlobalTest") == 0);

            // table value
            status = script.FindGlobal("GlobalTable", dc);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(dc.IsTable(0));

            AZ_TEST_ASSERT(dc.CheckTableElement(0, "valueA"));
            status = dc.ReadTableElement(0, "valueA", intResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(intResult == 5);

            AZ_TEST_ASSERT(dc.CheckTableElement(0, "valueB"));
            status = dc.ReadTableElement(0, "valueB", floatResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT_FLOAT_CLOSE(floatResult, 11.3f);

            AZ_TEST_ASSERT(dc.CheckTableElement(0, "valueC"));
            status = dc.ReadTableElement(0, "valueC", stringResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(strcmp(stringResult, "TableTest") == 0);

            // inspect table
            {
                ScriptDataContext dcTable;
                dc.InspectTable(0, dcTable);
                const char* fieldName;
                int fieldIndex;
                int valueIndex;
                while (dcTable.InspectNextElement(valueIndex, fieldName, fieldIndex))
                {
                    AZ_TEST_ASSERT(fieldIndex == -1); // we don't have indexed elements
                    if (strcmp(fieldName, "valueA") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsNumber(valueIndex));
                        int intValue = 0;
                        dcTable.ReadValue(valueIndex, intValue);
                        AZ_TEST_ASSERT(intValue == 5);
                    }
                    else if (strcmp(fieldName, "valueB") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsNumber(valueIndex));
                        float floatValue = 0.f;
                        dcTable.ReadValue(valueIndex, floatValue);
                        AZ_TEST_ASSERT(fabsf(floatValue - 11.3f) < 0.001f);
                    }
                    else if (strcmp(fieldName, "valueC") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsString(valueIndex));
                        const char* stringValue;
                        dcTable.ReadValue(valueIndex, stringValue);
                        AZ_TEST_ASSERT(strcmp(stringResult, "TableTest") == 0);
                    }
                    else if (strcmp(fieldName, "SubTable") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsTable(valueIndex));
                        ScriptDataContext subTable;
                        dcTable.InspectTable(valueIndex, subTable);
                        while (subTable.InspectNextElement(valueIndex, fieldName, fieldIndex))
                        {
                            AZ_TEST_ASSERT(fieldIndex == -1); // we don't have indexed elements
                            if (strcmp(fieldName, "valueA") == 0)
                            {
                                AZ_TEST_ASSERT(subTable.IsNumber(valueIndex));
                                int intValue = 0;
                                subTable.ReadValue(valueIndex, intValue);
                                AZ_TEST_ASSERT(intValue == 6);
                            }
                            else if (strcmp(fieldName, "valueB") == 0)
                            {
                                AZ_TEST_ASSERT(subTable.IsNumber(valueIndex));
                                float floatValue = 0.f;
                                subTable.ReadValue(valueIndex, floatValue);
                                AZ_TEST_ASSERT(fabsf(floatValue - 12.4f) < 0.001f);
                            }
                            else if (strcmp(fieldName, "SubTable") == 0)
                            {
                                ScriptDataContext subTable1;
                                subTable.InspectTable(valueIndex, subTable1);
                                while (subTable1.InspectNextElement(valueIndex, fieldName, fieldIndex))
                                {
                                    AZ_TEST_ASSERT(fieldIndex == -1); // we don't have indexed elements
                                    if (strcmp(fieldName, "valueA") == 0)
                                    {
                                        AZ_TEST_ASSERT(subTable1.IsNumber(valueIndex));
                                        int intValue = 0;
                                        subTable1.ReadValue(valueIndex, intValue);
                                        AZ_TEST_ASSERT(intValue == 7);
                                    }
                                }
                            }
                        }
                    }
                    else if (strcmp(fieldName, "TableFunction") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsFunction(valueIndex));
                        ScriptDataContext callContext;
                        dcTable.Call(valueIndex, callContext);
                        callContext.PushArg(11);
                        callContext.CallExecute();
                        AZ_TEST_ASSERT(callContext.GetNumResults() == 1);
                        int intValue = 0;
                        callContext.ReadResult(0, intValue);
                        AZ_TEST_ASSERT(intValue == 11);
                    }
                    else
                    {
                        AZ_TEST_ASSERT(false);
                    }
                }
            }

            // table replace
            dc.AddReplaceTableElement(0, "valueA", 4);
            status = dc.ReadTableElement(0, "valueA", intResult);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(intResult == 4);

            // table add
            AZ_TEST_ASSERT(dc.CheckTableElement(0, "valueD") == false);
            dc.AddReplaceTableElement(0, "valueD", 11);
            AZ_TEST_ASSERT(dc.CheckTableElement(0, "valueD"));
            dc.ReadTableElement(0, "valueD", intResult);
            AZ_TEST_ASSERT(intResult == 11);

            // global function call (no arguments 1 result)
            status = script.FindGlobal("GlobalFunction", dc);
            AZ_TEST_ASSERT(status);

            {
                ScriptDataContext callContext;
                status = dc.Call(0, callContext);
                AZ_TEST_ASSERT(status);
                status = callContext.CallExecute();
                AZ_TEST_ASSERT(status);
                AZ_TEST_ASSERT(callContext.GetNumResults() == 1);
                AZ_TEST_ASSERT(callContext.IsNumber(0));
                callContext.ReadResult(0, intResult);
                AZ_TEST_ASSERT(intResult == 25);
            }

            // global function call (1 argument 2 results)
            {
                ScriptDataContext callContext;
                status = script.FindGlobal("GlobalSquare", dc);
                AZ_TEST_ASSERT(status);
                status = dc.Call(0, callContext);
                AZ_TEST_ASSERT(status);
                AZ_TEST_ASSERT(callContext.GetNumArguments() == 0);
                callContext.PushArg(3);
                AZ_TEST_ASSERT(callContext.GetNumArguments() == 1);
                status = callContext.CallExecute();
                AZ_TEST_ASSERT(status);
                AZ_TEST_ASSERT(callContext.GetNumResults() == 2);
                AZ_TEST_ASSERT(callContext.IsNumber(0));
                AZ_TEST_ASSERT(callContext.IsNumber(1));
                int intResult2 = 0;
                callContext.ReadResult(0, intResult);
                callContext.ReadResult(1, intResult2);
                AZ_TEST_ASSERT(intResult == 3);
                AZ_TEST_ASSERT(intResult2 == 9);
            }

            // global cached function call
            int cachedIndex = script.CacheGlobal("GlobalFast");
            AZ_TEST_ASSERT(cachedIndex != -1);
            int value = 2;
            for (int i = 0; i < 6; ++i)
            {
                ScriptDataContext callContext;
                status = script.FindGlobal(cachedIndex, dc);
                AZ_TEST_ASSERT(status);
                status = dc.Call(0, callContext);
                AZ_TEST_ASSERT(status);
                callContext.PushArg(value);
                status = callContext.CallExecute();
                AZ_TEST_ASSERT(status);
                AZ_TEST_ASSERT(callContext.GetNumResults() == 1);
                AZ_TEST_ASSERT(callContext.IsNumber(0));
                callContext.ReadResult(0, value);
            }
            AZ_TEST_ASSERT(value == 128);
            script.ReleaseCached(cachedIndex);

            // metatable inspect
            status = script.FindGlobal("GlobalTableWithMeta", dc);
            AZ_TEST_ASSERT(status);
            AZ_TEST_ASSERT(dc.IsTable(0));

            {
                ScriptDataContext dcTable;
                dc.InspectTable(0, dcTable);
                const char* fieldName;
                int fieldIndex;
                int valueIndex;
                while (dcTable.InspectNextElement(valueIndex, fieldName, fieldIndex))
                {
                    AZ_TEST_ASSERT(false); // this table doesn't have any elements
                }

                status = dc.InspectMetaTable(0, dcTable);
                AZ_TEST_ASSERT(status);
                while (dcTable.InspectNextElement(valueIndex, fieldName, fieldIndex))
                {
                    if (strcmp(fieldName, "valueA") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsNumber(valueIndex));
                        int intValue = 0;
                        dcTable.ReadValue(valueIndex, intValue);
                        AZ_TEST_ASSERT(intValue == 4);
                    }
                    else if (strcmp(fieldName, "valueB") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsNumber(valueIndex));
                        float floatValue = 0.f;
                        dcTable.ReadValue(valueIndex, floatValue);
                        AZ_TEST_ASSERT(fabsf(floatValue - 11.3f) < 0.001f);
                    }
                    else if (strcmp(fieldName, "valueC") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsString(valueIndex));
                        const char* stringValue;
                        dcTable.ReadValue(valueIndex, stringValue);
                        AZ_TEST_ASSERT(strcmp(stringResult, "TableTest") == 0);
                    }
                    else if (strcmp(fieldName, "valueD") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsNumber(valueIndex));
                        int intValue = 0;
                        dcTable.ReadValue(valueIndex, intValue);
                        AZ_TEST_ASSERT(intValue == 11);
                    }
                    else if (strcmp(fieldName, "SubTable") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsTable(valueIndex));
                        ScriptDataContext subTable;
                        dcTable.InspectTable(valueIndex, subTable);
                        while (subTable.InspectNextElement(valueIndex, fieldName, fieldIndex))
                        {
                            AZ_TEST_ASSERT(fieldIndex == -1); // we don't have indexed elements
                            if (strcmp(fieldName, "valueA") == 0)
                            {
                                AZ_TEST_ASSERT(subTable.IsNumber(valueIndex));
                                int intValue = 0;
                                subTable.ReadValue(valueIndex, intValue);
                                AZ_TEST_ASSERT(intValue == 6);
                            }
                            else if (strcmp(fieldName, "valueB") == 0)
                            {
                                AZ_TEST_ASSERT(subTable.IsNumber(valueIndex));
                                float floatValue = 0.f;
                                subTable.ReadValue(valueIndex, floatValue);
                                AZ_TEST_ASSERT(fabsf(floatValue - 12.4f) < 0.001f);
                            }
                            else if (strcmp(fieldName, "SubTable") == 0)
                            {
                                ScriptDataContext subTable1;
                                subTable.InspectTable(valueIndex, subTable1);
                                while (subTable1.InspectNextElement(valueIndex, fieldName, fieldIndex))
                                {
                                    AZ_TEST_ASSERT(fieldIndex == -1); // we don't have indexed elements
                                    if (strcmp(fieldName, "valueA") == 0)
                                    {
                                        AZ_TEST_ASSERT(subTable1.IsNumber(valueIndex));
                                        int intValue = 0;
                                        subTable1.ReadValue(valueIndex, intValue);
                                        AZ_TEST_ASSERT(intValue == 7);
                                    }
                                }
                            }
                        }
                    }
                    else if (strcmp(fieldName, "TableFunction") == 0)
                    {
                        AZ_TEST_ASSERT(dcTable.IsFunction(valueIndex));
                        ScriptDataContext callContext;
                        dcTable.Call(valueIndex, callContext);
                        callContext.PushArg(11);
                        callContext.CallExecute();
                        AZ_TEST_ASSERT(callContext.GetNumResults() == 1);
                        int intValue = 0;
                        callContext.ReadResult(0, intValue);
                        AZ_TEST_ASSERT(intValue == 11);
                    }
                    else
                    {
                        AZ_TEST_ASSERT(false);
                    }
                }
            }

            // script loaders

            // from load function

            // from data buffer

            // from string
        }
    };

    TEST_F(ScriptContextTest, LuaTest)
    {
        run();
    }

    class ScriptDebugTest
        : public LeakDetectionFixture
    {
        int m_numBreakpointHits;

        static ScriptContext* s_currentSC;
    public:
        ScriptDebugTest()
            : m_numBreakpointHits(0) {}

        bool EnumLocal(const char* name, ScriptDataContext& dataContext)
        {
            AZ_TEST_ASSERT(strcmp(name, "a") == 0 || strcmp(name, "result") == 0);
            //AZ_Printf("Script","Local variable %s = ",name);
            if (dataContext.IsNumber(0))
            {
                double number = 0.;
                dataContext.ReadValue(0, number);
                //AZ_Printf("Script","%f [number]\n",number);
                AZ_TEST_ASSERT(number == 10.0);
            }
            return true;
        }

        void BreakpointCallback(ScriptContextDebug* debugContext, const ScriptContextDebug::Breakpoint* breakpoint)
        {
            //AZ_Printf("Script","\nBreakpoint at '%s(%d)' triggered!",breakpoint->m_sourceName,breakpoint->m_lineNumber);
            AZ_TEST_ASSERT(breakpoint->m_sourceName == "DebugCode");

            if (m_numBreakpointHits == 0)
            {
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 23);
                ScriptContextDebug::EnumLocalCallback enumCB = AZStd::bind(&ScriptDebugTest::EnumLocal, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
                debugContext->EnumLocals(enumCB);
                debugContext->StepInto(); // go inside the script to line 20
            }
            else if (m_numBreakpointHits == 1)
            {
                char stackOutput[2048];
                debugContext->StackTrace(stackOutput, AZ_ARRAY_SIZE(stackOutput));
                AZ_Printf("Script", "%s", stackOutput);
                AZ_TEST_ASSERT(strstr(stackOutput, "GlobalFunction") != nullptr);
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 20);
            }
            else if (m_numBreakpointHits == 2)
            {
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 23);
                // check A a local variable
                ScriptContextDebug::DebugValue value;
                value.m_name = "a";
                bool valueResult = debugContext->GetValue(value);
                AZ_TEST_ASSERT(valueResult);
                AZ_TEST_ASSERT(value.m_type == LUA_TNUMBER);
                AZ_TEST_ASSERT(value.m_value.find("11.0") != OSString::npos);
                value.m_value = "3.0"; // modify the value
                debugContext->SetValue(value);
                debugContext->StepOver(); // same place as before, but now step over to line 24
            }
            else if (m_numBreakpointHits == 3)
            {
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 24);
            }
            else if (m_numBreakpointHits == 4)
            {
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 23);
                debugContext->StepOut(); // same place as before, but now step out to line 28
            }
            else if (m_numBreakpointHits == 5)
            {
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 28);
            }
            else if (m_numBreakpointHits == 6)
            {
                // called from C->script->C->script scenario
                char stackOutput[2048];
                debugContext->StackTrace(stackOutput, AZ_ARRAY_SIZE(stackOutput));
                AZ_Printf("Script", "%s", stackOutput);
                AZ_TEST_ASSERT(strstr(stackOutput, "GlobalMult") != nullptr);
                AZ_TEST_ASSERT(breakpoint->m_lineNumber == 23);
            }

            ++m_numBreakpointHits;
        }

        static int CallCFuncOperation(int value)
        {
            int result = 0;
            // call a script function
            if (s_currentSC)
            {
                ScriptDataContext call;
                if (s_currentSC->Call("GlobalFast", call))
                {
                    call.PushArg(value);
                    if (call.CallExecute())
                    {
                        if (call.GetNumResults())
                        {
                            call.ReadResult(0, result);
                        }
                    }
                }
            }
            return result;
        }

        void run()
        {
            BehaviorContext behaviorContext;
            behaviorContext.Class<ScriptClass>()->
                Allocator(&ScriptClassAllocate, &ScriptClassFree)->
                Enum<ScriptClass::SC_ET_VALUE2>("SC_ET_VALUE2")->
                Enum<ScriptClass::SC_ET_VALUE3>("SC_ET_VALUE3")->
                Constant("EPSILON", BehaviorConstant(0.001f))->
                Method("MemberFunc0", &ScriptClass::MemberFunc0)->
                Method("MemberFunc1", &ScriptClass::MemberFunc1)->
                Method("MemberFunc2", &ScriptClass::MemberFunc2)->
                Method("MemberFunc3", &ScriptClass::MemberFunc3)->
                Method("MemberFunc4", &ScriptClass::MemberFunc4)->
                Method("MemberFunc5", &ScriptClass::MemberFunc5)->
                Method("MemberVoidFunc0", &ScriptClass::MemberVoidFunc0)->
                Method("MemberVoidFunc1", &ScriptClass::MemberVoidFunc1)->
                Method("MemberVoidFunc2", &ScriptClass::MemberVoidFunc2)->
                Method("MemberVoidFunc3", &ScriptClass::MemberVoidFunc3)->
                Method("MemberVoidFunc4", &ScriptClass::MemberVoidFunc4)->
                Method("MemberVoidFunc5", &ScriptClass::MemberVoidFunc5)->
                Method("VariadicFunc", &ScriptClass::VariadicFunc)->
                Property("data", &ScriptClass::GetData, &ScriptClass::SetData)->
                Property("data1", BehaviorValueProperty(&ScriptClass::m_data1))->
                Property("data2", BehaviorValueGetter(&ScriptClass::m_data2ReadOnly), nullptr)->
                Property("data3", nullptr, BehaviorValueSetter(&ScriptClass::m_data3WriteOnly));

            behaviorContext.Property("globalField", BehaviorValueProperty(&s_globalField));

            behaviorContext.Method("CallCFuncOperation", &CallCFuncOperation);

            behaviorContext.Class<ScriptValueClass>()->
                Property("data", BehaviorValueProperty(&ScriptValueClass::m_data));

            behaviorContext.Class<ScriptValueHolder>()->
                Property("value", BehaviorValueProperty(&ScriptValueHolder::m_value));

            ScriptContext script;

            script.BindTo(&behaviorContext);

            const char code[] = "GlobalValueA = 6\n\
                                GlobalValueB = 12.4\n\
                                GlobalValueC = \"GlobalTest\"\n\
                                GlobalTable = {\n\
                                    valueA = 5,\n\
                                    valueB = 11.3,\n\
                                    valueC = \"TableTest\",\n\
                                    SubTable = {\n\
                                        valueA = 6,\n\
                                        valueB = 12.4,\n\
                                        SubTable = {\n\
                                        valueA = 7,\n\
                                        },\n\
                                    },\n\
                                }\n\
                                function GlobalTable.TableFunction (a)\n\
                                    return a\n\
                                end\n\
                                function GlobalFunction ()\n\
                                    return 25\n\
                                end\n\
                                function GlobalMult (a)\n\
                                    local result = a*GlobalFunction()\n\
                                    return result\n\
                                end\n\
                                function GlobalFast (a)\n\
                                    local result = GlobalMult(a)*2\n\
                                    return result\n\
                                end\n\
                                Result=GlobalFast(10)\n\
                                Result=GlobalFast(11)\n\
                                Result=GlobalFast(12)\n\
                                GlobalValueB=Result*2\n\
                                GlobalTableDerived={}\n\
                                setmetatable(GlobalTableDerived,GlobalTable)";

            const char code2[] = "function CallACFunction(a)\n\
                                    return CallCFuncOperation(a)\n\
                                 end\n\
                                 Result = CallACFunction(10)";

            // enable script debug
            script.EnableDebug();

            // get debug context
            ScriptContextDebug* debugContext = script.GetDebugContext();

            if (debugContext == nullptr)
            {
                return;                           // don't test when not supported.
            }

            // add a breakpoint code line 23 (return a*a)
            ScriptContextDebug::Breakpoint bp;
            bp.m_sourceName = "DebugCode";
            bp.m_lineNumber = 23;
            debugContext->AddBreakpoint(bp);

            // enable breakpoints
            ScriptContextDebug::BreakpointCallback breakpointCallback = AZStd::bind(&ScriptDebugTest::BreakpointCallback, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
            debugContext->EnableBreakpoints(breakpointCallback);

            // enabled 'advanced' stack tracing
            debugContext->EnableStackRecord();

            // execute the code (that will trigger a breakpoint)
            script.Execute(code, "DebugCode");

            // now check debug break with script1->C Code->script2
            s_currentSC = &script;
            script.Execute(code2, "DebugCode2");
            s_currentSC = nullptr;

            // disable breakpoints (not this doesn't delete them, just disables them)
            debugContext->DisableBreakpoints();

            // test get value from globals, tables, function, and user class
            ScriptContextDebug::DebugValue values[9];
            bool getValueResult;

            // make sure we detect non existing variables
            values[0].m_name = "InvalidVariable";
            getValueResult = debugContext->GetValue(values[0]);
            AZ_TEST_ASSERT(getValueResult == false);

            // get global number
            values[0].m_name = "GlobalValueA";
            getValueResult = debugContext->GetValue(values[0]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[0].m_type == LUA_TNUMBER);
            AZ_TEST_ASSERT(values[0].m_value.find("6.0") != OSString::npos);

            // get global string
            values[1].m_name = "GlobalValueC";
            getValueResult = debugContext->GetValue(values[1]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[1].m_type == LUA_TSTRING);
            AZ_TEST_ASSERT(values[1].m_value == "GlobalTest");

            // get global table
            values[2].m_name = "GlobalTable";
            getValueResult = debugContext->GetValue(values[2]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[2].m_type == LUA_TTABLE);
            AZ_TEST_ASSERT(values[2].m_elements.size() == 5); // valueA,valueB,valueC,SubTable and TableFunction

            // get global table with a metatable
            values[3].m_name = "GlobalTableDerived";
            getValueResult = debugContext->GetValue(values[3]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[3].m_type == LUA_TTABLE);
            AZ_TEST_ASSERT(values[3].m_elements.size() == 1); // valueA,valueB,valueC,SubTable and TableFunction
            AZ_TEST_ASSERT(values[3].m_elements[0].m_elements.size() == 5); // valueA,valueB,valueC,SubTable and TableFunction

            // get global function
            values[4].m_name = "GlobalFunction";
            getValueResult = debugContext->GetValue(values[4]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[4].m_type == LUA_TFUNCTION);

            // get global user class
            script.Execute("sc = ScriptClass()"); // create a C++ class
            values[5].m_name = "sc";
            getValueResult = debugContext->GetValue(values[5]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[5].m_type == LUA_TUSERDATA);

            // check nested user classes
            script.Execute("sv = ScriptValueHolder()");
            values[6].m_name = "sv";
            getValueResult = debugContext->GetValue(values[6]);
            AZ_TEST_ASSERT(getValueResult);

            // get global property/field
            s_globalField = 23;
            values[7].m_name = "globalField";
            getValueResult = debugContext->GetValue(values[7]);
            AZ_TEST_ASSERT(getValueResult);
            AZ_TEST_ASSERT(values[7].m_value.find("23") != OSString::npos);

            // make sure we can get the global table safely (if we want to support cyclic tables)
            values[8].m_name = "_G";
            getValueResult = debugContext->GetValue(values[8]);
            AZ_TEST_ASSERT(getValueResult == true);

            ScriptContextDebug::DebugValue  valueCheck;
            // set a global number and change it's type to a string
            values[0].m_value = "BlaBla";
            values[0].m_type = LUA_TSTRING;
            debugContext->SetValue(values[0]);
            // check the SetValue
            valueCheck.m_name = values[0].m_name;
            debugContext->GetValue(valueCheck);
            AZ_TEST_ASSERT(valueCheck.m_type == LUA_TSTRING);
            AZ_TEST_ASSERT(valueCheck.m_value == "BlaBla");

            // set a global string
            values[1].m_value = "BlaBla1";
            debugContext->SetValue(values[1]);
            // check the set value
            valueCheck.m_name = values[1].m_name;
            debugContext->GetValue(valueCheck);
            AZ_TEST_ASSERT(valueCheck.m_value == "BlaBla1");

            // set a table value
            values[2].m_elements[0].m_value = "Aha";
            values[2].m_elements[0].m_type = LUA_TSTRING;
            debugContext->SetValue(values[2]);
            // check
            valueCheck.m_name = values[2].m_name;
            debugContext->GetValue(valueCheck);
            AZ_TEST_ASSERT(values[2].m_elements.size() == valueCheck.m_elements.size());

            // set a table value with metatable
            ScriptContextDebug::DebugValue& subValue = values[3].m_elements.emplace_back();
            subValue.m_name = "valueA";
            subValue.m_type = LUA_TNUMBER;
            subValue.m_value = "16";
            debugContext->SetValue(values[3]);
            // check
            valueCheck.m_name = values[3].m_name;
            debugContext->GetValue(valueCheck);
            AZ_TEST_ASSERT(values[3].m_elements.size() == valueCheck.m_elements.size());

            int data1Element = -1;
            for (size_t i = 0; i < values[5].m_elements.size(); ++i)
            {
                if (values[5].m_elements[i].m_name == "data1")
                {
                    data1Element = static_cast<int>(i);
                }
            }
            AZ_TEST_ASSERT(data1Element != -1); // make sure we have data 1 element

            // user class
            AZ_TEST_ASSERT(values[5].m_elements[data1Element].m_name == "data1");  // make sure we are modifying the correct element
            values[5].m_elements[data1Element].m_value = "6.0";
            debugContext->SetValue(values[5]);
            // check
            valueCheck.m_name = values[5].m_name;
            debugContext->GetValue(valueCheck);
            AZ_TEST_ASSERT(values[5].m_elements.size() == valueCheck.m_elements.size());
            AZ_TEST_ASSERT(valueCheck.m_elements[data1Element].m_value.find("6.0") != OSString::npos);

            // user nested class
            values[6].m_elements[0].m_elements[0].m_value = "3.0";
            debugContext->SetValue(values[6]);
            // check
            valueCheck.m_name = values[6].m_name;
            debugContext->GetValue(valueCheck);
            AZ_TEST_ASSERT(valueCheck.m_elements[0].m_elements[0].m_value.find("3.0") != OSString::npos);

            // global table
            // modify todo
            debugContext->SetValue(values[8]);
            // check (TODO)

            // disable debug support (this will delete all breakpoints and everything debug related!)
            script.DisableDebug();
        }
    };

    ScriptContext* ScriptDebugTest::s_currentSC = nullptr;

//     TEST_F(ScriptDebugTest, LuaTest)
//     {
//         run();
//     }

    //-----------------------------------------------------------------------------
    // Ebus script test
    //-----------------------------------------------------------------------------
    void TestAssert(bool check)
    {
        (void)check;
        AZ_Assert(!check, "Script Test assert");
    }

    class TestBusMessages
        : public AZ::EBusTraits
    {
    public:
        virtual ~TestBusMessages()  {}

        virtual void    Set() = 0;
        virtual void    SetNotImplemented() = 0;
        virtual void    SetSum1(int) = 0;
        virtual void    SetSum2(int, int) = 0;
        virtual void    SetSum3(int, int, int) = 0;
        virtual void    SetSum4(int, int, int, int) = 0;
        virtual void    SetSum5(int, int, int, int, int) = 0;
        virtual int     Get() = 0;
        virtual int     GetNotImplemented() = 0;
        virtual int     GetSum1(int) = 0;
        virtual int     GetSum2(int, int) = 0;
        virtual int     GetSum3(int, int, int) = 0;
        virtual int     GetSum4(int, int, int, int) = 0;
        virtual int     GetSum5(int, int, int, int, int) = 0;
    };
    typedef AZ::EBus<TestBusMessages> TestBus;

    class TestBusHandler
        : public TestBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(TestBusHandler, "{CD26E702-6F40-4FF9-816D-4DCB652D97DF}", AZ::SystemAllocator,
            Set, SetNotImplemented, SetSum1, SetSum2, SetSum3, SetSum4, SetSum5, Get, GetNotImplemented, GetSum1, GetSum2, GetSum3, GetSum4, GetSum5);

        void Set() override
        {
            Call(FN_Set);
        }
        void SetNotImplemented() override
        {
            Call(FN_SetNotImplemented);
        }
        void SetSum1(int d1) override
        {
            Call(FN_SetSum1, d1);
        }
        void SetSum2(int d1, int d2) override
        {
            Call(FN_SetSum2, d1, d2);
        }
        void SetSum3(int d1, int d2, int d3) override
        {
            Call(FN_SetSum3, d1, d2, d3);
        }
        void SetSum4(int d1, int d2, int d3, int d4) override
        {
            Call(FN_SetSum4, d1, d2, d3, d4);
        }
        void SetSum5(int d1, int d2, int d3, int d4, int d5) override
        {
            Call(FN_SetSum5, d1, d2, d3, d4, d5);
        }
        int Get() override
        {
            int result = 0;
            CallResult(result, FN_Get);
            return result;
        }
        int GetNotImplemented() override
        {
            int result = -1;
            CallResult(result, FN_GetNotImplemented);
            return result;
        }
        int GetSum1(int d1) override
        {
            int result = 0;
            CallResult(result, FN_GetSum1, d1);
            return result;
        }
        int GetSum2(int d1, int d2) override
        {
            int result = 0;
            CallResult(result, FN_GetSum2, d1, d2);
            return result;
        }
        int GetSum3(int d1, int d2, int d3) override
        {
            int result = 0;
            CallResult(result, FN_GetSum3, d1, d2, d3);
            return result;
        }
        int GetSum4(int d1, int d2, int d3, int d4) override
        {
            int result = 0;
            CallResult(result, FN_GetSum4, d1, d2, d3, d4);
            return result;
        }
        int GetSum5(int d1, int d2, int d3, int d4, int d5) override
        {
            int result = 0;
            CallResult(result, FN_GetSum5, d1, d2, d3, d4, d5);
            return result;
        }
    };

    //class TestBusSenderCheck : public TestBus::Handler
    //{
    //public:
    //    virtual void    Set()                                           { TestAssert(true); }
    //    virtual void    SetNotImplemented()                             { /*Not used*/ }
    //    virtual void    SetSum1(int _1)                                 { TestAssert(_1 == 1); }
    //    virtual void    SetSum2(int _1, int _2)                         { TestAssert(_1 + _2 == 3); }
    //    virtual void    SetSum3(int _1, int _2, int _3)                 { TestAssert(_1 + _2 + _3 == 6); }
    //    virtual void    SetSum4(int _1, int _2, int _3, int _4)         { TestAssert(_1 + _2 + _3 + _4 == 10); }
    //    virtual void    SetSum5(int _1, int _2, int _3, int _4, int _5) { TestAssert(_1 + _2 + _3 + _4 + _5 == 15); }
    //    virtual int     Get()                                           { return -1; }
    //    virtual int     GetNotImplemented()                             { return -2; /*Not used*/ }
    //    virtual int     GetSum1(int _1)                                 { return _1; }
    //    virtual int     GetSum2(int _1, int _2)                         { return _1 + _2; }
    //    virtual int     GetSum3(int _1, int _2, int _3)                 { return _1 + _2 + _3; }
    //    virtual int     GetSum4(int _1, int _2, int _3, int _4)         { return _1 + _2 + _3 + _4; }
    //    virtual int     GetSum5(int _1, int _2, int _3, int _4, int _5) { return _1 + _2 + _3 + _4 + _5; }
    //};

    //class MyInterface
    //{
    //public:
    //   // AZ_TYPE_INFO(MyInterface,"{96a83332-8cd1-4299-9972-e670be92ab99}");

    //    virtual int GetInt() const = 0;
    //};

   /* class MyImpl : public MyInterface
    {
    public:
        int GetInt() const override { return 5; }
    };*/

    //class MyRequests : public AZ::EBusTraits
    //{
    //public:
    //    virtual const MyInterface* GetIFace() = 0;
    //};
    //using MyRequestBus = AZ::EBus<MyRequests>;

    //AZ_SCRIPTABLE_EBUS(
    //    MyRequestBus,
    //    MyRequestBus,
    //    "{DD4261AD-416E-4824-97A8-EFEF0F54AD91}",
    //    "{9312EB0C-0717-4F91-97F8-30A946096E9B}",
    //    AZ_SCRIPTABLE_EBUS_EVENT_RESULT(const MyInterface*, nullptr, GetIFace)
    //    );

    class ScriptedBusTest
        : public LeakDetectionFixture {
    public:
        void run()
        {
            const char luaCode[] = "\
                                MyBusHandlerMetaTable1 = {\
                                    Set = function(self)\
                                        TestAssert(true)\
                                    end,\
                                    SetSum1 = function(self, _1)\
                                        TestAssert(_1 == 1)\
                                    end,\
                                    SetSum2 = function(self, _1, _2)\
                                        TestAssert(_1 + _2 == 3)\
                                    end,\
                                    SetSum3 = function(self, _1, _2, _3)\
                                        TestAssert(_1 + _2 + _3 == 6)\
                                    end,\
                                    SetSum4 = function(self, _1, _2, _3, _4)\
                                        TestAssert(_1 + _2 + _3 + _4 == 10)\
                                    end,\
                                    SetSum5 = function(self, _1, _2, _3, _4, _5)\
                                        TestAssert(_1 + _2 + _3 + _4 + _5 == 15)\
                                    end,\
                                    Get = function(self)\
                                        return -1\
                                    end,\
                                    GetSum1 = function(self, _1)\
                                        return _1\
                                    end,\
                                    GetSum2 = function(self, _1, _2)\
                                        return _1 + _2\
                                    end,\
                                    GetSum3 = function(self, _1, _2, _3)\
                                        return _1 + _2 + _3\
                                    end,\
                                    GetSum4 = function(self, _1, _2, _3, _4)\
                                        return _1 + _2 + _3 + _4\
                                    end,\
                                    GetSum5 = function(self, _1, _2, _3, _4, _5)\
                                        return _1 + _2 + _3 + _4 + _5\
                                    end\
                                }\
                                MyBusHandlerMetaTable2 = {\
                                    Get = function(self)\
                                        return -2\
                                    end\
                                }\
                                setmetatable(MyBusHandlerMetaTable2, MyBusHandlerMetaTable1)\
                            ";
            BehaviorContext behaviorContext;
            behaviorContext.Method("TestAssert", &TestAssert);
            behaviorContext.EBus<TestBus>("TestBus")->
                Handler<TestBusHandler>()->
                Event("Set", &TestBus::Events::Set)->
                Event("SetNotImplemented", &TestBus::Events::SetNotImplemented)->
                Event("SetSum1", &TestBus::Events::SetSum1)->
                Event("SetSum2", &TestBus::Events::SetSum2)->
                Event("SetSum3", &TestBus::Events::SetSum3)->
                Event("SetSum4", &TestBus::Events::SetSum4)->
                Event("SetSum5", &TestBus::Events::SetSum5)->
                Event("Get", &TestBus::Events::Get)->
                Event("GetNotImplemented", &TestBus::Events::GetNotImplemented)->
                Event("GetSum1", &TestBus::Events::GetSum1)->
                Event("GetSum2", &TestBus::Events::GetSum2)->
                Event("GetSum3", &TestBus::Events::GetSum3)->
                Event("GetSum4", &TestBus::Events::GetSum4)->
                Event("GetSum5", &TestBus::Events::GetSum5)
                ;

            ScriptContext script;
            script.BindTo(&behaviorContext);

            script.Execute(luaCode);

            // Test sending messages from C++ to LUA handler
            script.Execute("handler = TestBus.Connect(MyBusHandlerMetaTable1)");
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestBus::Broadcast(&TestBus::Events::Set);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            TestBus::Broadcast(&TestBus::Events::SetNotImplemented); // nothing should happen here
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestBus::Broadcast(&TestBus::Events::SetSum1, 1);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestBus::Broadcast(&TestBus::Events::SetSum2, 1, 2);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestBus::Broadcast(&TestBus::Events::SetSum3, 1, 2, 3);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestBus::Broadcast(&TestBus::Events::SetSum4, 1, 2, 3, 4);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestBus::Broadcast(&TestBus::Events::SetSum5, 1, 2, 3, 4, 5);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            script.Execute("handler:Disconnect()");

            script.Execute("handler = TestBus.Connect(MyBusHandlerMetaTable2)");
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.Set()");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.SetNotImplemented()");
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.SetSum1(1)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.SetSum2(1, 2)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.SetSum3(1, 2, 3)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.SetSum4(1, 2, 3, 4)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestBus.Broadcast.SetSum5(1, 2, 3, 4, 5)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.Get() == -2)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.GetNotImplemented() == -1)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.GetSum1(1) == 1)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.GetSum2(1, 2) == 3)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.GetSum3(1, 2, 3) == 6)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.GetSum4(1, 2, 3, 4) == 10)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestBus.Broadcast.GetSum5(1, 2, 3, 4, 5) == 15)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            script.Execute("handler:Disconnect()\
                            handler = nil");
        }
    };

    TEST_F(ScriptedBusTest, LuaTest)
    {
        run();
    }

    // RamenRequests
    class RamenRequests
        : public AZ::EBusTraits
    {
    public:
        typedef unsigned int BusIdType;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual void AddPepper() = 0;
    };
    using RamenRequestBus = AZ::EBus<RamenRequests>;

    class RamenRequestHandler
        : public RamenRequestBus::Handler
    {
    public:
        void AddPepper() override {  };
    };

    class RamenRequestBehaviorHandler
        : public RamenRequestBus::Handler
        , public BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(RamenRequestBehaviorHandler, "{EB4E043B-AD1A-4745-A725-91100E191517}", AZ::SystemAllocator, AddPepper);

        void AddPepper() override
        {
            Call(FN_AddPepper);
        }
    };
    // RamenRequests

    // RamenShopNotifications
    class RamenShopNotifications
        : public AZ::EBusTraits
    {
    public:
        typedef unsigned int BusIdType;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual void OnOrderCancelled(int id) = 0;
    };
    using RamenShopNotificationBus = AZ::EBus<RamenShopNotifications>;

    class RamenShopNotificationHandler
        : public RamenShopNotificationBus::MultiHandler
    {
    public:
        void OnOrderCancelled(int /*id*/) override {};
    };

    class RamenShopNotificationBehaviorHandler
        : public RamenShopNotificationBus::MultiHandler
        , public BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(RamenShopNotificationBehaviorHandler, "{98A0C1B1-0B81-4563-886A-03204DDBE146}", AZ::SystemAllocator, OnOrderCancelled);

        void OnOrderCancelled(int id) override
        {
            Call(FN_OnOrderCancelled, id);
        }
    };
    // RamenShopNotifications

    class ScriptBehaviorHandlerIsConnectedTest
        : public LeakDetectionFixture
    {
    public:
        static void TestAssert([[maybe_unused]] bool check)
        {
            AZ_Assert(check, "Script Test assert");
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_behaviorContext = aznew BehaviorContext();
            m_behaviorContext->Method("TestAssert", &ScriptBehaviorHandlerIsConnectedTest::TestAssert);

            m_behaviorContext->EBus<RamenRequestBus>("RamenRequestBus")->
                Handler<RamenRequestBehaviorHandler>()->
                    Event("AddPepper", &RamenRequestBus::Events::AddPepper);

            m_behaviorContext->EBus<RamenShopNotificationBus>("RamenShopNotificationBus")->
                Handler<RamenShopNotificationBehaviorHandler>()->
                    Event("OnOrderCancelled", &RamenShopNotificationBus::Events::OnOrderCancelled);

            m_scriptContext = aznew ScriptContext();
            m_scriptContext->BindTo(m_behaviorContext);

        }

        void TearDown() override
        {
            delete m_scriptContext;
            delete m_behaviorContext;

            LeakDetectionFixture::TearDown();
        }

        ScriptContext* m_scriptContext;
        BehaviorContext* m_behaviorContext;
    };

    TEST_F(ScriptBehaviorHandlerIsConnectedTest, LuaIsConnected_UberTest)
    {
            const char luaCode[] = R"(
ramen = {
    handler1 = nil,
    handler2 = nil
}
ramen.handler1 = RamenRequestBus.Connect(ramen, 1)
ramen.handler2 = RamenRequestBus.Connect(ramen, 2)

ramenShop = {
    handler = nil
}
ramenShop.handler = RamenShopNotificationBus.CreateHandler(ramenShop)
ramenShop.handler:Connect(3);
ramenShop.handler:Connect(4);
)";
        m_scriptContext->Execute(luaCode);

        m_scriptContext->Execute("TestAssert(ramen.handler1:IsConnected())");
        m_scriptContext->Execute("TestAssert(ramen.handler1:IsConnectedId(1))");
        m_scriptContext->Execute("TestAssert(ramen.handler2:IsConnectedId(2))");

        m_scriptContext->Execute("TestAssert(ramenShop.handler:IsConnected())");
        m_scriptContext->Execute("TestAssert(ramenShop.handler:IsConnectedId(3))");
        m_scriptContext->Execute("TestAssert(ramenShop.handler:IsConnectedId(4))");
    }

    //-----------------------------------------------------------------------------
    // Ebus script with bus id test
    //-----------------------------------------------------------------------------
    class TestIdBusMessages
        : public AZ::EBusTraits
    {
    public:
        typedef unsigned int BusIdType;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        virtual ~TestIdBusMessages()    {}

        virtual void    VoidFunc0() = 0;
        virtual float   Pick(float, float, float) = 0;
    };
    typedef AZ::EBus<TestIdBusMessages> TestIdBus;

    class TestIdBusCPPHandler
        : public TestIdBus::Handler
    {
    public:
        void    VoidFunc0() override                                 { TestAssert(true); }
        float   Pick(float /*_1*/, float /*_2*/, float _3) override  { return _3; }
    };

    //AZ_SCRIPTABLE_EBUS(
    //    TestIdBus,
    //    TestIdBus,
    //    "{2DACEBCC-CB6B-403A-9E91-4E7390F567C9}", // Handler type UUID
    //    "{EB13A5D0-D642-49E2-9D3C-DC15F96FD97A}", // Sender type UUID
    //    AZ_SCRIPTABLE_EBUS_EVENT(VoidFunc0)
    //    AZ_SCRIPTABLE_EBUS_EVENT_RESULT(float, 0.f, Pick, float, float, float)
    //    )

    class TestIdBusHandler
        : public TestIdBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(TestIdBusHandler, "{2DACEBCC-CB6B-403A-9E91-4E7390F567C9}", AZ::SystemAllocator,
            VoidFunc0, Pick);

        void VoidFunc0() override
        {
            Call(FN_VoidFunc0);
        }
        float Pick(float a, float b, float c) override
        {
            float result = 0.0f;
            CallResult(result, FN_Pick, a, b, c);
            return result;
        }
    };

    class ScriptedIdBusTest
        : public LeakDetectionFixture
    {
    public:
        void run()
        {
            const char luaCode[] = "\
                                   MyBusHandlerMetaTable1 = {\
                                        VoidFunc0 = function(self)\
                                            TestAssert(true)\
                                        end,\
                                        Pick = function(self, _1, _2, _3)\
                                            return _1\
                                        end\
                                   }\
                                   MyBusHandlerMetaTable2 = {\
                                        Pick = function(self, _1, _2, _3)\
                                            return _2\
                                        end\
                                   }\
                                   setmetatable(MyBusHandlerMetaTable2, MyBusHandlerMetaTable1)\
                                   handler1 = TestIdBus.Connect(MyBusHandlerMetaTable1, 1)\
                                   handler2 = TestIdBus.CreateHandler(MyBusHandlerMetaTable2)\
                                   handler2:Connect(2)\
                                   ";
            BehaviorContext behaviorContext;
            behaviorContext.Method("TestAssert", &TestAssert);
            behaviorContext.EBus<TestIdBus>("TestIdBus")->
                Handler<TestIdBusHandler>()->
                Event("VoidFunc0", &TestIdBus::Events::VoidFunc0)->
                Event("Pick", &TestIdBus::Events::Pick);

            ScriptContext script;
            script.BindTo(&behaviorContext);

            script.Execute(luaCode);

            // Test sending messages from C++ to two LUA handlers on different bus ids
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestIdBus::Event(1, &TestIdBus::Events::VoidFunc0);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            TestIdBus::Event(2, &TestIdBus::Events::VoidFunc0);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            float ret = 0.f;
            TestIdBus::EventResult(ret, 1, &TestIdBus::Events::Pick, 1.f, 2.f, 3.f);
            AZ_TEST_ASSERT(ret == 1.f);
            ret = 0;
            TestIdBus::EventResult(ret, 2, &TestIdBus::Events::Pick, 1.f, 2.f, 3.f);
            AZ_TEST_ASSERT(ret == 2.f);
            script.Execute("handler1:Disconnect()");
            script.Execute("handler2:Disconnect()");

            // Test sending messages from LUA to a LUA handler on id 1 and a C++ handler on id 3.
            script.Execute("handler = TestIdBus.Connect(MyBusHandlerMetaTable1, 1)");
            TestIdBusCPPHandler cppHandler;
            cppHandler.BusConnect(3);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestIdBus.Event.VoidFunc0(1)"); // send even on id=1
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestIdBus.Event.Pick(1, 1.0, 2.0, 3.0) == 1.0)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestIdBus.Event.VoidFunc0(3)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("TestAssert(TestIdBus.Event.Pick(3, 1.0, 2.0, 3.0) == 3.0)");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            script.Execute("handler:Disconnect()");

            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("handler:Connect('my fake bus id')");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            AZ_TEST_START_TRACE_SUPPRESSION;
            script.Execute("handler = TestIdBus.Connect({ }, 'my fake bus id')");
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            script.Execute("TestAssert(handler == nil)");
        }
    };

    TEST_F(ScriptedIdBusTest, LuaTest)
    {
        run();
    }

    void AZTestAssert(bool check)
    {
        (void)check;
        AZ_Assert(check, "Script Test assert");
    }

    class AnyScriptBindTest
        : public LeakDetectionFixture
    {
    public:
        struct DataContainer
        {
        public:
            AZ_TYPE_INFO(DataContainer, "{FC0B12F1-165F-4D0C-ABAE-E9F657370352}");

            DataContainer(int initData)
                : m_data(initData)
            { }

            int m_data;
        };

        static bool s_wasCalled;
        static void TestThatDataIs10(AZStd::any container)
        {
            ASSERT_TRUE(container.is<DataContainer>());
            ASSERT_EQ(10, AZStd::any_cast<DataContainer&>(container).m_data);
            s_wasCalled = true;
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            s_wasCalled = false;

            m_behavior = aznew BehaviorContext();

            m_behavior->Class<DataContainer>("DataContainer")
                ->Constructor<int>()
                ->Property("data", BehaviorValueProperty(&DataContainer::m_data))
                ;

            m_behavior->Method("TestThatDataIs10", &TestThatDataIs10);
            m_behavior->Property("wasCalled", BehaviorValueGetter(&s_wasCalled), nullptr);

            m_script = aznew ScriptContext();
            m_script->BindTo(m_behavior);
        }

        void TearDown() override
        {
            delete m_script;
            delete m_behavior;

            LeakDetectionFixture::TearDown();
        }

        BehaviorContext* m_behavior;
        ScriptContext* m_script;
    };
    bool AnyScriptBindTest::s_wasCalled = false;

    TEST_F(AnyScriptBindTest, LuaScriptContextAny_AnyFromLua)
    {
        m_script->Execute("TestThatDataIs10(DataContainer(10))");
        EXPECT_TRUE(s_wasCalled);
    }

    class BaseScriptTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_behavior = aznew BehaviorContext();
            m_behavior->Method("AZTestAssert", &AZTestAssert);
            SetupBehaviorContext(*m_behavior);

            m_script = aznew ScriptContext();
            m_script->BindTo(m_behavior);
        }

        void TearDown() override
        {
            delete m_script;
            delete m_behavior;
            LeakDetectionFixture::TearDown();
        }

        BehaviorContext* m_behavior;
        ScriptContext* m_script;

        virtual void SetupBehaviorContext(BehaviorContext& bc) { (void)bc; }
    };

    TEST_F(BaseScriptTest, LuaUnreflect)
    {
        m_behavior->Class<ScriptClass>();
        m_script->Execute("AZTestAssert(ScriptClass ~= nil)");

        m_behavior->EnableRemoveReflection();
        m_behavior->Class<ScriptClass>();
        m_behavior->DisableRemoveReflection();

        m_script->Execute("AZTestAssert(ScriptClass == nil)");
    }

    class MathScriptTest
        : public BaseScriptTest
    {
        static void ScriptAssertEqual(double lhs, double rhs)
        {
            EXPECT_TRUE(AZ::IsClose(lhs, rhs, static_cast<double>(AZ::Constants::FloatEpsilon)));
        }

        void SetupBehaviorContext(BehaviorContext& bc) override
        {
            MathReflect(&bc);
            bc.Method("ScriptAssertEqual", &ScriptAssertEqual);
        }
    };

    TEST_F(MathScriptTest, LuaScriptMath_BasicRandomTests)
    {
        // tests pseudo random number generator
        m_script->Execute(R"LUA(
        local r = Random();
        r = Random(4321);
        r:SetSeed(1212);
        ScriptAssertEqual(r:GetRandom(), 2783974736);
        ScriptAssertEqual(r:GetRandomFloat(), 0.61152875423431396);
        )LUA");
    }

    class ScriptTypeidTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_behavior = aznew BehaviorContext();

            AZ::MathReflect(m_behavior);

            m_behavior->Class<ScriptClass>();
            m_behavior->Method("AZTestAssert", &AZTestAssert);
            m_behavior->Method("CheckScriptClassTypeid", &CheckScriptTypeid<ScriptClass>);
            m_behavior->Method("CheckScriptBoolTypeid", &CheckScriptTypeid<bool>);
            m_behavior->Method("CheckScriptStringTypeid", &CheckScriptTypeid<AZStd::string>);
            m_behavior->Method("CheckScriptNumberTypeid", &CheckScriptTypeid<double>);
            m_behavior->Method("CheckScriptTypeidNullptr", &CheckScriptTypeidNullptr);

            m_script = aznew ScriptContext();
            m_script->BindTo(m_behavior);
        }

        void TearDown() override
        {
            delete m_script;
            delete m_behavior;

            LeakDetectionFixture::TearDown();
        }

        template <typename T>
        static void CheckScriptTypeid(const AZ::Uuid& typeId)
        {
            EXPECT_EQ(azrtti_typeid<T>(), typeId);
        }

        static void CheckScriptTypeidNullptr(const AZ::Uuid& typeId)
        {
            EXPECT_TRUE(typeId.IsNull());
        }

        BehaviorContext* m_behavior = nullptr;
        ScriptContext* m_script = nullptr;
    };

    TEST_F(ScriptTypeidTest, LuaTypeId_Class)
    {
        m_script->Execute("CheckScriptClassTypeid(typeid(ScriptClass))");
        m_script->Execute("local v = ScriptClass(); CheckScriptClassTypeid(typeid(v))");
    }

    TEST_F(ScriptTypeidTest, LuaTypeId_Bool)
    {
        m_script->Execute("CheckScriptBoolTypeid(typeid(true))");
    }

    TEST_F(ScriptTypeidTest, LuaTypeId_String)
    {
        m_script->Execute("CheckScriptStringTypeid(typeid(''))");
    }

    TEST_F(ScriptTypeidTest, LuaTypeId_Number)
    {
        m_script->Execute("CheckScriptNumberTypeid(typeid(1))");
    }

    TEST_F(ScriptTypeidTest, LuaTypeId_ArgMiscount)
    {
        m_script->Execute("CheckScriptTypeidNullptr(typeid())");
        m_script->Execute("CheckScriptTypeidNullptr(typeid(1, 2, 3))");
    }

    TEST_F(ScriptTypeidTest, LuaTypeId_UnhandledType)
    {
        m_script->Execute("CheckScriptTypeidNullptr(typeid(function() end))");
        m_script->Execute("CheckScriptTypeidNullptr(typeid({ }))");
    }

    class ScriptCacheTableTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_script = aznew ScriptContext();
            m_lua = m_script->NativeContext();
        }

        void TearDown() override
        {
            m_lua = nullptr;
            delete m_script;

            LeakDetectionFixture::TearDown();
        }

        ScriptContext* m_script = nullptr;
        lua_State* m_lua = nullptr;
    };

    TEST_F(ScriptCacheTableTest, LuaNilRef)
    {
        lua_pushnil(m_lua);
        EXPECT_EQ(LUA_REFNIL, Internal::LuaCacheValue(m_lua, -1));
        lua_pop(m_lua, 1);

        Internal::LuaLoadCached(m_lua, LUA_REFNIL);
        EXPECT_TRUE(lua_isnil(m_lua, -1));
        lua_pop(m_lua, 1);
    }

    TEST_F(ScriptCacheTableTest, LuaInsertElement_RetreiveElement_IsSame)
    {
        lua_pushliteral(m_lua, "TEST_STRING");
        int ref = Internal::LuaCacheValue(m_lua, -1);
        lua_pop(m_lua, 1);
        ASSERT_GT(ref, 0);

        Internal::LuaLoadCached(m_lua, ref);
        ASSERT_EQ(LUA_TSTRING, lua_type(m_lua, -1));
        EXPECT_STREQ("TEST_STRING", lua_tostring(m_lua, -1));
        lua_pop(m_lua, 1);

        Internal::LuaReleaseCached(m_lua, ref);
    }

    TEST_F(ScriptCacheTableTest, LuaInsertElements_RemoveFromMiddle_RemoveFromEnd)
    {
        int refs[6] = { };
        int removedRefs[] = { 2, 4, 5 };

        // Create a bunch of references
        lua_pushinteger(m_lua, 42);
        for (int& ref : refs)
        {
            ref = Internal::LuaCacheValue(m_lua, -1);
            ASSERT_GT(ref, 0);
        }

        // All refs should be valid at the beginning
        for (const int& ref : refs)
        {
            Internal::LuaLoadCached(m_lua, ref);
            ASSERT_FALSE(lua_isnil(m_lua, -1));
            EXPECT_EQ(42, lua_tointeger(m_lua, -1));
            lua_pop(m_lua, 1);
        }

        // Remove some refs
        for (const int& refIdx : removedRefs)
        {
            Internal::LuaReleaseCached(m_lua, refs[refIdx]);
            refs[refIdx] = LUA_NOREF;
        }

        // All non-removed refs here should be valid
        for (const int& ref : refs)
        {
            // Skip removed refs
            if (ref == LUA_NOREF)
            {
                continue;
            }

            Internal::LuaLoadCached(m_lua, ref);
            ASSERT_FALSE(lua_isnil(m_lua, -1));
            EXPECT_EQ(42, lua_tointeger(m_lua, -1));
            lua_pop(m_lua, 1);
        }

        // Reinsert the refs
        for (const int& refIdx : removedRefs)
        {
            refs[refIdx] = Internal::LuaCacheValue(m_lua, -1);
        }

        // All refs should be valid again
        for (const int& ref : refs)
        {
            Internal::LuaLoadCached(m_lua, ref);
            ASSERT_FALSE(lua_isnil(m_lua, -1));
            EXPECT_EQ(42, lua_tointeger(m_lua, -1));
            lua_pop(m_lua, 1);
        }

        for (int& ref : refs)
        {
            Internal::LuaReleaseCached(m_lua, ref);
            ref = LUA_NOREF;
        }

        // Get cache table to ensure it's reset
        lua_rawgeti(m_lua, LUA_REGISTRYINDEX, AZ_LUA_WEAK_CACHE_TABLE_REF);

        lua_rawgeti(m_lua, -1, 2); // Get first free list index
        EXPECT_EQ(3, (int)lua_tointeger(m_lua, -1)) << "Free list index does not point to first assignable element";
        lua_pop(m_lua, 1);

        lua_rawgeti(m_lua, -1, 3); // Get the first free element
        EXPECT_TRUE(lua_isnil(m_lua, -1)) << "First assignable element is not empty";
        lua_pop(m_lua, 1);

        EXPECT_EQ(2u, lua_rawlen(m_lua, -1)) << "Cache table contains more than just header";

        lua_pop(m_lua, 1);
    }

    class UnregisteredSharedPointerTest
        : public BehaviorContextFixture
    {
    public:
        struct UnregisteredType
        {
            AZ_CLASS_ALLOCATOR(UnregisteredType, SystemAllocator);
            AZ_TYPE_INFO(UnregisteredType, "{6C5C91A6-A9F6-470D-8841-179201A0D9E6}");
        };

        static AZStd::shared_ptr<UnregisteredType> GetPtr()
        {
            return AZStd::make_shared<UnregisteredType>();
        }

        static void TestIsPtrNullptr(AZStd::shared_ptr<UnregisteredType> ptr)
        {
            ASSERT_NE(nullptr, ptr);
        }

        void SetUp() override
        {
            BehaviorContextFixture::SetUp();

            m_behaviorContext->Method("AZTestAssert", &AZTestAssert);
            m_behaviorContext->Method("GetPtr", &GetPtr);
            m_behaviorContext->Method("TestIsPtrNullptr", &TestIsPtrNullptr);

            m_script = aznew ScriptContext();
            m_script->BindTo(m_behaviorContext);
        }

        void TearDown() override
        {
            delete m_script;

            BehaviorContextFixture::TearDown();
        }

        ScriptContext* m_script = nullptr;
    };

    TEST_F(UnregisteredSharedPointerTest, LuaTest)
    {
        // Test that the pointer remains valid
        m_script->Execute(R"LUA(
            instance = GetPtr();
            TestIsPtrNullptr(instance);
            AZTestAssert(instance:get() ~= nil);
        )LUA");

        ScriptContext::ErrorHook errorHook = [](ScriptContext*, ScriptContext::ErrorType error, const char*) {
            ASSERT_GT(error, ScriptContext::ErrorType::Log);
        };

        // Test that actually doing stuff with it alerts the user
        auto oldHook = m_script->GetErrorHook();
        m_script->SetErrorHook(errorHook);
        m_script->Execute(R"LUA(
            instance:DoThing();
        )LUA");
        m_script->SetErrorHook(oldHook);
    }
}


#endif // #if !defined(AZCORE_EXCLUDE_LUA)
