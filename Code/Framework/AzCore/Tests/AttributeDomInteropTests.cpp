/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/UnitTest/MockComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <DOM/DomFixtures.h>

namespace AZ::AttributeDomInteropTests
{
    template<typename FunctionSignature>
    struct InvokeTestHelper;

    // Helper for testing DOM invocation with the three attribute types that support invocation:
    // AttributeInvocable, AttributeFunction, and AttributeMemberFunction
    // Only one InvokeTestHelper of a given type should be instantiated as a static instance pointer is used
    // to provide a function pointer for AttributeFunction.
    template<typename R, typename... Args>
    struct InvokeTestHelper<R(Args...)>
    {
        AZ_TYPE_INFO(InvokeTestHelper, "{BE273EAA-FB6A-464F-A9F8-8A5C59650D70}", R(Args...));
        using FunctionType = AZStd::function<R(Args...)>;
        FunctionType m_fn;

        template<class F>
        InvokeTestHelper(F fn)
            : m_fn(AZStd::move(fn))
        {
            s_instance = this;
        }

        ~InvokeTestHelper() = default;

        inline static InvokeTestHelper* s_instance = nullptr;
        static R GlobalFunction(Args... args)
        {
            return s_instance->m_fn(args...);
        }

        R MemberFunction(Args... args)
        {
            return m_fn(args...);
        }

        void TestInvokeWithDomValues(AZ::Dom::Value expectedResult, AZ::Dom::Value args, bool invokeExpectedToWork = true)
        {
            // The invocable method must accept the class instance as the first argument
            auto invocableClassMethod = [](InvokeTestHelper* self, Args... args) -> R
            {
                return self->m_fn(args...);
            };
            AZ::AttributeInvocable invokeAttribute(invocableClassMethod);
            AZ::AttributeFunction<R(Args...)> functionAttribute(&GlobalFunction);
            AZ::AttributeMemberFunction<R (InvokeTestHelper::*)(Args...)> memberFunctionAttribute(&InvokeTestHelper::MemberFunction);
            AZStd::array<AZ::Attribute*, 3> attributesToTest = { &invokeAttribute, &functionAttribute, &memberFunctionAttribute };

            AZ::Dom::Value instanceAndArgs(AZ::Dom::Type::Array);
            instanceAndArgs.ArrayPushBack(AZ::Dom::Utils::ValueFromType(this));
            instanceAndArgs.ArrayInsert(instanceAndArgs.ArrayEnd(), args.ArrayBegin(), args.ArrayEnd());

            for (AZ::Attribute* attribute : attributesToTest)
            {
                EXPECT_EQ(true, attribute->IsInvokable());
                EXPECT_EQ(invokeExpectedToWork, attribute->CanDomInvoke(instanceAndArgs));
                // Move the arguments into an instanceAndArgs array and prepend the instance to the dom array
                EXPECT_TRUE(AZ::Dom::Utils::DeepCompareIsEqual(expectedResult, attribute->DomInvoke(instanceAndArgs)));
            }
        }

        template<typename Result = R>
        void TestInvoke(Result expectedResult, Args... args)
        {
            AZ::Dom::Value domArgs(AZ::Dom::Type::Array);
            (domArgs.ArrayPushBack(AZ::Dom::Utils::ValueFromType<Args>(args)), ...);
            TestInvokeWithDomValues(AZ::Dom::Utils::ValueFromType(expectedResult), AZStd::move(domArgs));
        }
    };

