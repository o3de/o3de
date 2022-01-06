/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/functional.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/std/delegate/delegate_bind.h>
#include <AzCore/std/string/string.h>

#include <AzCore/Memory/SystemAllocator.h>

using namespace AZStd;
using namespace AZStd::placeholders;
using namespace UnitTestInternal;

int global_int;

struct write_five_obj
{
    void operator()() const
    {
        global_int = 5;
    }
};
struct write_three_obj
{
    int operator()() const
    {
        global_int = 3;
        return 7;
    }
};
static void write_five() { global_int = 5; }
static void write_three() { global_int = 3; }
struct generate_five_obj
{
    int operator()() const
    {
        return 5;
    }
};
struct generate_three_obj
{
    int operator()() const
    {
        return 3;
    }
};
static int generate_five() { return 5; }
static int generate_three() { return 3; }
static AZStd::string identity_str(const AZStd::string& s) { return s; }
static AZStd::string string_cat(const AZStd::string& s1, const AZStd::string& s2) { return s1 + s2; }
static int sum_ints(int x, int y) { return x + y; }

struct write_const_1_nonconst_2
{
    void operator()() { global_int = 2; }
    void operator()() const { global_int = 1; }
};

struct add_to_obj
{
    add_to_obj(int v)
        : value(v) {}

    int operator()(int x) const { return value + x; }

    int value;
};

long f_0()
{
    return 17041L;
}

long f_1(long a)
{
    return a;
}

long f_2(long a, long b)
{
    return a + 10 * b;
}

long f_3(long a, long b, long c)
{
    return a + 10 * b + 100 * c;
}

long f_4(long a, long b, long c, long d)
{
    return a + 10 * b + 100 * c + 1000 * d;
}

long f_5(long a, long b, long c, long d, long e)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

long f_6(long a, long b, long c, long d, long e, long f)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

long f_7(long a, long b, long c, long d, long e, long f, long g)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

long f_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

long f_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
}

long global_result;

void fv_0()
{
    global_result = 17041L;
}

void fv_1(long a)
{
    global_result = a;
}

void fv_2(long a, long b)
{
    global_result = a + 10 * b;
}

void fv_3(long a, long b, long c)
{
    global_result = a + 10 * b + 100 * c;
}

void fv_4(long a, long b, long c, long d)
{
    global_result = a + 10 * b + 100 * c + 1000 * d;
}

void fv_5(long a, long b, long c, long d, long e)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

void fv_6(long a, long b, long c, long d, long e, long f)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

void fv_7(long a, long b, long c, long d, long e, long f, long g)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

void fv_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

void fv_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h + 100000000 * i;
}
void SimpleStaticFunction(int num, char* str) { global_int = num; (void)str; }
void SimpleVoidFunction() { global_int = -100; }

class BaseClass
{
protected:
    const char* m_name;
public:
    BaseClass(const char* name)
        : m_name(name) {};
    virtual ~BaseClass() {}
    void SimpleMemberFunction(int num, char* str)
    {
        global_int = num + 1;
        (void)str;
    }
    int SimpleMemberFunctionReturnsInt(int num, char* str)
    {
        global_int = num + 2;
        (void)str;
        return -1;
    }
    void ConstMemberFunction(int num, char* str) const
    {
        global_int = num + 3;
        (void)str;
    }
    virtual void SimpleVirtualFunction(int num, char* str)
    {
        global_int = num + 4;
        (void)str;
    }
    static void StaticMemberFunction(int num, char* str)
    {
        global_int = num + 5;
        (void)str;
    }
};

class OtherClass
{
    double rubbish; // to ensure this class has non-zero size.
public:
    virtual ~OtherClass() {}
    virtual void UnusedVirtualFunction() { (void)rubbish; }
    virtual void TrickyVirtualFunction(int num, char* str) = 0;
};

class VeryBigClass
{
    void UnusedFunction() { (void)letsMakeThingsComplicated; }
    int letsMakeThingsComplicated[400];
};

// This declaration ensures that we get a convoluted class heirarchy.
class DerivedClass
    : public VeryBigClass
    , virtual public OtherClass
    , virtual public BaseClass
{
    double m_somemember[8];
public:
    DerivedClass()
        : BaseClass("Base of Derived") { m_somemember[0] = 1.2345; }
    ~DerivedClass() override {}
    void SimpleDerivedFunction(int num, char* str) { global_int = num + 6; (void)str;  }
    virtual void AnotherUnusedVirtualFunction(int num, char* str) { global_int = num + 7; (void)str; }
    void TrickyVirtualFunction(int num, char* str) override { global_int = num + 8; (void)str; }
};

namespace UnitTest
{
    /**
     * Function
     * We use tuned version of the boost::function (which is in TR1), so we use the boost::function tests too
     */
    class Function
        : public AllocatorsFixture
    {
    public:
        void test_zero_args()
        {
            typedef function<void ()> func_void_type;

            write_five_obj five;
            write_three_obj three;

            // Default construction
            func_void_type v1;
            AZ_TEST_ASSERT(!v1);

            // Assignment to an empty function
            v1 = five;
            AZ_TEST_ASSERT(v1 != nullptr);

            // Invocation of a function
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 5);

            // clear() method
            v1.clear();
            AZ_TEST_ASSERT(v1 == nullptr);

            // Assignment to an empty function
            v1 = three;
            AZ_TEST_ASSERT(v1);

            // Invocation and self-assignment
            global_int = 0;
            AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
            v1 = v1;
            AZ_POP_DISABLE_WARNING
            v1();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a non-empty function
            v1 = five;

            // Invocation and self-assignment
            global_int = 0;
            AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
            v1 = (v1);
            AZ_POP_DISABLE_WARNING
            v1();
            AZ_TEST_ASSERT(global_int == 5);

            // clear
            v1 = nullptr;
            AZ_TEST_ASSERT(nullptr == v1);

            // Assignment to an empty function from a free function
            v1 = AZSTD_FUNCTION_TARGET_FIX(&) write_five;
            AZ_TEST_ASSERT(nullptr != v1);

            // Invocation
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v1 = AZSTD_FUNCTION_TARGET_FIX(&) write_three;
            AZ_TEST_ASSERT(v1);

            // Invocation
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment
            v1 = five;
            AZ_TEST_ASSERT(v1);

            // Invocation
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v1 = &write_three;
            AZ_TEST_ASSERT(v1);

            // Invocation
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 3);

