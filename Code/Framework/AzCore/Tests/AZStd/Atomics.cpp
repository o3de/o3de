/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// Converted from libcxx's tests
// https://github.com/llvm-mirror/libcxx/tree/7175a079211ec78c8232d9d55fa4c1f9eeae803d/test/AZStd/atomics/
// convert assert -> EXPECT_TRUE
// convert EXPECT_TRUE\((.+)\s*==\s*(.+)\); -> EXPECT_EQ($2, $1);
// * Exception: gtest cannot deal with atomic<T>::operator==, nor can it print the value, so these stay as-is
// convert std:: -> AZStd::
// convert T -> TypeParam
// remove constexpr

#include "UserTypes.h"

#include <AzCore/std/parallel/atomic.h>

namespace UnitTest
{
    class Atomics
        : public LeakDetectionFixture
    {
    };

    template <class A, class T>
    bool cmpxchg_weak_loop(A& atomic, T& expected, T desired) 
    {
        for (int i = 0; i < 10; i++) 
        {
            if (atomic.compare_exchange_weak(expected, desired) == true) 
            {
                return true;
            }
        }

        return false;
    }

    template <class A, class T>
    bool cmpxchg_weak_loop(A& atomic, T& expected, T desired, AZStd::memory_order success, AZStd::memory_order failure) 
    {
        for (int i = 0; i < 10; i++) 
        {
            if (atomic.compare_exchange_weak(expected, desired, success, failure) == true) 
            {
                return true;
            }
        }

        return false;
    }

    template <class A, class T>
    bool c_cmpxchg_weak_loop(A* atomic, T* expected, T desired) 
    {
        for (int i = 0; i < 10; i++) 
        {
            if (AZStd::atomic_compare_exchange_weak(atomic, expected, desired) == true) 
            {
                return true;
            }
        }

        return false;
    }

    template <class A, class T>
    bool c_cmpxchg_weak_loop(A* atomic, T* expected, T desired, AZStd::memory_order success, AZStd::memory_order failure)
    {
        for (int i = 0; i < 10; i++) 
        {
            if (AZStd::atomic_compare_exchange_weak_explicit(atomic, expected, desired, success, failure) == true) 
            {
                return true;
            }
        }

        return false;
    }

    struct IntWrapper
    {
        int i;

        explicit IntWrapper(int d = 0) { i = d; }

        friend bool operator==(const IntWrapper& x, const IntWrapper& y)
        {
            return x.i == y.i;
        }
    };

    using AllAtomicTypes = ::testing::Types<
        char,
        unsigned char,
        short,
        unsigned short,
        int,
        unsigned int,
        long,
        unsigned long,
        long long,
        unsigned long long,
        wchar_t,
        int*,
        const int*
    >;

    using IntegralAtomicTypes = ::testing::Types<
        char,
        unsigned char,
        short,
        unsigned short,
        int,
        unsigned int,
        long,
        unsigned long,
        long long,
        unsigned long long,
        wchar_t
    >;

    using PointerAtomicTypes = ::testing::Types<
        int*,
        const int*
    >;

    template <class T>
    struct AtomicOps : public Atomics {};
    template <class T>
    struct AtomicOpsIntegral : public Atomics {};
    template <class T>
    struct AtomicOpsPointer : public Atomics {};

    TYPED_TEST_CASE(AtomicOps, AllAtomicTypes);
    TYPED_TEST_CASE(AtomicOpsIntegral, IntegralAtomicTypes);
    TYPED_TEST_CASE(AtomicOpsPointer, PointerAtomicTypes);