    class AttributeDomInteropTests
        : public AZ::Dom::Tests::DomTestFixture
    {
    public:
        AttributeDomInteropTests()
            : m_serializeContext(AZStd::make_unique<AZ::SerializeContext>())
            , m_jsonRegistrationContext(AZStd::make_unique<AZ::JsonRegistrationContext>())
        {
            m_componentApplicationMock = AZStd::make_unique<::testing::NiceMock<UnitTest::MockComponentApplication>>();
            ON_CALL(*m_componentApplicationMock.get(), GetSerializeContext())
                .WillByDefault(::testing::Return(m_serializeContext.get()));

            ON_CALL(*m_componentApplicationMock.get(), GetJsonRegistrationContext())
                .WillByDefault(::testing::Return(m_jsonRegistrationContext.get()));
        }

        ~AttributeDomInteropTests()
        {
            m_componentApplicationMock.reset();
            m_serializeContext.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<::testing::NiceMock<UnitTest::MockComponentApplication>> m_componentApplicationMock;
    };

    TEST_F(AttributeDomInteropTests, ArgumentHandling)
    {
        InvokeTestHelper<int(int, int)> tester(
            [](int a, int b)
            {
                return a + b;
            });
        tester.TestInvoke(5, 2, 3);
        tester.TestInvoke(-1, 2, -3);

        AZ::Dom::Value domArgs(AZ::Dom::Type::Array);

        // Insufficient args should skip invoking return null and CanDomInvoke should return false
        domArgs.ArrayPushBack(AZ::Dom::Value(2));
        tester.TestInvokeWithDomValues(AZ::Dom::Value(), domArgs, false);

        // Correct number of args should work
        domArgs.ArrayPushBack(AZ::Dom::Value(2));
        tester.TestInvokeWithDomValues(AZ::Dom::Value(4), domArgs);

        // Additional arguments should be discarded without failing the invocation
        domArgs.ArrayPushBack(AZ::Dom::Value(2));
        tester.TestInvokeWithDomValues(AZ::Dom::Value(4), domArgs);

        // Invalid argument signatures should also flag as not able to invoke
        domArgs.ClearArray();
        domArgs.ArrayPushBack(AZ::Dom::Value("string1", false));
        domArgs.ArrayPushBack(AZ::Dom::Value("string2", false));
        // the result will be 0 even though it's an invalid call due to a safety feature that defaults arguments
        // to their default constructed contents when receiving invalid data
        tester.TestInvokeWithDomValues(AZ::Dom::Value(0), domArgs, false);
    }

    TEST_F(AttributeDomInteropTests, NoReturnValue)
    {
        int callCount = 0;
        InvokeTestHelper<void()> tester(
            [&callCount]()
            {
                ++callCount;
            });

        // We should return null, but 3 invocations should have occurred (one for each tested attribute type)
        tester.TestInvoke(AZ::Dom::Value());
        EXPECT_EQ(3, callCount);
    }

    TEST_F(AttributeDomInteropTests, ArgumentOrder)
    {
        InvokeTestHelper<bool(int, float, AZStd::string)> tester(
            [](int i, float f, AZStd::string str)
            {
                return i > 0 && f > 0.f && !str.empty();
            });
        tester.TestInvoke(true, 1, 2.f, "test");
    }

    TEST_F(AttributeDomInteropTests, PointerArguments)
    {
        InvokeTestHelper<float(AZ::Vector3*)> tester(
            [](AZ::Vector3* arg)
            {
                return arg->GetX();
            });

        AZ::Vector3 arg;
        arg.SetX(0.f);
        tester.TestInvoke(0.f, &arg);
        arg.SetX(12.2f);
        tester.TestInvoke(12.2f, &arg);
    }

    TEST_F(AttributeDomInteropTests, RefArguments)
    {
        int callCount = 0;
        InvokeTestHelper<float(AZ::Vector3&, int&)> tester(
            [](AZ::Vector3& arg1, int& arg2)
            {
                ++arg2;
                arg1.SetX(1.f);
                return arg1.GetX();
            });

        AZ::Vector3 arg;
        arg.SetX(0.f);
        tester.TestInvoke(1.f, arg, callCount);
        EXPECT_EQ(1.0, arg.GetX());
        EXPECT_EQ(3, callCount);
        arg.SetX(12.2f);
        tester.TestInvoke(1.f, arg, callCount);
        EXPECT_EQ(1.0, arg.GetX());
        EXPECT_EQ(6, callCount);
    }

    TEST_F(AttributeDomInteropTests, ConstRefArguments)
    {
        InvokeTestHelper<float(const AZ::Vector3&, const int&)> tester(
            [](const AZ::Vector3& arg, const int& n)
            {
                return arg.GetX() + n;
            });

        AZ::Vector3 arg;
        int n = 1;
        arg.SetX(0.f);
        tester.TestInvoke(1.f, arg, n);
        arg.SetX(12.2f);
        tester.TestInvoke(13.2f, arg, n);
    }
} // namespace AZ::AttributeDomInteropTests
