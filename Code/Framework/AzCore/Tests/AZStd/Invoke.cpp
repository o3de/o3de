/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/any.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/function/invoke.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string_view.h>

#include "UserTypes.h"

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // Fixtures

    struct InvokeNonCopyable
    {
        InvokeNonCopyable() = default;

    private:
        InvokeNonCopyable(const InvokeNonCopyable&) = delete;
        InvokeNonCopyable& operator=(const InvokeNonCopyable&) = delete;
    };

    struct InvokeTestStruct
    {
        explicit InvokeTestStruct(int num)
            : m_data(num)
        {}

        int IntResultIntParameter(int) { return m_data; }

        int& operator()(InvokeNonCopyable&&) & { return m_data; }
        const int& operator()(InvokeNonCopyable&&) const & { return m_data; }
        volatile int& operator()(InvokeNonCopyable&&) volatile & { return m_data; }
        const volatile int& operator()(InvokeNonCopyable&&) const volatile & { return m_data; }

        int&& operator()(InvokeNonCopyable&&) && { return AZStd::move(m_data); }
        const int&& operator()(InvokeNonCopyable&&) const && { return AZStd::move(m_data); }
        volatile int&& operator()(InvokeNonCopyable&&) volatile && { return AZStd::move(m_data); }
        const volatile int&& operator()(InvokeNonCopyable&&) const volatile && { return AZStd::move(m_data); }

        int m_data;
    };

    struct InvokeTestDerivedStruct : InvokeTestStruct
    {
        explicit InvokeTestDerivedStruct(int num)
            : InvokeTestStruct(num)
        {}
    };

    struct InvokeTestImplicitConstructor
    {
        InvokeTestImplicitConstructor(AZ::s32) {}
    };

    struct InvokeTestExplicitConstructor
    {
        explicit InvokeTestExplicitConstructor(AZ::s32) {}
    };

    struct InvokeTestDeletedS32Callable
    {
        bool operator()(InvokeTestStruct) { return false; }

    private:
        bool operator()(AZ::s32) = delete;
    };

    // Fixture for non-typed tests
    class InvocableTest
        : public LeakDetectionFixture
    {
    };

    TEST_F(InvocableTest, InvalidInvocableArgsTest)
    {
        using Func = int(InvokeTestStruct::*)(int);
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, Test, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<int>::value));
    }

    TEST_F(InvocableTest, MemberFunctionTest)
    {
        using Func = int(InvokeTestStruct::*)(int);
        using CLFunc = int(InvokeTestStruct::*)(int) const &;
        using RFunc = int(InvokeTestStruct::*)(int) &&;
        using CRFunc = int(InvokeTestStruct::*)(int) const &&;
        // Member functions require a "this" object in order to be invocable 
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<AZStd::decay_t<Func>>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<AZStd::decay_t<CLFunc>>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<AZStd::decay_t<RFunc>>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<AZStd::decay_t<CRFunc>>::value));
        // Bullet 1
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, InvokeTestStruct, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, InvokeTestStruct&&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, InvokeTestDerivedStruct, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, const InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, const InvokeTestStruct&&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, int, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, InvokeTestStruct, AZStd::string_view>::value));

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const InvokeTestDerivedStruct&, int>::value));

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<RFunc, InvokeTestStruct, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<RFunc, InvokeTestStruct&&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, const InvokeTestStruct&&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, const InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, InvokeTestStruct&, int>::value));

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CRFunc, InvokeTestStruct, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CRFunc, InvokeTestStruct&&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CRFunc, const InvokeTestStruct&&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, const InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, InvokeTestStruct&, int>::value));

        // Bullet 2
        using RefTest = AZStd::reference_wrapper<InvokeTestStruct>;
        using RefDerivedTest = AZStd::reference_wrapper<InvokeTestDerivedStruct>;
        using RefConstTest = AZStd::reference_wrapper<const InvokeTestStruct>;
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, RefTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, RefDerivedTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, const RefTest, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, RefConstTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, RefTest&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, RefDerivedTest&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, const RefTest&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, RefConstTest&, int>::value));

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, RefTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const RefTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const RefTest&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, RefTest&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, RefTest&&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const RefTest&&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const RefDerivedTest, int>::value));

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, RefConstTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, RefConstTest&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const RefConstTest, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, RefConstTest&&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const RefConstTest&&, int>::value));

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, RefTest, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, RefDerivedTest, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, RefConstTest, int>::value));

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, RefTest, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, RefDerivedTest, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, RefConstTest, int>::value));

        // Bullet 3
        using TestPtrType = InvokeTestStruct*;
        using DerivedTestPtrType = InvokeTestDerivedStruct*;
        using ConstTestPtrType = const InvokeTestStruct*;
        using UniqueTestPtrType = AZStd::unique_ptr<InvokeTestStruct>;
        using SharedTestPtrType = AZStd::shared_ptr<InvokeTestStruct>;

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, const TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, DerivedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, UniqueTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<Func, SharedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, ConstTestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<Func, ConstTestPtrType&&, int>::value));

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, const TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, DerivedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, UniqueTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, SharedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, ConstTestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<CLFunc, ConstTestPtrType&&, int>::value));

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, const TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, DerivedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, UniqueTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, SharedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, ConstTestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RFunc, ConstTestPtrType&&, int>::value));

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, const TestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, DerivedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, UniqueTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, SharedTestPtrType, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, ConstTestPtrType&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<CRFunc, ConstTestPtrType&&, int>::value));
    }

    TEST_F(InvocableTest, MemberObjectTest)
    {
        using MemberFn = int(InvokeTestStruct::*);
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<MemberFn>::value));
        // Bullet 4
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, InvokeTestStruct>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, InvokeTestStruct&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, const InvokeTestStruct&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, InvokeTestStruct&&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, const InvokeTestStruct&&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, InvokeTestDerivedStruct&>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<MemberFn, InvokeTestStruct, int>::value));
       
        // Bullet 5
        using RefTest = AZStd::reference_wrapper<InvokeTestStruct>;
        using RefDerivedTest = AZStd::reference_wrapper<InvokeTestDerivedStruct>;
        using RefConstTest = AZStd::reference_wrapper<const InvokeTestStruct>;
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, RefTest>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, RefTest&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, const RefTest&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, RefConstTest&&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, const RefConstTest&&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, RefDerivedTest&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, RefConstTest&>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<MemberFn, RefTest, float>::value));

        // Bullet 6
        using TestPtrType = InvokeTestStruct*;
        using DerivedTestPtrType = InvokeTestDerivedStruct;
        using ConstTestPtrType = const InvokeTestStruct*;
        using UniqueTestPtrType = AZStd::unique_ptr<InvokeTestStruct>;
        using SharedTestPtrType = AZStd::shared_ptr<InvokeTestStruct>;

        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, TestPtrType>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, TestPtrType&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, const TestPtrType&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, TestPtrType&&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, DerivedTestPtrType&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, ConstTestPtrType&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, SharedTestPtrType&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<MemberFn, UniqueTestPtrType&>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<MemberFn, TestPtrType, float>::value));
    }

    TEST_F(InvocableTest, FunctionObjectTest)
    {
        using RawFuncPtr = AZStd::string_view(*)(InvokeTestStruct, int);
        using RawFuncRef = AZStd::string_view(&)(InvokeTestStruct&, int);
        using FuncObject = InvokeTestDeletedS32Callable;
        using StdFunctionObject = AZStd::function<int(InvokeTestStruct&&)>;

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RawFuncPtr>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RawFuncPtr, InvokeTestStruct&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<RawFuncPtr, InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<RawFuncPtr, InvokeTestDerivedStruct&, int>::value));
        
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RawFuncRef>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<RawFuncRef, InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<RawFuncRef, const InvokeTestStruct&, int>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<RawFuncRef, InvokeTestDerivedStruct&, int>::value));

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<FuncObject, AZ::s32>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<FuncObject, InvokeTestStruct>::value));

        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable<StdFunctionObject, InvokeTestStruct&>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable<StdFunctionObject, InvokeTestStruct&&>::value));
    }

    TEST_F(InvocableTest, Invocable_R_Test)
    {
        using Func = int(*)();
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable_r<int, Func>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable_r<double, Func>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable_r<const volatile void, Func>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_invocable_r<InvokeTestImplicitConstructor, Func>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable_r<InvokeTestExplicitConstructor, Func>::value));
        AZ_TEST_STATIC_ASSERT((!AZStd::is_invocable_r<InvokeTestStruct, Func>::value));
    }

    class InvokeTest
        : public LeakDetectionFixture
    {
    protected:
        static int RawIntFunc(int num)
        {
            return num;
        }

        static int DoubleRValueIntValue(int&& num)
        {
            return num * 2;
        }

    public:
        static const int s_rawFuncResult;
    };

    const int InvokeTest::s_rawFuncResult = 24;

    template<typename FuncSig, typename ExpectResultType, typename Functor>
    void InvokeMemberFunctionTester(Functor&& functor, int expectResult)
    {
        using MemberFunc = FuncSig;
        MemberFunc memberFunc = &InvokeTestStruct::operator();

        InvokeNonCopyable nonCopyableArg;

        using DeducedResultType = decltype(AZStd::invoke(memberFunc, AZStd::forward<Functor>(functor), AZStd::move(nonCopyableArg)));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<ExpectResultType, DeducedResultType>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<ExpectResultType, AZStd::invoke_result_t<MemberFunc, Functor, InvokeNonCopyable&&>>::value));
        
        auto result = AZStd::invoke(memberFunc, AZStd::forward<Functor>(functor), AZStd::move(nonCopyableArg));
        EXPECT_EQ(expectResult, result);
    };

    template<typename ExpectResultType, typename Functor>
    void InvokeMemberObjectTester(Functor&& functor, int expectResult)
    {
        auto memberObjPtr = &InvokeTestStruct::m_data;

        using DeducedResultType = decltype(AZStd::invoke(memberObjPtr, AZStd::forward<Functor>(functor)));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<ExpectResultType, DeducedResultType>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<ExpectResultType, AZStd::invoke_result_t<decltype(memberObjPtr), Functor>>::value));

        auto result = AZStd::invoke(memberObjPtr, AZStd::forward<Functor>(functor));
        EXPECT_EQ(expectResult, result);
    };

    template<typename ExpectResultType, typename Functor>
    void InvokeFunctionObjectTester(Functor&& functor, int expectResult)
    {
        InvokeNonCopyable nonCopyableArg;

        using DeducedResultType = decltype(AZStd::invoke(AZStd::forward<Functor>(functor), AZStd::move(nonCopyableArg)));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<ExpectResultType, DeducedResultType>::value));
        AZ_TEST_STATIC_ASSERT((AZStd::is_same<ExpectResultType, AZStd::invoke_result_t<Functor, InvokeNonCopyable&&>>::value));

        auto result = AZStd::invoke(AZStd::forward<Functor>(functor), AZStd::move(nonCopyableArg));
        EXPECT_EQ(expectResult, result);
    };

    int InvokeRawFunc(InvokeNonCopyable&&)
    {
        return InvokeTest::s_rawFuncResult;
    }

    TEST_F(InvokeTest, MemberFunctionTest)
    {
        {
            // Bullet 1
            {
                InvokeTestStruct test(1);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(test, test.m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(test, test.m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(test, test.m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(test, test.m_data);

                InvokeMemberFunctionTester<int&& (InvokeTestStruct::*)(InvokeNonCopyable&&) && , int&&>(AZStd::move(test), test.m_data);
                InvokeMemberFunctionTester<const int&& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &&, const int&&>(AZStd::move(test), test.m_data);
                InvokeMemberFunctionTester<volatile int&& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &&, volatile int&&>(AZStd::move(test), test.m_data);
                InvokeMemberFunctionTester<const volatile int&& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &&, const volatile int&&>(AZStd::move(test), test.m_data);
            }
            {

                InvokeTestDerivedStruct derivedTest(2);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(derivedTest, derivedTest.m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(derivedTest, derivedTest.m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(derivedTest, derivedTest.m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(derivedTest, derivedTest.m_data);

                using MemberDerivedRFunc = int&& (InvokeTestDerivedStruct::*)(InvokeNonCopyable&&) &&;
                using MemberDerivedCRFunc = const int&& (InvokeTestDerivedStruct::*)(InvokeNonCopyable&&) const &&;
                using MemberDerivedVRFunc = volatile int&& (InvokeTestDerivedStruct::*)(InvokeNonCopyable&&) volatile&&;
                using MemberDerivedCVRFunc = const volatile int&& (InvokeTestDerivedStruct::*)(InvokeNonCopyable&&) const volatile&&;
                InvokeMemberFunctionTester<MemberDerivedRFunc, int&&>(AZStd::move(derivedTest), derivedTest.m_data);
                InvokeMemberFunctionTester<MemberDerivedCRFunc, const int&&>(AZStd::move(derivedTest), derivedTest.m_data);
                InvokeMemberFunctionTester<MemberDerivedVRFunc, volatile int&&>(AZStd::move(derivedTest), derivedTest.m_data);
                InvokeMemberFunctionTester<MemberDerivedCVRFunc, const volatile int&&>(AZStd::move(derivedTest), derivedTest.m_data);
            }
        }

        {
            // Bullet 2
            {
                InvokeTestStruct testObj(3);
                AZStd::reference_wrapper<InvokeTestStruct> test(testObj);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(test, test.get().m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(test, test.get().m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(test, test.get().m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(test, test.get().m_data);

                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) & , int&>(AZStd::move(test), test.get().m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(AZStd::move(test), test.get().m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(AZStd::move(test), test.get().m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(AZStd::move(test), test.get().m_data);
            }
            {
                InvokeTestDerivedStruct derivedTestObj(4);
                AZStd::reference_wrapper<InvokeTestDerivedStruct> derivedTest(derivedTestObj);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(derivedTest, derivedTest.get().m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(derivedTest, derivedTest.get().m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(derivedTest, derivedTest.get().m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(derivedTest, derivedTest.get().m_data);

                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) & , int&>(AZStd::move(derivedTest), derivedTest.get().m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(AZStd::move(derivedTest), derivedTest.get().m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(AZStd::move(derivedTest), derivedTest.get().m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(AZStd::move(derivedTest), derivedTest.get().m_data);
            }
        }

        {
            // Bullet 3
            {
                InvokeTestStruct testObj(5);
                InvokeTestStruct* test(&testObj);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(test, test->m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(test, test->m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(test, test->m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(test, test->m_data);

                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) & , int&>(AZStd::move(test), test->m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(AZStd::move(test), test->m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(AZStd::move(test), test->m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(AZStd::move(test), test->m_data);
                
                AZStd::unique_ptr<InvokeTestStruct> testUniquePtr(&testObj);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(test, test->m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(test, test->m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(test, test->m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(test, test->m_data);
                testUniquePtr.release();
            }
            {
                InvokeTestDerivedStruct derivedTestObj(6);
                InvokeTestDerivedStruct* derivedTest(&derivedTestObj);
                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) &, int&>(derivedTest, derivedTest->m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(derivedTest, derivedTest->m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(derivedTest, derivedTest->m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(derivedTest, derivedTest->m_data);

                InvokeMemberFunctionTester<int& (InvokeTestStruct::*)(InvokeNonCopyable&&) & , int&>(AZStd::move(derivedTest), derivedTest->m_data);
                InvokeMemberFunctionTester<const int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const &, const int&>(AZStd::move(derivedTest), derivedTest->m_data);
                InvokeMemberFunctionTester<volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) volatile &, volatile int&>(AZStd::move(derivedTest), derivedTest->m_data);
                InvokeMemberFunctionTester<const volatile int& (InvokeTestStruct::*)(InvokeNonCopyable&&) const volatile &, const volatile int&>(AZStd::move(derivedTest), derivedTest->m_data);
            }
        }
    }

    TEST_F(InvokeTest, MemberObjectTest)
    {
        {
            // Bullet 4
            {
                using TestStruct = InvokeTestStruct;
                TestStruct test(7);
                InvokeMemberObjectTester<int&>(test, test.m_data);
                InvokeMemberObjectTester<const int&>(static_cast<const TestStruct&>(test), test.m_data);
                InvokeMemberObjectTester<volatile int&>(static_cast<volatile TestStruct&>(test), test.m_data);
                InvokeMemberObjectTester<const volatile int&>(static_cast<const volatile TestStruct&>(test), test.m_data);

                InvokeMemberObjectTester<int&&>(static_cast<TestStruct&&>(test), test.m_data);
                InvokeMemberObjectTester<const int&&>(static_cast<const TestStruct&&>(test), test.m_data);
                InvokeMemberObjectTester<volatile int&&>(static_cast<volatile TestStruct&&>(test), test.m_data);
                InvokeMemberObjectTester<const volatile int&&>(static_cast<const volatile TestStruct&&>(test), test.m_data);
            }
            {
                using TestStruct = InvokeTestDerivedStruct;
                TestStruct test(8);
                InvokeMemberObjectTester<int&>(test, test.m_data);
                InvokeMemberObjectTester<const int&>(static_cast<const TestStruct&>(test), test.m_data);
                InvokeMemberObjectTester<volatile int&>(static_cast<volatile TestStruct&>(test), test.m_data);
                InvokeMemberObjectTester<const volatile int&>(static_cast<const volatile TestStruct&>(test), test.m_data);

                InvokeMemberObjectTester<int&&>(static_cast<TestStruct&&>(test), test.m_data);
                InvokeMemberObjectTester<const int&&>(static_cast<const TestStruct&&>(test), test.m_data);
                InvokeMemberObjectTester<volatile int&&>(static_cast<volatile TestStruct&&>(test), test.m_data);
                InvokeMemberObjectTester<const volatile int&&>(static_cast<const volatile TestStruct&&>(test), test.m_data);
            }
        }

        {
            // Bullet 5
            {
                using TestStruct = InvokeTestStruct;
                TestStruct testObj(9);
                InvokeMemberObjectTester<int&>(AZStd::reference_wrapper<TestStruct>(testObj), testObj.m_data);
                InvokeMemberObjectTester<const int&>(AZStd::reference_wrapper<const TestStruct>(testObj), testObj.m_data);
                InvokeMemberObjectTester<volatile int&>(AZStd::reference_wrapper<volatile TestStruct>(testObj), testObj.m_data);
                InvokeMemberObjectTester<const volatile int&>(AZStd::reference_wrapper<const volatile TestStruct>(testObj), testObj.m_data);
            }
            {
                using TestStruct = InvokeTestDerivedStruct;
                TestStruct testObj(10);
                InvokeMemberObjectTester<int&>(AZStd::reference_wrapper<TestStruct>(testObj), testObj.m_data);
                InvokeMemberObjectTester<const int&>(AZStd::reference_wrapper<const TestStruct>(testObj), testObj.m_data);
                InvokeMemberObjectTester<volatile int&>(AZStd::reference_wrapper<volatile TestStruct>(testObj), testObj.m_data);
                InvokeMemberObjectTester<const volatile int&>(AZStd::reference_wrapper<const volatile TestStruct>(testObj), testObj.m_data);
            }
        }

        {
            // Bullet 6
            {
                using TestStruct = InvokeTestStruct;
                TestStruct testObj(11);
                TestStruct* test(&testObj);
                const TestStruct* cTest(&testObj);
                volatile TestStruct* vTest(&testObj);
                const volatile TestStruct* cvTest(&testObj);
                InvokeMemberObjectTester<int&>(test, test->m_data);
                InvokeMemberObjectTester<const int&>(cTest, test->m_data);
                InvokeMemberObjectTester<volatile int&>(vTest, test->m_data);
                InvokeMemberObjectTester<const volatile int&>(cvTest, test->m_data);
            }
            {
                using TestStruct = InvokeTestDerivedStruct;
                TestStruct testObj(12);
                TestStruct* test(&testObj);
                InvokeMemberObjectTester<int&>(test, test->m_data);
                InvokeMemberObjectTester<const int&>(static_cast<const TestStruct*>(test), test->m_data);
                InvokeMemberObjectTester<volatile int&>(static_cast<volatile TestStruct*>(test), test->m_data);
                InvokeMemberObjectTester<const volatile int&>(static_cast<const volatile TestStruct*>(test), test->m_data);
            }
        }
    }

    TEST_F(InvokeTest, FunctionObjectTest)
    {
        // Bullet 7
        using RawFuncPtr = int(*)(InvokeNonCopyable&&);
        using RawFuncRef = int(&)(InvokeNonCopyable&&);
        RawFuncPtr testRawFuncPtr = &InvokeRawFunc;
        RawFuncRef testRawFuncRef = InvokeRawFunc;
        InvokeFunctionObjectTester<int>(testRawFuncPtr, InvokeTest::s_rawFuncResult);
        InvokeFunctionObjectTester<int>(testRawFuncRef, InvokeTest::s_rawFuncResult);
        
        AZStd::function<int(int)> testStdFunc = &RawIntFunc;
        int numResult = AZStd::invoke(testStdFunc, InvokeTest::s_rawFuncResult);
        EXPECT_EQ(InvokeTest::s_rawFuncResult, numResult);

        AZStd::function<int(int&&)> testStdFuncWithRValueParam = &DoubleRValueIntValue;
        numResult = AZStd::invoke(testStdFuncWithRValueParam, 520);
        EXPECT_EQ(1040, numResult);
        
        InvokeTestStruct testFunctor(13);
        InvokeFunctionObjectTester<int&>(testFunctor, testFunctor.m_data);
        InvokeFunctionObjectTester<const int&>(static_cast<const InvokeTestStruct&>(testFunctor), testFunctor.m_data);
        InvokeFunctionObjectTester<volatile int&>(static_cast<volatile InvokeTestStruct&>(testFunctor), testFunctor.m_data);
        InvokeFunctionObjectTester<const volatile int&>(static_cast<const volatile InvokeTestStruct&>(testFunctor), testFunctor.m_data);
        InvokeFunctionObjectTester<int&&>(static_cast<InvokeTestStruct&&>(testFunctor), testFunctor.m_data);
        InvokeFunctionObjectTester<const int&&>(static_cast<const InvokeTestStruct&&>(testFunctor), testFunctor.m_data);
        InvokeFunctionObjectTester<volatile int&&>(static_cast<volatile InvokeTestStruct&&>(testFunctor), testFunctor.m_data);
        InvokeFunctionObjectTester<const volatile int&&>(static_cast<const volatile InvokeTestStruct&&>(testFunctor), testFunctor.m_data);
    }
}