            // Construction from another function (that is empty)
            v1.clear();
            func_void_type v2(v1);
            AZ_TEST_ASSERT(!v2 ? true : false);

            // Assignment to an empty function
            v2 = three;
            AZ_TEST_ASSERT(v2);

            // Invocation
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a non-empty function
            v2 = (five);

            // Invocation
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 5);

            v2.clear();
            AZ_TEST_ASSERT(!v2);

            // Assignment to an empty function from a free function
            v2 = (AZSTD_FUNCTION_TARGET_FIX(&) write_five);
            AZ_TEST_ASSERT(v2 ? true : false);

            // Invocation
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v2 = AZSTD_FUNCTION_TARGET_FIX(&) write_three;
            AZ_TEST_ASSERT(v2);

            // Invocation
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 3);

            // Swapping
            v1 = five;
            swap(v1, v2);
            v2();
            AZ_TEST_ASSERT(global_int == 5);
            v1();
            AZ_TEST_ASSERT(global_int == 3);
            swap(v1, v2);
            v1.clear();

            // Assignment
            v2 = five;
            AZ_TEST_ASSERT(v2);

            // Invocation
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v2 = &write_three;
            AZ_TEST_ASSERT(v2);

            // Invocation
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a function from an empty function
            v2 = v1;
            AZ_TEST_ASSERT(!v2);

            // Assignment to a function from a function with a functor
            v1 = three;
            v2 = v1;
            AZ_TEST_ASSERT(v1);
            AZ_TEST_ASSERT(v2);

            // Invocation
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 3);
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 3);

            // Assign to a function from a function with a function
            v2 = AZSTD_FUNCTION_TARGET_FIX(&) write_five;
            v1 = v2;
            AZ_TEST_ASSERT(v1);
            AZ_TEST_ASSERT(v2);
            global_int = 0;
            v1();
            AZ_TEST_ASSERT(global_int == 5);
            global_int = 0;
            v2();
            AZ_TEST_ASSERT(global_int == 5);

            // Construct a function given another function containing a function
            func_void_type v3(v1);

            // Invocation of a function
            global_int = 0;
            v3();
            AZ_TEST_ASSERT(global_int == 5);

            // clear() method
            v3.clear();
            AZ_TEST_ASSERT(!v3 ? true : false);

            // Assignment to an empty function
            v3 = three;
            AZ_TEST_ASSERT(v3);

            // Invocation
            global_int = 0;
            v3();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a non-empty function
            v3 = five;

            // Invocation
            global_int = 0;
            v3();
            AZ_TEST_ASSERT(global_int == 5);

            // clear()
            v3.clear();
            AZ_TEST_ASSERT(!v3);

            // Assignment to an empty function from a free function
            v3 = &write_five;
            AZ_TEST_ASSERT(v3);

            // Invocation
            global_int = 0;
            v3();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v3 = &write_three;
            AZ_TEST_ASSERT(v3);

            // Invocation
            global_int = 0;
            v3();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment
            v3 = five;
            AZ_TEST_ASSERT(v3);

            // Invocation
            global_int = 0;
            v3();
            AZ_TEST_ASSERT(global_int == 5);

            // Construction of a function from a function containing a functor
            func_void_type v4(v3);

            // Invocation of a function
            global_int = 0;
            v4();
            AZ_TEST_ASSERT(global_int == 5);

            // clear() method
            v4.clear();
            AZ_TEST_ASSERT(!v4);

            // Assignment to an empty function
            v4 = three;
            AZ_TEST_ASSERT(v4);

            // Invocation
            global_int = 0;
            v4();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a non-empty function
            v4 = five;

            // Invocation
            global_int = 0;
            v4();
            AZ_TEST_ASSERT(global_int == 5);

            // clear()
            v4.clear();
            AZ_TEST_ASSERT(!v4);

            // Assignment to an empty function from a free function
            v4 = &write_five;
            AZ_TEST_ASSERT(v4);

            // Invocation
            global_int = 0;
            v4();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v4 = &write_three;
            AZ_TEST_ASSERT(v4);

            // Invocation
            global_int = 0;
            v4();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment
            v4 = five;
            AZ_TEST_ASSERT(v4);

            // Invocation
            global_int = 0;
            v4();
            AZ_TEST_ASSERT(global_int == 5);

            // Construction of a function from a functor
            func_void_type v5(five);

            // Invocation of a function
            global_int = 0;
            v5();
            AZ_TEST_ASSERT(global_int == 5);

            // clear() method
            v5.clear();
            AZ_TEST_ASSERT(!v5);

            // Assignment to an empty function
            v5 = three;
            AZ_TEST_ASSERT(v5);

            // Invocation
            global_int = 0;
            v5();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a non-empty function
            v5 = five;

            // Invocation
            global_int = 0;
            v5();
            AZ_TEST_ASSERT(global_int == 5);

            // clear()
            v5.clear();
            AZ_TEST_ASSERT(!v5);

            // Assignment to an empty function from a free function
            v5 = &write_five;
            AZ_TEST_ASSERT(v5);

            // Invocation
            global_int = 0;
            v5();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v5 = &write_three;
            AZ_TEST_ASSERT(v5);

            // Invocation
            global_int = 0;
            v5();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment
            v5 = five;
            AZ_TEST_ASSERT(v5);

            // Invocation
            global_int = 0;
            v5();
            AZ_TEST_ASSERT(global_int == 5);

            // Construction of a function from a function
            func_void_type v6(&write_five);

            // Invocation of a function
            global_int = 0;
            v6();
            AZ_TEST_ASSERT(global_int == 5);

            // clear() method
            v6.clear();
            AZ_TEST_ASSERT(!v6);

            // Assignment to an empty function
            v6 = three;
            AZ_TEST_ASSERT(v6);

            // Invocation
            global_int = 0;
            v6();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment to a non-empty function
            v6 = five;

            // Invocation
            global_int = 0;
            v6();
            AZ_TEST_ASSERT(global_int == 5);

            // clear()
            v6.clear();
            AZ_TEST_ASSERT(!v6);

            // Assignment to an empty function from a free function
            v6 = &write_five;
            AZ_TEST_ASSERT(v6);

            // Invocation
            global_int = 0;
            v6();
            AZ_TEST_ASSERT(global_int == 5);

            // Assignment to a non-empty function from a free function
            v6 = &write_three;
            AZ_TEST_ASSERT(v6);

            // Invocation
            global_int = 0;
            v6();
            AZ_TEST_ASSERT(global_int == 3);

            // Assignment
            v6 = five;
            AZ_TEST_ASSERT(v6);

            // Invocation
            global_int = 0;
            v6();
            AZ_TEST_ASSERT(global_int == 5);

            // Const vs. non-const
            write_const_1_nonconst_2 one_or_two;
            const function<void ()> v7(one_or_two);
            function<void ()> v8(one_or_two);

            global_int = 0;
            v7();
            AZ_TEST_ASSERT(global_int == 2);

            global_int = 0;
            v8();
            AZ_TEST_ASSERT(global_int == 2);

            // Test construction from 0 and comparison to 0
            func_void_type v9(nullptr);
            AZ_TEST_ASSERT(v9 == nullptr);
            AZ_TEST_ASSERT(nullptr == v9);

            // Test return values
            typedef function<int ()> func_int_type;
            generate_five_obj gen_five;
            generate_three_obj gen_three;

            func_int_type i0(gen_five);

            AZ_TEST_ASSERT(i0() == 5);
            i0 = gen_three;
            AZ_TEST_ASSERT(i0() == 3);
            i0 = &generate_five;
            AZ_TEST_ASSERT(i0() == 5);
            i0 = &generate_three;
            AZ_TEST_ASSERT(i0() == 3);
            AZ_TEST_ASSERT(i0 ? true : false);
            i0.clear();
            AZ_TEST_ASSERT(!i0 ? true : false);

            // Test return values with compatible types
            typedef function<long ()> func_long_type;
            func_long_type i1(gen_five);

            AZ_TEST_ASSERT(i1() == 5);
            i1 = gen_three;
            AZ_TEST_ASSERT(i1() == 3);
            i1 = &generate_five;
            AZ_TEST_ASSERT(i1() == 5);
            i1 = &generate_three;
            AZ_TEST_ASSERT(i1() == 3);
            AZ_TEST_ASSERT(i1 ? true : false);
            i1.clear();
            AZ_TEST_ASSERT(!i1 ? true : false);
        }

        void test_one_arg()
        {
            negate<int> neg;

            function<int (int)> f1(neg);
            AZ_TEST_ASSERT(f1(5) == -5);

            function<AZStd::string (AZStd::string)> id(&identity_str);
            AZ_TEST_ASSERT(id("str") == "str");

            function<AZStd::string (const char*)> id2(&identity_str);
            AZ_TEST_ASSERT(id2("foo") == "foo");

            add_to_obj add_to(5);
            function<int (int)> f2(add_to);
            AZ_TEST_ASSERT(f2(3) == 8);

            const function<int (int)> cf2(add_to);
            AZ_TEST_ASSERT(cf2(3) == 8);
        }

        void test_two_args()
        {
            function<AZStd::string (const AZStd::string&, const AZStd::string&)> cat(&string_cat);
            AZ_TEST_ASSERT(cat("str", "ing") == "string");

            function<int (short, short)> sum(&sum_ints);
            AZ_TEST_ASSERT(sum(2, 3) == 5);
        }

        static void test_emptiness()
        {
            function<float ()> f1;
            AZ_TEST_ASSERT(!f1);

            function<float ()> f2;
            f2 = f1;
            AZ_TEST_ASSERT(!f2);

            function<double ()> f3;
            f3 = f2;
            AZ_TEST_ASSERT(!f3);
        }

        struct X
        {
            X(int v)
                : value(v) {}

            int twice() const { return 2 * value; }
            int plus(int v) { return value + v; }

            int value;
        };

        void test_member_functions()
        {
            AZStd::function<int (X*)> f1(&X::twice);

            X one(1);
            X five(5);

            AZ_TEST_ASSERT(f1(&one) == 2);
            AZ_TEST_ASSERT(f1(&five) == 10);

            AZStd::function<int (X*)> f1_2;
            f1_2 = &X::twice;

            AZ_TEST_ASSERT(f1_2(&one) == 2);
            AZ_TEST_ASSERT(f1_2(&five) == 10);

            AZStd::function<int (X&, int)> f2(&X::plus);
            AZ_TEST_ASSERT(f2(one, 3) == 4);
            AZ_TEST_ASSERT(f2(five, 4) == 9);
        }

        struct add_with_throw_on_copy
        {
            int operator()(int x, int y) const { return x + y; }

            add_with_throw_on_copy() {}

            add_with_throw_on_copy(const add_with_throw_on_copy&)
            {
                //throw runtime_error("But this CAN'T throw");
            }

            add_with_throw_on_copy& operator=(const add_with_throw_on_copy&)
            {
                //throw runtime_error("But this CAN'T throw");
                return *this;
            }
        };

        void test_ref()
        {
            add_with_throw_on_copy atc;
            AZStd::function<int (int, int)> f(ref(atc));
            AZ_TEST_ASSERT(f(1, 3) == 4);
        }

        typedef AZStd::function< void* (void* reader) > reader_type;
        typedef AZStd::pair<int, reader_type> mapped_type;

        void test_implicit()
        {
            mapped_type m;
            m = mapped_type();
        }

        void test_call_obj(AZStd::function<int (int, int)> f)
        {
            AZ_TEST_ASSERT(f);
        }

        void test_call_cref(const AZStd::function<int (int, int)>& f)
        {
            AZ_TEST_ASSERT(f);
        }

        void test_call()
        {
            test_call_obj(AZStd::plus<int>());
            test_call_cref(AZStd::plus<int>());
        }
    };

    TEST_F(Function, ZeroArgs)
    {
        test_zero_args();
    }

    TEST_F(Function, OneArg)
    {
        test_one_arg();
    }

    TEST_F(Function, TwoArgs)
    {
        test_two_args();
    }

    TEST_F(Function, Emptiness)
    {
        test_emptiness();
    }

    TEST_F(Function, MemberFunctions)
    {
        test_member_functions();
    }

    TEST_F(Function, Ref)
    {
        test_ref();
    }

    TEST_F(Function, Implicit)
    {
        test_implicit();
    }

    TEST_F(Function, Call)
    {
        test_call();
    }

    TEST_F(Function, FunctionWithRValueParametersIsCallable)
    {
        auto rValuePlusLvalueFunc = [](int&& lhs, int& rhs) -> int
        {
            return lhs + rhs;
        };

        AZStd::function<int(int&&, int&)> testFunction1(rValuePlusLvalueFunc);
        int testInt1 = 17;
        EXPECT_EQ(25, testFunction1(8, testInt1));
        int testInt2 = 21;
        EXPECT_EQ(38, testFunction1(AZStd::move(testInt2), testInt1));

        auto pureRValueMultiplysLvalueFunc = [](int lhs, int& rhs) -> int
        {
            return lhs * rhs;
        };

        testFunction1 = pureRValueMultiplysLvalueFunc;
        int testInt3 = 23;
        EXPECT_EQ(92, testFunction1(4, testInt3));
        EXPECT_EQ(391, testFunction1(AZStd::move(testInt3), testInt1));
    }

    TEST_F(Function, FunctionWithValueParametersIsCallableWithRValueArguments)
    {
        auto xValueAndConstXValueFunc = [](int&& lhs, const int&& rhs) -> int
        {
            return lhs + rhs;
        };

        AZStd::function<int(int, const int&&)> testFunction1(xValueAndConstXValueFunc);
        const int testInt1 = 65;
        const int testInt2 = 13;
        EXPECT_EQ(78, testFunction1(testInt1, AZStd::move(testInt2)));
    }

    TEST_F(Function, FunctionWithNonAZStdAllocatorDestructsSuccessfully)
    {
        // 64 Byte buffer is used to prevent AZStd::function for storing the
        // lambda internal storage using the small buffer optimization
        // Therefore causing the supplied allocator to be used
        [[maybe_unused]] AZStd::aligned_storage_t<64, 1> bufferToAvoidSmallBufferOptimization;
        auto xValueAndConstXValueFunc = [bufferToAvoidSmallBufferOptimization](int lhs, int rhs) -> int
        {
            AZ_UNUSED(bufferToAvoidSmallBufferOptimization);
            return lhs + rhs;
        };

        {
            AZStd::function<int(int, const int&&)> testFunction1(xValueAndConstXValueFunc, AZ::OSStdAllocator());
            const int testInt1 = 76;
            const int testInt2 = -56;
            EXPECT_EQ(20, testFunction1(testInt1, AZStd::move(testInt2)));
        }
    }

    inline namespace FunctionTestInternal
    {
        static int s_functorCopyAssignmentCount;
        static int s_functorCopyConstructorCount;
        static int s_functorMoveAssignmentCount;
        static int s_functorMoveConstructorCount;
        template<size_t FunctorSize>
        struct Functor
        {
            Functor() = default;
            Functor(const Functor&)
            {
                ++s_functorCopyConstructorCount;
            };
            Functor& operator=(const Functor&)
            {
                ++s_functorCopyAssignmentCount;
                return *this;
            }
            Functor(Functor&& other)
            {
                *this = AZStd::move(other);
                ++s_functorMoveConstructorCount;
            }
            Functor& operator=(Functor&&)
            {
                ++s_functorMoveAssignmentCount;
                return *this;
            }

            double operator()(int lhs, double rhs)
            {
                return static_cast<double>(lhs) + rhs;
            }

            // Make sure the functor have a specific size so
            // that it can be used to test both the AZStd::function small_object_optimization path
            // and the heap allocated function object path
            AZStd::aligned_storage_t<FunctorSize, 1> m_functorPadding;
        };
    }

    // Fixture for Type Parameter test to test AZStd::function move operations
    template <typename FunctorType>
    class FunctionFunctorTestFixture
        : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            FunctionTestInternal::s_functorCopyAssignmentCount = 0;
            FunctionTestInternal::s_functorCopyConstructorCount = 0;
            FunctionTestInternal::s_functorMoveAssignmentCount = 0;
            FunctionTestInternal::s_functorMoveConstructorCount = 0;
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }

    };

    using FunctionFunctorTypes= ::testing::Types<
        FunctionTestInternal::Functor<1>,
        FunctionTestInternal::Functor<sizeof(AZStd::Internal::function_util::function_buffer) + 8>
    >;
    TYPED_TEST_CASE(FunctionFunctorTestFixture, FunctionFunctorTypes);



    TYPED_TEST(FunctionFunctorTestFixture, FunctorsCanBeMovedConstructedIntoFunction)
    {
        using TestFunctor = TypeParam;
        AZStd::function<double(int, double)> testFunction1{ TestFunctor() };
        double testFunc1Result = testFunction1(8, 16.0);
        EXPECT_GT(s_functorMoveConstructorCount, 0);
        EXPECT_DOUBLE_EQ(24, testFunc1Result);

        s_functorMoveConstructorCount = 0;
        s_functorMoveAssignmentCount = 0;
        TestFunctor testFunctor;
        AZStd::function<double(int, double)> testFunction2(AZStd::move(testFunctor));
        EXPECT_GT(s_functorMoveConstructorCount, 0);

        double testFunc2Result = testFunction2(16, 4.0);
        EXPECT_DOUBLE_EQ(20, testFunc2Result);

        // Check that only the move operations were invoked
        EXPECT_EQ(0, s_functorCopyConstructorCount);
        EXPECT_EQ(0, s_functorCopyAssignmentCount);
    }

    TYPED_TEST(FunctionFunctorTestFixture, FunctorsCanBeMovedAssignedIntoFunction)
    {
        using TestFunctor = TypeParam;
        AZStd::function<double(int, double)> testFunction1;
        testFunction1 = TestFunctor();
        double testFunc1Result = testFunction1(8, 16.0);
        EXPECT_GT(s_functorMoveConstructorCount + s_functorMoveAssignmentCount, 0);
        EXPECT_DOUBLE_EQ(24, testFunc1Result);

        s_functorMoveConstructorCount = 0;
        s_functorMoveAssignmentCount = 0;
        TestFunctor testFunctor;
        AZStd::function<double(int, double)> testFunction2;
        testFunction2 = AZStd::move(testFunctor);
        EXPECT_GT(s_functorMoveConstructorCount + s_functorMoveAssignmentCount, 0);

        double testFunc2Result = testFunction2(16, 4.0);
        EXPECT_DOUBLE_EQ(20, testFunc2Result);

        // Check that only the move operations were invoked
        EXPECT_EQ(0, s_functorCopyConstructorCount);
        EXPECT_EQ(0, s_functorCopyAssignmentCount);
    }

    /**
    * Bind
    * We use tuned version of the boost::bind (which is in TR1), so we use the boost::bind tests too
    */
    class Bind
        : public AllocatorsFixture
    {
    public:
        void function_test()
        {
            int const i = 1;

            AZ_TEST_ASSERT(bind(f_0)(i) == 17041L);
            AZ_TEST_ASSERT(bind(f_1, _1)(i) == 1L);
            AZ_TEST_ASSERT(bind(f_2, _1, 2)(i) == 21L);
            AZ_TEST_ASSERT(bind(f_3, _1, 2, 3)(i) == 321L);
            AZ_TEST_ASSERT(bind(f_4, _1, 2, 3, 4)(i) == 4321L);
            AZ_TEST_ASSERT(bind(f_5, _1, 2, 3, 4, 5)(i) == 54321L);
            AZ_TEST_ASSERT(bind(f_6, _1, 2, 3, 4, 5, 6)(i) == 654321L);
            AZ_TEST_ASSERT(bind(f_7, _1, 2, 3, 4, 5, 6, 7)(i) == 7654321L);
            AZ_TEST_ASSERT(bind(f_8, _1, 2, 3, 4, 5, 6, 7, 8)(i) == 87654321L);
            AZ_TEST_ASSERT(bind(f_9, _1, 2, 3, 4, 5, 6, 7, 8, 9)(i) == 987654321L);

            AZ_TEST_ASSERT((bind(fv_0)(i), (global_result == 17041L)));
            AZ_TEST_ASSERT((bind(fv_1, _1)(i), (global_result == 1L)));
            AZ_TEST_ASSERT((bind(fv_2, _1, 2)(i), (global_result == 21L)));
            AZ_TEST_ASSERT((bind(fv_3, _1, 2, 3)(i), (global_result == 321L)));
            AZ_TEST_ASSERT((bind(fv_4, _1, 2, 3, 4)(i), (global_result == 4321L)));
            AZ_TEST_ASSERT((bind(fv_5, _1, 2, 3, 4, 5)(i), (global_result == 54321L)));
            AZ_TEST_ASSERT((bind(fv_6, _1, 2, 3, 4, 5, 6)(i), (global_result == 654321L)));
            AZ_TEST_ASSERT((bind(fv_7, _1, 2, 3, 4, 5, 6, 7)(i), (global_result == 7654321L)));
            AZ_TEST_ASSERT((bind(fv_8, _1, 2, 3, 4, 5, 6, 7, 8)(i), (global_result == 87654321L)));
            AZ_TEST_ASSERT((bind(fv_9, _1, 2, 3, 4, 5, 6, 7, 8, 9)(i), (global_result == 987654321L)));
        }

        //

        struct Y
        {
            short operator()(short& r) const { return ++r; }
            int operator()(int a, int b) const { return a + 10 * b; }
            long operator() (long a, long b, long c) const { return a + 10 * b + 100 * c; }
            void operator() (long a, long b, long c, long d) const { global_result = a + 10 * b + 100 * c + 1000 * d; }
        };

        void function_object_test()
        {
            short i(6);

            int const k = 3;

            AZ_TEST_ASSERT(bind<short>(Y(), ref(i))() == 7);
            AZ_TEST_ASSERT(bind<short>(Y(), ref(i))() == 8);
            AZ_TEST_ASSERT(bind<int>(Y(), i, _1)(k) == 38);
            AZ_TEST_ASSERT(bind<long>(Y(), i, _1, 9)(k) == 938);

            global_result = 0;
            bind<void>(Y(), i, _1, 9, 4)(k);
            AZ_TEST_ASSERT(global_result == 4938);
        }

        void function_object_test2()
        {
            short i(6);

            int const k = 3;

            AZ_TEST_ASSERT(bind<short>(Y(), ref(i))() == 7);
            AZ_TEST_ASSERT(bind<short>(Y(), ref(i))() == 8);
            AZ_TEST_ASSERT(bind<int>(Y(), i, _1)(k) == 38);
            AZ_TEST_ASSERT(bind<long>(Y(), i, _1, 9)(k) == 938);

            global_result = 0;
            bind<void>( Y(), i, _1, 9, 4)(k);
            AZ_TEST_ASSERT(global_result == 4938);
        }

        //

        struct Z
        {
            typedef int result_type;
            int operator()(int a, int b) const { return a + 10 * b; }
        };

        void adaptable_function_object_test()
        {
            AZ_TEST_ASSERT(AZStd::bind(Z(), 7, 4)() == 47);
        }
        //

        struct X
        {
            mutable unsigned int hash;

            X()
                : hash(0) {}

            int f0() { f1(17); return 0; }
            int g0() const { g1(17); return 0; }

            int f1(int a1) { hash = (hash * 17041 + a1) % 32768; return 0; }
            int g1(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; return 0; }

            int f2(int a1, int a2) { f1(a1); f1(a2); return 0; }
            int g2(int a1, int a2) const { g1(a1); g1(a2); return 0; }

            int f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); return 0; }
            int g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); return 0; }

            int f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); return 0; }
            int g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); return 0; }

            int f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); return 0; }
            int g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); return 0; }

            int f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); return 0; }
            int g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); return 0; }

            int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); return 0; }
            int g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); return 0; }

            int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); return 0; }
            int g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); return 0; }
        };

        struct V
        {
            mutable unsigned int hash;

            V()
                : hash(0) {}

            void f0() { f1(17); }
            void g0() const { g1(17); }

            void f1(int a1) { hash = (hash * 17041 + a1) % 32768; }
            void g1(int a1) const { hash = (hash * 17041 + a1 * 2) % 32768; }

            void f2(int a1, int a2) { f1(a1); f1(a2); }
            void g2(int a1, int a2) const { g1(a1); g1(a2); }

            void f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); }
            void g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); }

            void f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); }
            void g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); }

            void f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); }
            void g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); }

            void f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); }
            void g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); }

            void f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); }
            void g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); }

            void f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); }
            void g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); }
        };

        void member_function_test()
        {
            X x;

            // 0
            bind(&X::f0, &x)();
            bind(&X::f0, ref(x))();

            bind(&X::g0, &x)();
            bind(&X::g0, x)();
            bind(&X::g0, ref(x))();

            // 1

            bind(&X::f1, &x, 1)();
            bind(&X::f1, ref(x), 1)();

            bind(&X::g1, &x, 1)();
            bind(&X::g1, x, 1)();
            bind(&X::g1, ref(x), 1)();

            // 2

            bind(&X::f2, &x, 1, 2)();
            bind(&X::f2, ref(x), 1, 2)();

            bind(&X::g2, &x, 1, 2)();
            bind(&X::g2, x, 1, 2)();
            bind(&X::g2, ref(x), 1, 2)();

            // 3

            bind(&X::f3, &x, 1, 2, 3)();
            bind(&X::f3, ref(x), 1, 2, 3)();

            bind(&X::g3, &x, 1, 2, 3)();
            bind(&X::g3, x, 1, 2, 3)();
            bind(&X::g3, ref(x), 1, 2, 3)();

            // 4

            bind(&X::f4, &x, 1, 2, 3, 4)();
            bind(&X::f4, ref(x), 1, 2, 3, 4)();

            bind(&X::g4, &x, 1, 2, 3, 4)();
            bind(&X::g4, x, 1, 2, 3, 4)();
            bind(&X::g4, ref(x), 1, 2, 3, 4)();

            // 5

            bind(&X::f5, &x, 1, 2, 3, 4, 5)();
            bind(&X::f5, ref(x), 1, 2, 3, 4, 5)();

            bind(&X::g5, &x, 1, 2, 3, 4, 5)();
            bind(&X::g5, x, 1, 2, 3, 4, 5)();
            bind(&X::g5, ref(x), 1, 2, 3, 4, 5)();

            // 6

            bind(&X::f6, &x, 1, 2, 3, 4, 5, 6)();
            bind(&X::f6, ref(x), 1, 2, 3, 4, 5, 6)();

            bind(&X::g6, &x, 1, 2, 3, 4, 5, 6)();
            bind(&X::g6, x, 1, 2, 3, 4, 5, 6)();
            bind(&X::g6, ref(x), 1, 2, 3, 4, 5, 6)();

            // 7

            bind(&X::f7, &x, 1, 2, 3, 4, 5, 6, 7)();
            bind(&X::f7, ref(x), 1, 2, 3, 4, 5, 6, 7)();

            bind(&X::g7, &x, 1, 2, 3, 4, 5, 6, 7)();
            bind(&X::g7, x, 1, 2, 3, 4, 5, 6, 7)();
            bind(&X::g7, ref(x), 1, 2, 3, 4, 5, 6, 7)();

            // 8

            bind(&X::f8, &x, 1, 2, 3, 4, 5, 6, 7, 8)();
            bind(&X::f8, ref(x), 1, 2, 3, 4, 5, 6, 7, 8)();

            bind(&X::g8, &x, 1, 2, 3, 4, 5, 6, 7, 8)();
            bind(&X::g8, x, 1, 2, 3, 4, 5, 6, 7, 8)();
            bind(&X::g8, ref(x), 1, 2, 3, 4, 5, 6, 7, 8)();

            AZ_TEST_ASSERT(x.hash == 23558);
        }

        void member_function_void_test()
        {
            V v;

            // 0

            bind(&V::f0, &v)();
            bind(&V::f0, ref(v))();

            bind(&V::g0, &v)();
            bind(&V::g0, v)();
            bind(&V::g0, ref(v))();

            // 1

            bind(&V::f1, &v, 1)();
            bind(&V::f1, ref(v), 1)();

            bind(&V::g1, &v, 1)();
            bind(&V::g1, v, 1)();
            bind(&V::g1, ref(v), 1)();

            // 2

            bind(&V::f2, &v, 1, 2)();
            bind(&V::f2, ref(v), 1, 2)();

            bind(&V::g2, &v, 1, 2)();
            bind(&V::g2, v, 1, 2)();
            bind(&V::g2, ref(v), 1, 2)();

            // 3

            bind(&V::f3, &v, 1, 2, 3)();
            bind(&V::f3, ref(v), 1, 2, 3)();

            bind(&V::g3, &v, 1, 2, 3)();
            bind(&V::g3, v, 1, 2, 3)();
            bind(&V::g3, ref(v), 1, 2, 3)();

            // 4

            bind(&V::f4, &v, 1, 2, 3, 4)();
            bind(&V::f4, ref(v), 1, 2, 3, 4)();

            bind(&V::g4, &v, 1, 2, 3, 4)();
            bind(&V::g4, v, 1, 2, 3, 4)();
            bind(&V::g4, ref(v), 1, 2, 3, 4)();

            // 5

            bind(&V::f5, &v, 1, 2, 3, 4, 5)();
            bind(&V::f5, ref(v), 1, 2, 3, 4, 5)();

            bind(&V::g5, &v, 1, 2, 3, 4, 5)();
            bind(&V::g5, v, 1, 2, 3, 4, 5)();
            bind(&V::g5, ref(v), 1, 2, 3, 4, 5)();

            // 6

            bind(&V::f6, &v, 1, 2, 3, 4, 5, 6)();
            bind(&V::f6, ref(v), 1, 2, 3, 4, 5, 6)();

            bind(&V::g6, &v, 1, 2, 3, 4, 5, 6)();
            bind(&V::g6, v, 1, 2, 3, 4, 5, 6)();
            bind(&V::g6, ref(v), 1, 2, 3, 4, 5, 6)();

            // 7

            bind(&V::f7, &v, 1, 2, 3, 4, 5, 6, 7)();
            bind(&V::f7, ref(v), 1, 2, 3, 4, 5, 6, 7)();

            bind(&V::g7, &v, 1, 2, 3, 4, 5, 6, 7)();
            bind(&V::g7, v, 1, 2, 3, 4, 5, 6, 7)();
            bind(&V::g7, ref(v), 1, 2, 3, 4, 5, 6, 7)();

            // 8

            bind(&V::f8, &v, 1, 2, 3, 4, 5, 6, 7, 8)();
            bind(&V::f8, ref(v), 1, 2, 3, 4, 5, 6, 7, 8)();

            bind(&V::g8, &v, 1, 2, 3, 4, 5, 6, 7, 8)();
            bind(&V::g8, v, 1, 2, 3, 4, 5, 6, 7, 8)();
            bind(&V::g8, ref(v), 1, 2, 3, 4, 5, 6, 7, 8)();

            AZ_TEST_ASSERT(v.hash == 23558);
        }

        void nested_bind_test()
        {
            int const x = 1;
            int const y = 2;

            AZ_TEST_ASSERT(bind(f_1, bind(f_1, _1))(x) == 1L);
            AZ_TEST_ASSERT(bind(f_1, bind(f_2, _1, _2))(x, y) == 21L);
            AZ_TEST_ASSERT(bind(f_2, bind(f_1, _1), bind(f_1, _1))(x) == 11L);
            AZ_TEST_ASSERT(bind(f_2, bind(f_1, _1), bind(f_1, _2))(x, y) == 21L);
            AZ_TEST_ASSERT(bind(f_1, bind(f_0))() == 17041L);

            AZ_TEST_ASSERT((bind(fv_1, bind(f_1, _1))(x), (global_result == 1L)));
            AZ_TEST_ASSERT((bind(fv_1, bind(f_2, _1, _2))(x, y), (global_result == 21L)));
            AZ_TEST_ASSERT((bind(fv_2, bind(f_1, _1), bind(f_1, _1))(x), (global_result == 11L)));
            AZ_TEST_ASSERT((bind(fv_2, bind(f_1, _1), bind(f_1, _2))(x, y), (global_result == 21L)));
            AZ_TEST_ASSERT((bind(fv_1, bind(f_0))(), (global_result == 17041L)));
        }

        // Big value parameter type, causing the small buffer optimization to fail and force allocation using the allocator
        struct BigValueParameterType
        {
            BigValueParameterType()      { memset(&m_data, 0, sizeof(m_data)); }
            BigValueParameterType(int d)
            {
                for (unsigned int i = 0; i < AZ_ARRAY_SIZE(m_data); ++i)
                {
                    m_data[i] = d;
                }
            }
            int m_data[10];
        };

        // Function to find which will cause system allocation
        int functionToBindWithAllocation(int a, BigValueParameterType c)
        {
            return a * c.m_data[0];
        }

        void bind_function_allocator_test()
        {
            typedef AZStd::function<int()> FunctionType;
            FunctionType f(AZStd::bind(&Bind::functionToBindWithAllocation, this, 5, BigValueParameterType(3)), AZStd::allocator());
            AZ_TEST_ASSERT(f() == (5 * 3));
        }
    };

    TEST_F(Bind, Function)
    {
        function_test();
    }

    TEST_F(Bind, FunctionObject)
    {
        function_object_test();
    }

    TEST_F(Bind, FunctionObject2)
    {
        function_object_test2();
    }

    TEST_F(Bind, AdaptableFunctionObject)
    {
        adaptable_function_object_test();
    }

    TEST_F(Bind, MemberFunction)
    {
        member_function_test();
    }

    TEST_F(Bind, MemberFunctionVoid)
    {
        member_function_void_test();
    }

    TEST_F(Bind, NestedBind)
    {
        nested_bind_test();
    }

    TEST_F(Bind, BindFunctionAllocator)
    {
        bind_function_allocator_test();
    }

    /**
     * Delegates
     *
     */
    TEST(Delegate, Test)
    {
        // Delegates with up to 8 parameters are supported.
        // Here's the case for a void function.
        // We declare a delegate and attach it to SimpleVoidFunction()
        delegate<void ()> noparameterdelegate(&SimpleVoidFunction);
        noparameterdelegate();  // invoke the delegate - this calls SimpleVoidFunction()
        AZ_TEST_ASSERT(global_int == -100);
        // ADD ASSERTS

        // By default, the return value is void.
        typedef delegate<void (int, char*)> MyDelegate;

        // If you want to have a non-void return value, put it at the end.
        typedef delegate<int (int, char*)> IntMyDelegate;

        MyDelegate funclist[12];  // delegates are initialized to empty
        BaseClass a("Base A");
        BaseClass b("Base B");
        DerivedClass d;
        DerivedClass c;

        IntMyDelegate newdeleg;
        newdeleg = make_delegate(&a, &BaseClass::SimpleMemberFunctionReturnsInt);
        int ret = newdeleg(1, 0);
        AZ_TEST_ASSERT(global_int == 1 + 2 && ret == -1);

        // Binding a simple member function
        funclist[0].bind(&a, &BaseClass::SimpleMemberFunction);
        funclist[0](2, 0);
        AZ_TEST_ASSERT(global_int == 2 + 1);

        // You can also bind static (free) functions
        funclist[1].bind(&SimpleStaticFunction);
        funclist[1](3, 0);
        AZ_TEST_ASSERT(global_int == 3);

        // and static member functions
        funclist[2].bind(&BaseClass::StaticMemberFunction);
        funclist[2](4, 0);
        AZ_TEST_ASSERT(global_int == 4 + 5);

        // and const member functions (these only need a const class pointer).
        funclist[11].bind((const BaseClass*)&a, &BaseClass::ConstMemberFunction);
        funclist[11](5, 0);
        AZ_TEST_ASSERT(global_int == 5 + 3);
        funclist[3].bind(&a, &BaseClass::ConstMemberFunction);
        funclist[3](6, 0);
        AZ_TEST_ASSERT(global_int == 6 + 3);
        // and virtual member functions
        funclist[4].bind(&b, &BaseClass::SimpleVirtualFunction);
        funclist[4](7, 0);
        AZ_TEST_ASSERT(global_int == 7 + 4);

        // You can also use the = operator. For static functions, a fastdelegate
        // looks identical to a simple function pointer.
        funclist[5] = &BaseClass::StaticMemberFunction;
        funclist[5](8, 0);
        AZ_TEST_ASSERT(global_int == 8 + 5);


        // The weird rule about the class of derived member function pointers is avoided.
        // For MSVC, you can use &DerivedClass::SimpleVirtualFunction here, but DMC will complain.
        // Note that as well as .bind(), you can also use the MakeDelegate()
        // global function.
        funclist[6] = make_delegate(&d, &BaseClass::SimpleVirtualFunction);
        funclist[6](9, 0);
        AZ_TEST_ASSERT(global_int == 9 + 4);

        // The worst case is an abstract virtual function of a virtually-derived class
        // with at least one non-virtual base class. This is a VERY obscure situation,
        // which you're unlikely to encounter in the real world.
        funclist[7].bind(&c, &DerivedClass::TrickyVirtualFunction);
        funclist[7](10, 0);
        AZ_TEST_ASSERT(global_int == 10 + 8);
        // BUT... in such cases you should be using the base class as an
        // interface, anyway.

        funclist[8].bind(&c, &OtherClass::TrickyVirtualFunction);
        funclist[8](11, 0);
        AZ_TEST_ASSERT(global_int == 11 + 8);
        // Calling a function that was first declared in the derived class is straightforward
        funclist[9] = make_delegate(&c, &DerivedClass::SimpleDerivedFunction);
        funclist[9](12, 0);
        AZ_TEST_ASSERT(global_int == 12 + 6);

        // You can also bind directly using the constructor
        MyDelegate dg(&b, &BaseClass::SimpleVirtualFunction);

        // The == and != operators are provided
        // Note that they work even for inline functions.
        AZ_TEST_ASSERT(funclist[4] == dg);
        AZ_TEST_ASSERT(funclist[0] != dg);
        AZ_TEST_ASSERT(funclist[1] != dg);
        AZ_TEST_ASSERT(funclist[2] != dg);
        AZ_TEST_ASSERT(funclist[3] != dg);
        AZ_TEST_ASSERT(funclist[5] != dg);
        AZ_TEST_ASSERT(funclist[6] != dg);
        AZ_TEST_ASSERT(funclist[7] != dg);
        AZ_TEST_ASSERT(funclist[8] != dg);
        AZ_TEST_ASSERT(funclist[9] != dg);
        AZ_TEST_ASSERT(funclist[10] != dg);
        AZ_TEST_ASSERT(funclist[11] != dg);

        AZ_TEST_ASSERT(!funclist[10]);
    }

    TEST_F(Bind, Lambda)
    {
        auto lambda = []() -> bool
        {
            return true;
        };

        EXPECT_TRUE(AZStd::bind(lambda)());

        EXPECT_EQ(5, AZStd::bind([](int shouldBe5) { return shouldBe5; }, 5)());
    }

    inline double FuncDoubleSelector(double select)
    {
        return select * 2.0;
    }

    inline double FuncWithMultiArgs(int32_t x, int16_t y, double z, AZStd::string&& strValue, AZStd::reference_wrapper<uint64_t>& refTimeStamp)
    {
        char* strEnd;
        int64_t timeStamp{ static_cast<int64_t>(strtoll(strValue.data(), &strEnd, 10)) };
        double resultTimeStamp = static_cast<double>(timeStamp + x + y + static_cast<int64_t>(z));
        refTimeStamp.get() = timeStamp;
        return static_cast<double>(resultTimeStamp);
    }

    TEST_F(Bind, NestedBind_Success)
    {
        auto nestedFunc = AZStd::bind(&FuncWithMultiArgs, AZStd::placeholders::_1, static_cast<int16_t>(16), AZStd::bind(&FuncDoubleSelector, AZStd::placeholders::_3), "512", AZStd::placeholders::_2);
        uint64_t timeStamp = 128;
        AZStd::reference_wrapper<uint64_t> refTimeStamp(timeStamp);
        double result = nestedFunc(32, refTimeStamp, 64.0);
        EXPECT_EQ(512, timeStamp);

        constexpr double expectedResult = static_cast<double>(32 + 16 + 128.0 + 512);
        EXPECT_DOUBLE_EQ(expectedResult, result);
    }
}