    TYPED_TEST(AtomicOps, CompareExchangeStrong)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(AZStd::atomic_compare_exchange_strong(&a, &t, TypeParam(2)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(1) == t);
            EXPECT_FALSE(AZStd::atomic_compare_exchange_strong(&a, &t, TypeParam(3)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(2) == t);
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(AZStd::atomic_compare_exchange_strong(&a, &t, TypeParam(2)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(1) == t);
            EXPECT_FALSE(AZStd::atomic_compare_exchange_strong(&a, &t, TypeParam(3)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(2) == t);
        }
    }

    TYPED_TEST(AtomicOps, CompareExchangeStrongExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(AZStd::atomic_compare_exchange_strong_explicit(&a, &t, TypeParam(2), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(1) == t);
            EXPECT_FALSE(AZStd::atomic_compare_exchange_strong_explicit(&a, &t, TypeParam(3), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(2) == t);
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(AZStd::atomic_compare_exchange_strong_explicit(&a, &t, TypeParam(2), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(1) == t);
            EXPECT_FALSE(AZStd::atomic_compare_exchange_strong_explicit(&a, &t, TypeParam(3), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(2) == t);
        }
    }

    TYPED_TEST(AtomicOps, CompareExchangeWeak)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(c_cmpxchg_weak_loop(&a, &t, TypeParam(2)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(1) == t);
            EXPECT_FALSE(AZStd::atomic_compare_exchange_weak(&a, &t, TypeParam(3)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(2) == t);
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(c_cmpxchg_weak_loop(&a, &t, TypeParam(2)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(1) == t);
            EXPECT_FALSE(AZStd::atomic_compare_exchange_weak(&a, &t, TypeParam(3)));
            EXPECT_TRUE(TypeParam(2) == a);
            EXPECT_TRUE(TypeParam(2) == t);
        }
    }

    TYPED_TEST(AtomicOps, CompareExchangeWeakExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(c_cmpxchg_weak_loop(&a, &t, TypeParam(2), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(a == TypeParam(2));
            EXPECT_TRUE(t == TypeParam(1));
            EXPECT_FALSE(AZStd::atomic_compare_exchange_weak_explicit(&a, &t, TypeParam(3), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(a == TypeParam(2));
            EXPECT_TRUE(t == TypeParam(2));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A a;
            TypeParam t(TypeParam(1));
            AZStd::atomic_init(&a, t);
            EXPECT_TRUE(c_cmpxchg_weak_loop(&a, &t, TypeParam(2), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(a == TypeParam(2));
            EXPECT_TRUE(t == TypeParam(1));
            EXPECT_FALSE(AZStd::atomic_compare_exchange_weak_explicit(&a, &t, TypeParam(3), AZStd::memory_order_seq_cst, AZStd::memory_order_seq_cst));
            EXPECT_TRUE(a == TypeParam(2));
            EXPECT_TRUE(t == TypeParam(2));
        }
    }

    TYPED_TEST(AtomicOps, Exchange)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_init(&t, TypeParam(1));
        EXPECT_TRUE(AZStd::atomic_exchange(&t, TypeParam(2)) == TypeParam(1));
        EXPECT_TRUE(t == TypeParam(2));
        volatile A vt;
        AZStd::atomic_init(&vt, TypeParam(3));
        EXPECT_TRUE(AZStd::atomic_exchange(&vt, TypeParam(4)) == TypeParam(3));
        EXPECT_TRUE(vt == TypeParam(4));
    }

    TYPED_TEST(AtomicOps, ExchangeExplicit)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_init(&t, TypeParam(1));
        EXPECT_TRUE(AZStd::atomic_exchange_explicit(&t, TypeParam(2), AZStd::memory_order_seq_cst) == TypeParam(1));
        EXPECT_TRUE(t == TypeParam(2));
        volatile A vt;
        AZStd::atomic_init(&vt, TypeParam(3));
        EXPECT_TRUE(AZStd::atomic_exchange_explicit(&vt, TypeParam(4), AZStd::memory_order_seq_cst) == TypeParam(3));
        EXPECT_TRUE(vt == TypeParam(4));
    }

    TYPED_TEST(AtomicOpsIntegral, FetchAdd)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_add(&t, TypeParam(2)) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_add(&t, TypeParam(2)) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
    }

    TYPED_TEST(AtomicOpsPointer, FetchAdd)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            typedef typename AZStd::remove_pointer<TypeParam>::type X;
            A t;
            AZStd::atomic_init(&t, TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(AZStd::atomic_fetch_add(&t, 2) == TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(t == TypeParam(3 * sizeof(X)));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            typedef typename AZStd::remove_pointer<TypeParam>::type X;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(AZStd::atomic_fetch_add(&t, 2) == TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(t == TypeParam(3 * sizeof(X)));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchAddExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_add_explicit(&t, TypeParam(2), AZStd::memory_order_seq_cst) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_add_explicit(&t, TypeParam(2), AZStd::memory_order_seq_cst) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
    }

    TYPED_TEST(AtomicOpsPointer, FetchAddExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            typedef typename AZStd::remove_pointer<TypeParam>::type X;
            A t;
            AZStd::atomic_init(&t, TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(AZStd::atomic_fetch_add_explicit(&t, 2, AZStd::memory_order_seq_cst) == TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(t == TypeParam(3 * sizeof(X)));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            typedef typename AZStd::remove_pointer<TypeParam>::type X;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(AZStd::atomic_fetch_add_explicit(&t, 2, AZStd::memory_order_seq_cst) == TypeParam(1 * sizeof(X)));
            EXPECT_TRUE(t == TypeParam(3 * sizeof(X)));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchAnd)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_and(&t, TypeParam(2)) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(0));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_and(&t, TypeParam(2)) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(2));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchAndExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_and_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(0));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_and_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(2));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchOr)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_or(&t, TypeParam(2)) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_or(&t, TypeParam(2)) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(3));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchOrExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_or_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_or_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(3));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchSub)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_sub(&t, TypeParam(2)) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(1));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_sub(&t, TypeParam(2)) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(1));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchSubExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_sub_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(1));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_sub_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(1));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchXor)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_xor(&t, TypeParam(2)) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_xor(&t, TypeParam(2)) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(1));
        }
    }

    TYPED_TEST(AtomicOpsIntegral, FetchXorExplicit)
    {
        {
            typedef AZStd::atomic<TypeParam> A;
            A t;
            AZStd::atomic_init(&t, TypeParam(1));
            EXPECT_TRUE(AZStd::atomic_fetch_xor_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(1));
            EXPECT_TRUE(t == TypeParam(3));
        }
        {
            typedef AZStd::atomic<TypeParam> A;
            volatile A t;
            AZStd::atomic_init(&t, TypeParam(3));
            EXPECT_TRUE(AZStd::atomic_fetch_xor_explicit(&t, TypeParam(2),
                AZStd::memory_order_seq_cst) == TypeParam(3));
            EXPECT_TRUE(t == TypeParam(1));
        }
    }

    TYPED_TEST(AtomicOps, Init)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_init(&t, TypeParam(1));
        EXPECT_TRUE(t == TypeParam(1));
        volatile A vt;
        AZStd::atomic_init(&vt, TypeParam(2));
        EXPECT_TRUE(vt == TypeParam(2));
    }

    TYPED_TEST(AtomicOps, IsLockFree)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        bool b1 = AZStd::atomic_is_lock_free(static_cast<const A*>(&t));
        volatile A vt;
        bool b2 = AZStd::atomic_is_lock_free(static_cast<const volatile A*>(&vt));
        EXPECT_TRUE(b1 == b2);
    }

    TYPED_TEST(AtomicOps, Load)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_init(&t, TypeParam(1));
        EXPECT_TRUE(AZStd::atomic_load(&t) == TypeParam(1));
        volatile A vt;
        AZStd::atomic_init(&vt, TypeParam(2));
        EXPECT_TRUE(AZStd::atomic_load(&vt) == TypeParam(2));
    }

    TYPED_TEST(AtomicOps, LoadExplicit)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_init(&t, TypeParam(1));
        EXPECT_TRUE(AZStd::atomic_load_explicit(&t, AZStd::memory_order_seq_cst) == TypeParam(1));
        volatile A vt;
        AZStd::atomic_init(&vt, TypeParam(2));
        EXPECT_TRUE(AZStd::atomic_load_explicit(&vt, AZStd::memory_order_seq_cst) == TypeParam(2));
    }

    TYPED_TEST(AtomicOps, Store)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_store(&t, TypeParam(1));
        EXPECT_TRUE(t == TypeParam(1));
        volatile A vt;
        AZStd::atomic_store(&vt, TypeParam(2));
        EXPECT_TRUE(vt == TypeParam(2));
    }

    TYPED_TEST(AtomicOps, StoreExplicit)
    {
        typedef AZStd::atomic<TypeParam> A;
        A t;
        AZStd::atomic_store_explicit(&t, TypeParam(1), AZStd::memory_order_seq_cst);
        EXPECT_TRUE(t == TypeParam(1));
        volatile A vt;
        AZStd::atomic_store_explicit(&vt, TypeParam(2), AZStd::memory_order_seq_cst);
        EXPECT_TRUE(vt == TypeParam(2));
    }

    TEST_F(Atomics, AtomicVarInit)
    {
        AZStd::atomic<int> v = AZ_ATOMIC_VAR_INIT(5);
        EXPECT_TRUE(v == 5);
    }

    template <class Tp>
    void test_ctor() {
        typedef AZStd::atomic<Tp> Atomic;
        Tp t(42);
        {
            Atomic a(t);
            EXPECT_TRUE(a == t);
        }
        {
            Atomic a{ t };
            EXPECT_TRUE(a == t);
        }
        {
            Atomic a = AZ_ATOMIC_VAR_INIT(t);
            EXPECT_TRUE(a == t);
        }
    }

    TEST_F(Atomics, IntegralCtor)
    {
        test_ctor<int>();
    }

    TEST_F(Atomics, UserCtor)
    {
        test_ctor<IntWrapper>();
    }

    TEST_F(Atomics, SignalFence)
    {
        AZStd::atomic_signal_fence(AZStd::memory_order_seq_cst);
    }

    TEST_F(Atomics, ThreadFence)
    {
        AZStd::atomic_thread_fence(AZStd::memory_order_seq_cst);
    }

    TEST_F(Atomics, AtomicFlagClear)
    {
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear(&f);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear(&f);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
    }

    TEST_F(Atomics, AtomicFlagClearExplicit)
    {
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear_explicit(&f, AZStd::memory_order_relaxed);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear_explicit(&f, AZStd::memory_order_release);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear_explicit(&f, AZStd::memory_order_seq_cst);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear_explicit(&f, AZStd::memory_order_relaxed);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear_explicit(&f, AZStd::memory_order_release);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            atomic_flag_clear_explicit(&f, AZStd::memory_order_seq_cst);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
    }

    TEST_F(Atomics, AtomicFlagTestAndSet)
    {
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set(&f) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set(&f) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
    }

    TEST_F(Atomics, AtomicFlagTestAndSetExplicit)
    {
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_relaxed) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_consume) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_acquire) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_release) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_acq_rel) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_seq_cst) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_relaxed) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_consume) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_acquire) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_release) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_acq_rel) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(atomic_flag_test_and_set_explicit(&f, AZStd::memory_order_seq_cst) == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
    }

    TEST_F(Atomics, AtomicFlagClearCombinations)
    {
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear();
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear(AZStd::memory_order_relaxed);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear(AZStd::memory_order_release);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear(AZStd::memory_order_seq_cst);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear();
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear(AZStd::memory_order_relaxed);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear(AZStd::memory_order_release);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
        {
            volatile AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
            f.test_and_set();
            f.clear(AZStd::memory_order_seq_cst);
            EXPECT_TRUE(f.test_and_set() == 0);
        }
    }

    TEST_F(Atomics, AtomicFlagDefault)
    {
        AZStd::atomic_flag f;
        f.clear();
        EXPECT_TRUE(f.test_and_set() == 0);
        {
            typedef AZStd::atomic_flag A;
            alignas(A) char storage[sizeof(A)] = { 1 };
            A& zero = *new (storage) A();
            EXPECT_TRUE(!zero.test_and_set());
            zero.~A();
        }
    }

    TEST_F(Atomics, AtomicFlagInit)
    {
        AZStd::atomic_flag f = AZ_ATOMIC_FLAG_INIT;
        EXPECT_TRUE(f.test_and_set() == 0);
    }

    TEST_F(Atomics, AtomicFlagTestAndSetCombinations)
    {
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set() == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_relaxed) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_relaxed) == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_consume) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_consume) == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acquire) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acquire) == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_release) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_release) == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acq_rel) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acq_rel) == 1);
        }
        {
            AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_seq_cst) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_seq_cst) == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set() == 0);
            EXPECT_TRUE(f.test_and_set() == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_relaxed) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_relaxed) == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_consume) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_consume) == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acquire) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acquire) == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_release) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_release) == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acq_rel) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_acq_rel) == 1);
        }
        {
            volatile AZStd::atomic_flag f;
            f.clear();
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_seq_cst) == 0);
            EXPECT_TRUE(f.test_and_set(AZStd::memory_order_seq_cst) == 1);
        }
    }

    TEST_F(Atomics, ValidateLockFreeMacros)
    {
        EXPECT_TRUE(AZ_ATOMIC_CHAR_LOCK_FREE == 0 ||
            AZ_ATOMIC_CHAR_LOCK_FREE == 1 ||
            AZ_ATOMIC_CHAR_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_CHAR16_T_LOCK_FREE == 0 ||
            AZ_ATOMIC_CHAR16_T_LOCK_FREE == 1 ||
            AZ_ATOMIC_CHAR16_T_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_CHAR32_T_LOCK_FREE == 0 ||
            AZ_ATOMIC_CHAR32_T_LOCK_FREE == 1 ||
            AZ_ATOMIC_CHAR32_T_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_WCHAR_T_LOCK_FREE == 0 ||
            AZ_ATOMIC_WCHAR_T_LOCK_FREE == 1 ||
            AZ_ATOMIC_WCHAR_T_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_SHORT_LOCK_FREE == 0 ||
            AZ_ATOMIC_SHORT_LOCK_FREE == 1 ||
            AZ_ATOMIC_SHORT_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_INT_LOCK_FREE == 0 ||
            AZ_ATOMIC_INT_LOCK_FREE == 1 ||
            AZ_ATOMIC_INT_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_LONG_LOCK_FREE == 0 ||
            AZ_ATOMIC_LONG_LOCK_FREE == 1 ||
            AZ_ATOMIC_LONG_LOCK_FREE == 2);
        EXPECT_TRUE(AZ_ATOMIC_LLONG_LOCK_FREE == 0 ||
            AZ_ATOMIC_LLONG_LOCK_FREE == 1 ||
            AZ_ATOMIC_LLONG_LOCK_FREE == 2);
    }

    TEST_F(Atomics, KillDependency)
    {
        EXPECT_TRUE(AZStd::kill_dependency(5) == 5);
        EXPECT_TRUE(AZStd::kill_dependency(-5.5) == -5.5);
    }

    TEST_F(Atomics, ValidateMemoryOrder)
    {
        EXPECT_TRUE(AZStd::memory_order_relaxed == 0);
        EXPECT_TRUE(AZStd::memory_order_consume == 1);
        EXPECT_TRUE(AZStd::memory_order_acquire == 2);
        EXPECT_TRUE(AZStd::memory_order_release == 3);
        EXPECT_TRUE(AZStd::memory_order_acq_rel == 4);
        EXPECT_TRUE(AZStd::memory_order_seq_cst == 5);
        AZStd::memory_order o = AZStd::memory_order_seq_cst;
        EXPECT_TRUE(o == 5);
    }

    template <class A, class T>
    void validate_atomic_ptr()
    {
        typedef typename AZStd::remove_pointer<T>::type X;
        A obj(T(0));
        bool b0 = obj.is_lock_free();
        ((void)b0); // mark as unused
        EXPECT_TRUE(obj == T(nullptr));
        AZStd::atomic_init(&obj, T(1));
        EXPECT_TRUE(obj == T(1));
        AZStd::atomic_init(&obj, T(2));
        EXPECT_TRUE(obj == T(2));
        obj.store(T(0));
        EXPECT_TRUE(obj == T(nullptr));
        obj.store(T(1), AZStd::memory_order_release);
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(obj.load() == T(1));
        EXPECT_TRUE(obj.load(AZStd::memory_order_acquire) == T(1));
        EXPECT_TRUE(obj.exchange(T(2)) == T(1));
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(obj.exchange(T(3), AZStd::memory_order_relaxed) == T(2));
        EXPECT_TRUE(obj == T(3));
        T x = obj;
        EXPECT_TRUE(cmpxchg_weak_loop(obj, x, T(2)) == true);
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(x == T(3));
        EXPECT_TRUE(obj.compare_exchange_weak(x, T(1)) == false);
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(x == T(2));
        x = T(2);
        EXPECT_TRUE(obj.compare_exchange_strong(x, T(1)) == true);
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(x == T(2));
        EXPECT_TRUE(obj.compare_exchange_strong(x, T(nullptr)) == false);
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(x == T(1));
        EXPECT_TRUE((obj = T(nullptr)) == T(nullptr));
        EXPECT_TRUE(obj == T(nullptr));
        obj = T(2 * sizeof(X));
        EXPECT_TRUE((obj += AZStd::ptrdiff_t(3)) == T(5 * sizeof(X)));
        EXPECT_TRUE(obj == T(5 * sizeof(X)));
        EXPECT_TRUE((obj -= AZStd::ptrdiff_t(3)) == T(2 * sizeof(X)));
        EXPECT_TRUE(obj == T(2 * sizeof(X)));

        {
            alignas(A) char storage[sizeof(A)] = { 23 };
            A& zero = *new (storage) A();
            EXPECT_TRUE(zero == T(nullptr));
            zero.~A();
        }
    }

    template <class A, class T>
    void validate_atomic_ptr_variations()
    {
        validate_atomic_ptr<A, T>();
        validate_atomic_ptr<volatile A, T>();
    }

    TEST_F(Atomics, AtomicPtrVariations)
    {
        validate_atomic_ptr_variations<AZStd::atomic<int*>, int*>();
    }

    TEST_F(Atomics, AtomicBoolFullSuite)
    {
        {
            volatile AZStd::atomic<bool> obj(true);
            EXPECT_TRUE(obj == true);
            AZStd::atomic_init(&obj, false);
            EXPECT_TRUE(obj == false);
            AZStd::atomic_init(&obj, true);
            EXPECT_TRUE(obj == true);
            bool b0 = obj.is_lock_free();
            (void)b0; // to placate scan-build
            obj.store(false);
            EXPECT_TRUE(obj == false);
            obj.store(true, AZStd::memory_order_release);
            EXPECT_TRUE(obj == true);
            EXPECT_TRUE(obj.load() == true);
            EXPECT_TRUE(obj.load(AZStd::memory_order_acquire) == true);
            EXPECT_TRUE(obj.exchange(false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(obj.exchange(true, AZStd::memory_order_relaxed) == false);
            EXPECT_TRUE(obj == true);
            bool x = obj;
            EXPECT_TRUE(cmpxchg_weak_loop(obj, x, false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE(obj.compare_exchange_weak(x, true,
                AZStd::memory_order_seq_cst) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == false);
            obj.store(true);
            x = true;
            EXPECT_TRUE(cmpxchg_weak_loop(obj, x, false,
                AZStd::memory_order_seq_cst,
                AZStd::memory_order_seq_cst) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            x = true;
            obj.store(true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, true,
                AZStd::memory_order_seq_cst) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == false);
            x = true;
            obj.store(true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, false,
                AZStd::memory_order_seq_cst,
                AZStd::memory_order_seq_cst) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE((obj = false) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE((obj = true) == true);
            EXPECT_TRUE(obj == true);
        }
        {
            AZStd::atomic<bool> obj(true);
            EXPECT_TRUE(obj == true);
            AZStd::atomic_init(&obj, false);
            EXPECT_TRUE(obj == false);
            AZStd::atomic_init(&obj, true);
            EXPECT_TRUE(obj == true);
            bool b0 = obj.is_lock_free();
            (void)b0; // to placate scan-build
            obj.store(false);
            EXPECT_TRUE(obj == false);
            obj.store(true, AZStd::memory_order_release);
            EXPECT_TRUE(obj == true);
            EXPECT_TRUE(obj.load() == true);
            EXPECT_TRUE(obj.load(AZStd::memory_order_acquire) == true);
            EXPECT_TRUE(obj.exchange(false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(obj.exchange(true, AZStd::memory_order_relaxed) == false);
            EXPECT_TRUE(obj == true);
            bool x = obj;
            EXPECT_TRUE(cmpxchg_weak_loop(obj, x, false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE(obj.compare_exchange_weak(x, true,
                AZStd::memory_order_seq_cst) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == false);
            obj.store(true);
            x = true;
            EXPECT_TRUE(cmpxchg_weak_loop(obj, x, false,
                AZStd::memory_order_seq_cst,
                AZStd::memory_order_seq_cst) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            x = true;
            obj.store(true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, true,
                AZStd::memory_order_seq_cst) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == false);
            x = true;
            obj.store(true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, false,
                AZStd::memory_order_seq_cst,
                AZStd::memory_order_seq_cst) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE((obj = false) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE((obj = true) == true);
            EXPECT_TRUE(obj == true);
        }
        {
            AZStd::atomic_bool obj(true);
            EXPECT_TRUE(obj == true);
            AZStd::atomic_init(&obj, false);
            EXPECT_TRUE(obj == false);
            AZStd::atomic_init(&obj, true);
            EXPECT_TRUE(obj == true);
            bool b0 = obj.is_lock_free();
            (void)b0; // to placate scan-build
            obj.store(false);
            EXPECT_TRUE(obj == false);
            obj.store(true, AZStd::memory_order_release);
            EXPECT_TRUE(obj == true);
            EXPECT_TRUE(obj.load() == true);
            EXPECT_TRUE(obj.load(AZStd::memory_order_acquire) == true);
            EXPECT_TRUE(obj.exchange(false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(obj.exchange(true, AZStd::memory_order_relaxed) == false);
            EXPECT_TRUE(obj == true);
            bool x = obj;
            EXPECT_TRUE(cmpxchg_weak_loop(obj, x, false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE(obj.compare_exchange_weak(x, true,
                AZStd::memory_order_seq_cst) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == false);
            obj.store(true);
            x = true;
            EXPECT_TRUE(cmpxchg_weak_loop(obj, x, false,
                AZStd::memory_order_seq_cst,
                AZStd::memory_order_seq_cst) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            x = true;
            obj.store(true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, false) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, true,
                AZStd::memory_order_seq_cst) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == false);
            x = true;
            obj.store(true);
            EXPECT_TRUE(obj.compare_exchange_strong(x, false,
                AZStd::memory_order_seq_cst,
                AZStd::memory_order_seq_cst) == true);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE(x == true);
            EXPECT_TRUE((obj = false) == false);
            EXPECT_TRUE(obj == false);
            EXPECT_TRUE((obj = true) == true);
            EXPECT_TRUE(obj == true);
        }
        {
            typedef AZStd::atomic<bool> A;
            alignas(A) char storage[sizeof(A)] = { 1 };
            A& zero = *new (storage) A();
            EXPECT_TRUE(zero == false);
            zero.~A();
        }
    }

    template <class A, class T>
    void test_atomic_integral()
    {
        A obj(T(0));
        EXPECT_TRUE(obj == T(0));
        AZStd::atomic_init(&obj, T(1));
        EXPECT_TRUE(obj == T(1));
        AZStd::atomic_init(&obj, T(2));
        EXPECT_TRUE(obj == T(2));
        bool b0 = obj.is_lock_free();
        ((void)b0); // mark as unused
        obj.store(T(0));
        EXPECT_TRUE(obj == T(0));
        obj.store(T(1), AZStd::memory_order_release);
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(obj.load() == T(1));
        EXPECT_TRUE(obj.load(AZStd::memory_order_acquire) == T(1));
        EXPECT_TRUE(obj.exchange(T(2)) == T(1));
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(obj.exchange(T(3), AZStd::memory_order_relaxed) == T(2));
        EXPECT_TRUE(obj == T(3));
        T x = obj;
        EXPECT_TRUE(cmpxchg_weak_loop(obj, x, T(2)) == true);
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(x == T(3));
        EXPECT_TRUE(obj.compare_exchange_weak(x, T(1)) == false);
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(x == T(2));
        x = T(2);
        EXPECT_TRUE(obj.compare_exchange_strong(x, T(1)) == true);
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(x == T(2));
        EXPECT_TRUE(obj.compare_exchange_strong(x, T(0)) == false);
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(x == T(1));
        EXPECT_TRUE((obj = T(0)) == T(0));
        EXPECT_TRUE(obj == T(0));
        EXPECT_TRUE(obj++ == T(0));
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(++obj == T(2));
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE(--obj == T(1));
        EXPECT_TRUE(obj == T(1));
        EXPECT_TRUE(obj-- == T(1));
        EXPECT_TRUE(obj == T(0));
        obj = T(2);
        EXPECT_TRUE((obj += T(3)) == T(5));
        EXPECT_TRUE(obj == T(5));
        EXPECT_TRUE((obj -= T(3)) == T(2));
        EXPECT_TRUE(obj == T(2));
        EXPECT_TRUE((obj |= T(5)) == T(7));
        EXPECT_TRUE(obj == T(7));
        EXPECT_TRUE((obj &= T(0xF)) == T(7));
        EXPECT_TRUE(obj == T(7));
        EXPECT_TRUE((obj ^= T(0xF)) == T(8));
        EXPECT_TRUE(obj == T(8));

        {
            alignas(A) char storage[sizeof(A)] = { 23 };
            A& zero = *new (storage) A();
            EXPECT_TRUE(zero == 0);
            zero.~A();
        }
    }

    template <class A, class T>
    void test_atomic_integral_variations()
    {
        test_atomic_integral<A, T>();
        test_atomic_integral<volatile A, T>();
    }

    TEST_F(Atomics, AtomicIntegralsFullSuite)
    {
        // NOTE: wchar_t is not integral in our impl, and schar was removed
        test_atomic_integral_variations<AZStd::atomic_char, char>();
        test_atomic_integral_variations<AZStd::atomic_uchar, unsigned char>();
        test_atomic_integral_variations<AZStd::atomic_short, short>();
        test_atomic_integral_variations<AZStd::atomic_ushort, unsigned short>();
        test_atomic_integral_variations<AZStd::atomic_int, int>();
        test_atomic_integral_variations<AZStd::atomic_uint, unsigned int>();
        test_atomic_integral_variations<AZStd::atomic_long, long>();
        test_atomic_integral_variations<AZStd::atomic_ulong, unsigned long>();
        test_atomic_integral_variations<AZStd::atomic_llong, long long>();
        test_atomic_integral_variations<AZStd::atomic_ullong, unsigned long long>();

        test_atomic_integral_variations<AZStd::atomic_size_t, size_t>();
        test_atomic_integral_variations<AZStd::atomic_ptrdiff_t, ptrdiff_t>();

        test_atomic_integral_variations<AZStd::atomic_int8_t, int8_t>();
        test_atomic_integral_variations<AZStd::atomic_uint8_t, uint8_t>();
        test_atomic_integral_variations<AZStd::atomic_int16_t, int16_t>();
        test_atomic_integral_variations<AZStd::atomic_uint16_t, uint16_t>();
        test_atomic_integral_variations<AZStd::atomic_int32_t, int32_t>();
        test_atomic_integral_variations<AZStd::atomic_uint32_t, uint32_t>();
        test_atomic_integral_variations<AZStd::atomic_int64_t, int64_t>();
        test_atomic_integral_variations<AZStd::atomic_uint64_t, uint64_t>();
    }
}
