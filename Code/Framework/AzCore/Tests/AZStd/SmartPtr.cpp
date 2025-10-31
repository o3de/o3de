/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_array.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>
#include <AzCore/std/smart_ptr/scoped_array.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    class SmartPtr
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override;

        void TearDown() override;

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // Shared pointer
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        class SharedPtr
        {
        public:
            class n_element_type
            {
            public:
                static void f(int&)
                {
                }

                static void test()
                {
                    typedef AZStd::shared_ptr<int>::element_type T;
                    T t;
                    f(t);
                }
            }; // namespace n_element_type

            class incomplete;

            class test
            {
            public:
                struct A
                {
                    int dummy;
                };

                struct B
                {
                    int dummy2;
                };

                struct C
                    : public A
                    , public virtual B
                {
                };

                struct X
                {
                    X(test& instanceOwner)
                        : instanceRef(instanceOwner)
                    {
                        ++instanceRef.m_xInstances;
                    }

                    virtual ~X()
                    {
                        --instanceRef.m_xInstances;
                    }

                    bool operator==(const X&) const
                    {
                        return true;
                    }

                private:
                    test& instanceRef;
                    X(X const&) = delete;
                    X& operator= (X const&) = delete;
                };

                // virtual inheritance stresses the implementation

                struct Y
                    : public A
                    , public virtual X
                {

                    Y(test& instanceOwner)
                        : X(instanceOwner)
                        , instanceRef(instanceOwner)
                    {
                        ++instanceRef.m_yInstances;
                    }

                    ~Y() override
                    {
                        --instanceRef.m_yInstances;
                    }

                private:
                    test& instanceRef;
                    Y(Y const&) = delete;
                    Y& operator= (Y const&) = delete;
                };


                static void deleter(int* p)
                {
                    EXPECT_TRUE(p == nullptr);
                }

                struct deleter2
                {
                    deleter2(const deleter2&) = default;
                    deleter2& operator=(const deleter2&) = default;
                    deleter2(int* basePtr) : memberPtr(basePtr) {}
                    void operator()(int* p)
                    {
                        EXPECT_TRUE(p == memberPtr);
                        ++*p;
                    }

                    int* memberPtr = nullptr;
                };

                struct deleter3
                {
                    void operator()(incomplete* p)
                    {
                        EXPECT_TRUE(p == nullptr);
                    }
                };


                struct deleter_void
                {
                    deleter_void(const deleter_void&) = default;
                    deleter_void& operator=(const deleter_void& other) = default;
                    deleter_void(void*& deletedPtr) : d(&deletedPtr) {}

                    void operator()(void* p)
                    {
                        *d = p;
                    }
                    void** d;
                };

                int m = 0;
                void* deleted = nullptr;
                AZ::s32 m_xInstances = 0;
                AZ::s32 m_yInstances = 0;
            };

            template <class P, class T>
            void TestPtr(AZStd::shared_ptr<P>& pt, T* p)
            {
                if (p)
                {
                    EXPECT_TRUE(pt);
                    EXPECT_FALSE(!pt);
                }
                else
                {
                    EXPECT_FALSE(pt);
                    EXPECT_TRUE(!pt);
                }

                EXPECT_EQ(p, pt.get());
                EXPECT_EQ(1, pt.use_count());
                EXPECT_TRUE(pt.unique());
            }

            template <class P, class T>
            void TestAddr(T* p)
            {
                AZStd::shared_ptr<P> pt(p);
                TestPtr(pt, p);
            }

            template <class P, class T>
            void TestNull(T* p)
            {
                EXPECT_EQ(nullptr, p);
                TestAddr<P, T>(p);
            }

            template <class T>
            void TestType(T* p)
            {
                TestNull<T>(p);
            }

            template <class P, class T>
            void TestValue(T* p, const T& val)
            {
                AZStd::shared_ptr<P> pi(p);
                EXPECT_TRUE(pi);
                EXPECT_FALSE(!pi);
                EXPECT_EQ(p, pi.get());
                EXPECT_EQ(1, pi.use_count());
                EXPECT_TRUE(pi.unique());
                EXPECT_EQ(val, *reinterpret_cast<const T*>(pi.get()));
            }

            template <class P, class T>
            void TestPolymorphic(T* p)
            {
                AZStd::shared_ptr<P> pi(p);
                EXPECT_TRUE(pi);
                EXPECT_FALSE(!pi);
                EXPECT_EQ(p, pi.get());
                EXPECT_EQ(1, pi.use_count());
                EXPECT_TRUE(pi.unique());
                EXPECT_EQ(1, m_test.m_xInstances);
                EXPECT_EQ(1, m_test.m_yInstances);
            }

            test m_test;
        };

        SharedPtr* m_sharedPtr = nullptr;
    
        /// Special deleter for RefCountedClass to use, that will track whether the object has been deleted.
        struct TestDeleter
            : public AZStd::intrusive_default_delete
        {
            bool* m_deleted = nullptr;

            explicit TestDeleter(bool* deleted = nullptr)
            {
                m_deleted = deleted;
            }

            template <typename U>
            void operator () (U* p) const
            {
                if (m_deleted)
                {
                    *m_deleted = true;
                }

                AZStd::intrusive_default_delete::operator () (p);
            }
        };

        struct RefCountedClass : public AZStd::intrusive_refcount<uint32_t, TestDeleter>
        {
            AZ_RTTI(RefCountedClass, "{622324BD-1407-4843-B798-90F7AA48D615}")

            RefCountedClass(bool* deleted = nullptr) : AZStd::intrusive_refcount<uint32_t, TestDeleter>(TestDeleter{deleted})
            {}

            AZStd::string m_string;
        };

        struct RefCountedSubclass : public RefCountedClass
        {
            AZ_RTTI(RefCountedSubclass, "{8C687739-01DC-4272-8008-5FE38A0839FB}", RefCountedClass)

            RefCountedSubclass(bool* deleted = nullptr) : RefCountedClass(deleted)
            {}

            AZStd::string m_anotherString;
        };

        struct RefCountedSubclassB : public RefCountedClass
        {
            AZ_RTTI(RefCountedSubclassB, "{809046A7-18F5-4DC3-8841-34EFACC5C5E8}", RefCountedClass)

            RefCountedSubclassB(bool* deleted = nullptr) : RefCountedClass(deleted)
            {}

            int m_number;
        };
    };

    void SmartPtr::SetUp()
    {
        LeakDetectionFixture::SetUp();
        m_sharedPtr = new SharedPtr;
    }

    void SmartPtr::TearDown()
    {
        delete m_sharedPtr;
        LeakDetectionFixture::TearDown();
    }

    TEST_F(SmartPtr, SharedPtrCtorInt)
    {
        AZStd::shared_ptr<int> pi;
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_EQ(nullptr, pi.get());
        EXPECT_EQ(0, pi.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorVoid)
    {
        AZStd::shared_ptr<void> pv;
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_EQ(nullptr, pv.get());
        EXPECT_EQ(0, pv.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorIncomplete)
    {
        AZStd::shared_ptr<SharedPtr::incomplete> px;
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_EQ(nullptr, px.get());
        EXPECT_EQ(0, px.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorNullptr)
    {
        AZStd::shared_ptr<int> pi(nullptr);
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_EQ(nullptr, pi.get());
        EXPECT_EQ(0, pi.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCtorIntPtr)
    {
        m_sharedPtr->TestType(static_cast<int*>(nullptr));
        m_sharedPtr->TestType(static_cast<int const*>(nullptr));
        m_sharedPtr->TestType(static_cast<int volatile*>(nullptr));
        m_sharedPtr->TestType(static_cast<int const volatile*>(nullptr));
    }

    TEST_F(SmartPtr, SharedPtrCtorConstIntPtr)
    {
        m_sharedPtr->TestNull<int const>(static_cast<int*>(nullptr));
        m_sharedPtr->TestNull<int volatile>(static_cast<int*>(nullptr));
        m_sharedPtr->TestNull<void>(static_cast<int*>(nullptr));
        m_sharedPtr->TestNull<void const>(static_cast<int*>(nullptr));
    }

    TEST_F(SmartPtr, SharedPtrCtorX)
    {
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X*>(nullptr));
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X const*>(nullptr));
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X volatile*>(nullptr));
        m_sharedPtr->TestType(static_cast<SharedPtr::test::X const volatile*>(nullptr));
    }

    TEST_F(SmartPtr, SharedPtrCtorXConvert)
    {
        m_sharedPtr->TestNull<SharedPtr::test::X const>(static_cast<SharedPtr::test::X*>(nullptr));
        m_sharedPtr->TestNull<SharedPtr::test::X>(static_cast<SharedPtr::test::Y*>(nullptr));
        m_sharedPtr->TestNull<SharedPtr::test::X const>(static_cast<SharedPtr::test::Y*>(nullptr));
        m_sharedPtr->TestNull<void>(static_cast<SharedPtr::test::X*>(nullptr));
        m_sharedPtr->TestNull<void const>(static_cast<SharedPtr::test::X*>(nullptr));
    }

    TEST_F(SmartPtr, SharedPtrCtorIntValue)
    {
        m_sharedPtr->TestValue<int>(new int(7), 7);
        m_sharedPtr->TestValue<int const>(new int(7), 7);
        m_sharedPtr->TestValue<void>(new int(7), 7);
        m_sharedPtr->TestValue<void const>(new int(7), 7);
    }

    TEST_F(SmartPtr, SharedPtrTestXValue)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<X>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
        
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<X const>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
        
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<void>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
        
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        m_sharedPtr->TestValue<void const>(new X(m_sharedPtr->m_test), X(m_sharedPtr->m_test));
    }

    TEST_F(SmartPtr, SharedPtrTestXValuePoly)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
        m_sharedPtr->TestPolymorphic< X, Y>(new Y(m_sharedPtr->m_test));

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
        m_sharedPtr->TestPolymorphic< X const, Y>(new Y(m_sharedPtr->m_test));
    }

    TEST_F(SmartPtr, SharedPtrFromUniquePtr)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        {
            X* p = new X(m_sharedPtr->m_test);
            AZStd::unique_ptr<X> pu(p);
            AZStd::shared_ptr<X> px(AZStd::move(pu));
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        {
            X* p = new X(m_sharedPtr->m_test);
            AZStd::unique_ptr<X> pu(p);
            AZStd::shared_ptr<X> px(nullptr);
            px = AZStd::move(pu);
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        {
            Y* p = new Y(m_sharedPtr->m_test);
            AZStd::unique_ptr<Y> pu(p);
            AZStd::shared_ptr<X> px(AZStd::move(pu));
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        {
            Y* p = new Y(m_sharedPtr->m_test);
            AZStd::unique_ptr<Y> pu(p);
            AZStd::shared_ptr<X> px(nullptr);
            px = AZStd::move(pu);
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, px.use_count());
            EXPECT_TRUE(px.unique());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, UniquePtrToUniquePtrPoly)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
        {
            // Test moving unique_ptr of child type to unique_ptr of base type
            Y * p = new Y(m_sharedPtr->m_test);
            AZStd::unique_ptr<Y> pu(p);
            AZStd::unique_ptr<X> px(AZStd::move(pu));
            EXPECT_TRUE(!!px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrCtorNullDeleter)
    {
        {
            AZStd::shared_ptr<int> pi(static_cast<int*>(nullptr), &SharedPtr::test::deleter);
            m_sharedPtr->TestPtr<int, int>(pi, nullptr);
        }
        {
            AZStd::shared_ptr<void> pv(static_cast<int*>(nullptr), &SharedPtr::test::deleter);
            m_sharedPtr->TestPtr<void, int>(pv, nullptr);
        }
        {
            AZStd::shared_ptr<void const> pv(static_cast<int*>(nullptr), &SharedPtr::test::deleter);
            m_sharedPtr->TestPtr<void const, int>(pv, nullptr);
        }
    }

    TEST_F(SmartPtr, SharedPtrCtorIncompleteDeleter)
    {
        SharedPtr::incomplete* p0 = nullptr;
        {
            AZStd::shared_ptr<SharedPtr::incomplete> px(p0, SharedPtr::test::deleter3());
            m_sharedPtr->TestPtr(px, p0);
        }
        {
            AZStd::shared_ptr<void> pv(p0, SharedPtr::test::deleter3());
            m_sharedPtr->TestPtr(pv, p0);
        }
        {
            AZStd::shared_ptr<void const> pv(p0, SharedPtr::test::deleter3());
            m_sharedPtr->TestPtr(pv, p0);
        }
    }

    TEST_F(SmartPtr, SharedPtrCtorIntDeleter)
    {
        EXPECT_EQ(0, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<int> pi(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pi, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(1, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<int const> pi(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pi, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(2, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<void> pv(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pv, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(3, m_sharedPtr->m_test.m);

        {
            AZStd::shared_ptr<void const> pv(&m_sharedPtr->m_test.m, SharedPtr::test::deleter2(&m_sharedPtr->m_test.m));
            m_sharedPtr->TestPtr(pv, &m_sharedPtr->m_test.m);
        }

        EXPECT_EQ(4, m_sharedPtr->m_test.m);
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorEmptyRhs)
    {
        AZStd::shared_ptr<int> pi;

        AZStd::shared_ptr<int> pi2(pi);
        EXPECT_EQ(pi2, pi);
        EXPECT_FALSE(pi2);
        EXPECT_TRUE(!pi2);
        EXPECT_EQ(nullptr, pi2.get());
        EXPECT_EQ(pi2.use_count(), pi.use_count());

        AZStd::shared_ptr<void> pi3(pi);
        EXPECT_EQ(pi3, pi);
        EXPECT_FALSE(pi3);
        EXPECT_TRUE(!pi3);
        EXPECT_EQ(nullptr, pi3.get());
        EXPECT_EQ(pi3.use_count(), pi.use_count());

        AZStd::shared_ptr<void> pi4(pi3);
        EXPECT_EQ(pi4, pi3);
        EXPECT_FALSE(pi4);
        EXPECT_TRUE(!pi4);
        EXPECT_EQ(nullptr, pi4.get());
        EXPECT_EQ(pi4.use_count(), pi3.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorEmptyVoid)
    {
        AZStd::shared_ptr<void> pv;

        AZStd::shared_ptr<void> pv2(pv);
        EXPECT_EQ(pv2, pv);
        EXPECT_FALSE(pv2);
        EXPECT_TRUE(!pv2);
        EXPECT_EQ(nullptr, pv2.get());
        EXPECT_EQ(pv2.use_count(), pv.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorVoidFromIncomplete)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<incomplete> px;

        AZStd::shared_ptr<incomplete> px2(px);
        EXPECT_EQ(px, px2);
        EXPECT_FALSE(px2);
        EXPECT_TRUE(!px2);
        EXPECT_EQ(nullptr, px2.get());
        EXPECT_EQ(px.use_count(), px2.use_count());

        AZStd::shared_ptr<void> px3(px);
        EXPECT_EQ(px, px3);
        EXPECT_FALSE(px3);
        EXPECT_TRUE(!px3);
        EXPECT_EQ(nullptr, px3.get());
        EXPECT_EQ(px.use_count(), px3.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorIntVoidSharedOwnershipTest)
    {
        AZStd::shared_ptr<int> pi(static_cast<int*>(nullptr));

        AZStd::shared_ptr<int> pi2(pi);
        EXPECT_EQ(pi, pi2);
        EXPECT_FALSE(pi2);
        EXPECT_TRUE(!pi2);
        EXPECT_EQ(nullptr, pi2.get());
        EXPECT_EQ(2, pi2.use_count());
        EXPECT_FALSE(pi2.unique());
        EXPECT_EQ(pi.use_count(), pi2.use_count());
        EXPECT_TRUE(!(pi < pi2 || pi2 < pi)); // shared ownership test

        AZStd::shared_ptr<void> pi3(pi);
        EXPECT_EQ(pi, pi3);
        EXPECT_FALSE(pi3);
        EXPECT_TRUE(!pi3);
        EXPECT_EQ(nullptr, pi3.get());
        EXPECT_EQ(3, pi3.use_count());
        EXPECT_FALSE(pi3.unique());
        EXPECT_EQ(pi.use_count(), pi3.use_count());
        EXPECT_TRUE(!(pi < pi3 || pi3 < pi)); // shared ownership test

        AZStd::shared_ptr<void> pi4(pi2);
        EXPECT_EQ(pi2, pi4);
        EXPECT_FALSE(pi4);
        EXPECT_TRUE(!pi4);
        EXPECT_EQ(nullptr, pi4.get());
        EXPECT_EQ(4, pi4.use_count());
        EXPECT_FALSE(pi4.unique());
        EXPECT_EQ(pi2.use_count(), pi4.use_count());
        EXPECT_TRUE(!(pi2 < pi4 || pi4 < pi2)); // shared ownership test

        EXPECT_EQ(pi4.use_count(), pi3.use_count());
        EXPECT_TRUE(!(pi3 < pi4 || pi4 < pi3)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorClassSharedOwnershipTest)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X> px(static_cast<X*>(nullptr));

        AZStd::shared_ptr<X> px2(px);
        EXPECT_EQ(px, px2);
        EXPECT_FALSE(px2);
        EXPECT_TRUE(!px2);
        EXPECT_EQ(nullptr, px2.get());
        EXPECT_EQ(2, px2.use_count());
        EXPECT_FALSE(px2.unique());
        EXPECT_EQ(px.use_count(), px2.use_count());
        EXPECT_TRUE(!(px < px2 || px2 < px)); // shared ownership test

        AZStd::shared_ptr<void> px3(px);
        EXPECT_EQ(px, px3);
        EXPECT_FALSE(px3);
        EXPECT_TRUE(!px3);
        EXPECT_EQ(nullptr, px3.get());
        EXPECT_EQ(3, px3.use_count());
        EXPECT_FALSE(px3.unique());
        EXPECT_EQ(px.use_count(), px3.use_count());
        EXPECT_TRUE(!(px < px3 || px3 < px)); // shared ownership test

        AZStd::shared_ptr<void> px4(px2);
        EXPECT_EQ(px2, px4);
        EXPECT_FALSE(px4);
        EXPECT_TRUE(!px4);
        EXPECT_EQ(nullptr, px4.get());
        EXPECT_EQ(4, px4.use_count());
        EXPECT_FALSE(px4.unique());
        EXPECT_EQ(px2.use_count(), px4.use_count());
        EXPECT_TRUE(!(px2 < px4 || px4 < px2)); // shared ownership test

        EXPECT_EQ(px4.use_count(), px3.use_count());
        EXPECT_TRUE(!(px3 < px4 || px4 < px3)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedIntValue)
    {
        int* p = new int(7);
        AZStd::shared_ptr<int> pi(p);

        AZStd::shared_ptr<int> pi2(pi);
        EXPECT_EQ(pi, pi2);
        EXPECT_TRUE(pi2 ? true : false);
        EXPECT_TRUE(!!pi2);
        EXPECT_EQ(p, pi2.get());
        EXPECT_EQ(2, pi2.use_count());
        EXPECT_TRUE(!pi2.unique());
        EXPECT_EQ(7, *pi2);
        EXPECT_EQ(pi.use_count(), pi2.use_count());
        EXPECT_TRUE(!(pi < pi2 || pi2 < pi)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedIntVoidValue)
    {
        int* p = new int(7);
        AZStd::shared_ptr<void> pv(p);
        EXPECT_EQ(p, pv.get());

        AZStd::shared_ptr<void> pv2(pv);
        EXPECT_EQ(pv, pv2);
        EXPECT_TRUE(pv2 ? true : false);
        EXPECT_TRUE(!!pv2);
        EXPECT_EQ(p, pv2.get());
        EXPECT_EQ(2, pv2.use_count());
        EXPECT_TRUE(!pv2.unique());
        EXPECT_EQ(pv.use_count(), pv2.use_count());
        EXPECT_TRUE(!(pv < pv2 || pv2 < pv)); // shared ownership test
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedInstanceCount)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0 ,m_sharedPtr->m_test.m_xInstances);

        {
            X* p = new X(m_sharedPtr->m_test);
            AZStd::shared_ptr<X> px(p);
            EXPECT_EQ(p, px.get());

            AZStd::shared_ptr<X> px2(px);
            EXPECT_EQ(px, px2);
            EXPECT_TRUE(px2 ? true : false);
            EXPECT_TRUE(!!px2);
            EXPECT_EQ(p, px2.get());
            EXPECT_EQ(2, px2.use_count());
            EXPECT_TRUE(!px2.unique());

            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

            EXPECT_EQ(px.use_count(), px2.use_count());
            EXPECT_TRUE(!(px < px2 || px2 < px)); // shared ownership test

            AZStd::shared_ptr<void> px3(px);
            EXPECT_EQ(px, px3);
            EXPECT_TRUE(px3 ? true : false);
            EXPECT_TRUE(!!px3);
            EXPECT_EQ(p, px3.get());
            EXPECT_EQ(3, px3.use_count());
            EXPECT_TRUE(!px3.unique());
            EXPECT_EQ(px.use_count(), px3.use_count());
            EXPECT_TRUE(!(px < px3 || px3 < px)); // shared ownership test

            AZStd::shared_ptr<void> px4(px2);
            EXPECT_EQ(px2, px4);
            EXPECT_TRUE(px4 ? true : false);
            EXPECT_TRUE(!!px4);
            EXPECT_EQ(p, px4.get());
            EXPECT_EQ(4, px4.use_count());
            EXPECT_TRUE(!px4.unique());
            EXPECT_EQ(px2.use_count(), px4.use_count());
            EXPECT_TRUE(!(px2 < px4 || px4 < px2)); // shared ownership test

            EXPECT_EQ(px4.use_count(), px3.use_count());
            EXPECT_TRUE(!(px3 < px4 || px4 < px3)); // shared ownership test
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrCopyCtorSharedPolyInstanceCount)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        {
            Y* p = new Y(m_sharedPtr->m_test);
            AZStd::shared_ptr<Y> py(p);
            EXPECT_EQ(p, py.get());

            AZStd::shared_ptr<X> px(py);
            EXPECT_EQ(py, px);
            EXPECT_TRUE(px);
            EXPECT_FALSE(!px);
            EXPECT_EQ(p, px.get());
            EXPECT_EQ(2, px.use_count());
            EXPECT_TRUE(!px.unique());
            EXPECT_EQ(py.use_count(), px.use_count());
            EXPECT_TRUE(!(px < py || py < px)); // shared ownership test

            EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
            EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

            AZStd::shared_ptr<void const> pv(px);
            EXPECT_EQ(px, pv);
            EXPECT_TRUE(pv);
            EXPECT_FALSE(!pv);
            EXPECT_EQ(px.get(), pv.get());
            EXPECT_EQ(3, pv.use_count());
            EXPECT_TRUE(!pv.unique());
            EXPECT_EQ(px.use_count(), pv.use_count());
            EXPECT_TRUE(!(px < pv || pv < px)); // shared ownership test

            AZStd::shared_ptr<void const> pv2(py);
            EXPECT_EQ(py, pv2);
            EXPECT_TRUE(pv2);
            EXPECT_FALSE(!pv2);
            EXPECT_EQ(py.get(), pv2.get());
            EXPECT_EQ(4, pv2.use_count());
            EXPECT_TRUE(!pv2.unique());
            EXPECT_EQ(py.use_count(), pv2.use_count());
            EXPECT_TRUE(!(py < pv2 || pv2 < py)); // shared ownership test

            EXPECT_EQ(pv2.use_count(), pv.use_count());
            EXPECT_TRUE(!(pv < pv2 || pv2 < pv)); // shared ownership test
        }

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrWeakPtrCtor)
    {
        using Y = SharedPtr::test::Y;
        AZStd::weak_ptr<Y> wp;
        EXPECT_EQ(0, wp.use_count());
    }

    TEST_F(SmartPtr, SharedPtrWeakPtrCtorSharedOwnership)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<Y> p;
        AZStd::weak_ptr<Y> wp(p);

        if (wp.use_count() != 0) // 0 allowed but not required
        {
            AZStd::shared_ptr<Y> p2(wp);
            EXPECT_EQ(wp.use_count(), p2.use_count());
            EXPECT_EQ(nullptr, p2.get());

            AZStd::shared_ptr<X> p3(wp);
            EXPECT_EQ(wp.use_count(), p3.use_count());
            EXPECT_EQ(nullptr, p3.get());
        }
    }


    TEST_F(SmartPtr, SharedPtrWeakPtrSharedOwnershipReset)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<Y> p(new Y(m_sharedPtr->m_test));
        AZStd::weak_ptr<Y> wp(p);

        {
            AZStd::shared_ptr<Y> p2(wp);
            EXPECT_TRUE(p2 ? true : false);
            EXPECT_TRUE(!!p2);
            EXPECT_EQ(p.get(), p2.get());
            EXPECT_EQ(2, p2.use_count());
            EXPECT_TRUE(!p2.unique());
            EXPECT_EQ(wp.use_count(), p2.use_count());

            EXPECT_EQ(p2.use_count(), p.use_count());
            EXPECT_TRUE(!(p < p2 || p2 < p)); // shared ownership test

            AZStd::shared_ptr<X> p3(wp);
            EXPECT_TRUE(p3 ? true : false);
            EXPECT_TRUE(!!p3);
            EXPECT_EQ(p.get(), p3.get());
            EXPECT_EQ(3, p3.use_count());
            EXPECT_TRUE(!p3.unique());
            EXPECT_EQ(wp.use_count(), p3.use_count());

            EXPECT_EQ(p3.use_count(), p.use_count());
        }

        p.reset();
        EXPECT_EQ(0, wp.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyAssignIncomplete)
    {
        using incomplete = SharedPtr::incomplete;

        AZStd::shared_ptr<incomplete> p1;

        AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
        p1 = p1;
        AZ_POP_DISABLE_WARNING

        EXPECT_EQ(p1, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<incomplete> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<incomplete> p3(p1);

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());
    }

    TEST_F(SmartPtr, SharedPtrCopyAssignVoid)
    {
        AZStd::shared_ptr<void> p1;

        AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
        p1 = p1;
        AZ_POP_DISABLE_WARNING

        EXPECT_EQ(p1, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<void> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<void> p3(p1);

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<void> p4(new int);
        EXPECT_EQ(1, p4.use_count());

        p1 = p4;

        EXPECT_EQ(p4, p1);
        EXPECT_TRUE(!(p1 < p4 || p4 < p1));
        EXPECT_EQ(2, p1.use_count());
        EXPECT_EQ(2, p4.use_count());

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_EQ(1, p4.use_count());
    }

    TEST_F(SmartPtr, SharedPtrCopyAssignClass)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X> p1;

        AZ_PUSH_DISABLE_WARNING(, "-Wself-assign-overloaded")
        p1 = p1;
        AZ_POP_DISABLE_WARNING

        EXPECT_EQ(p1, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<X> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<X> p3(p1);

        p1 = p3;

        EXPECT_EQ(p3, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        AZStd::shared_ptr<X> p4(new X(m_sharedPtr->m_test));

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        p1 = p4;

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        EXPECT_EQ(p4, p1);
        EXPECT_TRUE(!(p1 < p4 || p4 < p1));

        EXPECT_EQ(2, p1.use_count());

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        p4 = p3;

        EXPECT_EQ(p3, p4);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrConvertAssignIntVoidIncomplete)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<void> p1;

        AZStd::shared_ptr<incomplete> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        AZStd::shared_ptr<int> p4(new int);
        EXPECT_EQ(1, p4.use_count());

        AZStd::shared_ptr<void> p5(p4);
        EXPECT_EQ(2, p4.use_count());

        p1 = p4;

        EXPECT_EQ(p4, p1);
        EXPECT_TRUE(!(p1 < p5 || p5 < p1));
        EXPECT_EQ(3, p1.use_count());
        EXPECT_EQ(3, p4.use_count());

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_EQ(2, p4.use_count());
    }

    TEST_F(SmartPtr, SharedPtrConvertAssignPolymorphic)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<X> p1;
        AZStd::shared_ptr<Y> p2;

        p1 = p2;

        EXPECT_EQ(p2, p1);
        EXPECT_FALSE(p1);
        EXPECT_TRUE(!p1);
        EXPECT_EQ(nullptr, p1.get());

        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        AZStd::shared_ptr<Y> p4(new Y(m_sharedPtr->m_test));

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(m_sharedPtr->m_test.m_yInstances, 1);
        EXPECT_TRUE(p4.use_count() == 1);

        AZStd::shared_ptr<X> p5(p4);
        EXPECT_TRUE(p4.use_count() == 2);

        p1 = p4;

        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

        EXPECT_TRUE(p1 == p4);
        EXPECT_TRUE(!(p1 < p5 || p5 < p1));

        EXPECT_TRUE(p1.use_count() == 3);
        EXPECT_TRUE(p4.use_count() == 3);

        p1 = p2;

        EXPECT_TRUE(p1 == p2);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);
        EXPECT_TRUE(p4.use_count() == 2);

        p4 = p2;
        p5 = p2;

        EXPECT_TRUE(p4 == p2);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyInt)
    {
        AZStd::shared_ptr<int> pi;
        pi.reset();
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNullInt)
    {
        AZStd::shared_ptr<int> pi(static_cast<int*>(nullptr));
        pi.reset();
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNewInt)
    {
        AZStd::shared_ptr<int> pi(new int);
        pi.reset();
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetIncomplete)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<incomplete> px;
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetIncompleteWithDeleter)
    {
        using incomplete = SharedPtr::incomplete;
        incomplete* p0 = nullptr;
        AZStd::shared_ptr<incomplete> px(p0, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyClass)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X> px;
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNewClass)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        AZStd::shared_ptr<X> px(new X(m_sharedPtr->m_test));
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        px.reset();
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 0);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyVoid)
    {
        AZStd::shared_ptr<void> pv;
        pv.reset();
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 0);
    }

    TEST_F(SmartPtr, SharedPtrResetNewVoid)
    {
        using X = SharedPtr::test::X;
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        AZStd::shared_ptr<void> pv(new X(m_sharedPtr->m_test));
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        pv.reset();
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 0);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyToNewInt)
    {
        AZStd::shared_ptr<int> pi;

        pi.reset(static_cast<int*>(nullptr));
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        int* p = new int;
        pi.reset(p);
        EXPECT_TRUE(pi ? true : false);
        EXPECT_TRUE(!!pi);
        EXPECT_TRUE(pi.get() == p);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        pi.reset(static_cast<int*>(nullptr));
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyClassToNewClass)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<X> px;

        px.reset(static_cast<X*>(nullptr));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        X* p = new X(m_sharedPtr->m_test);
        px.reset(p);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        px.reset(static_cast<X*>(nullptr));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        Y* q = new Y(m_sharedPtr->m_test);
        px.reset(q);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == q);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

        px.reset(static_cast<Y*>(nullptr));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetEmptyVoidToNewClass)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<void> pv;

        pv.reset(static_cast<X*>(nullptr));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);

        X* p = new X(m_sharedPtr->m_test);
        pv.reset(p);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == p);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);

        pv.reset(static_cast<X*>(nullptr));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);

        Y* q = new Y(m_sharedPtr->m_test);
        pv.reset(q);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == q);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(1, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(1, m_sharedPtr->m_test.m_yInstances);

        pv.reset(static_cast<Y*>(nullptr));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());
        EXPECT_EQ(0, m_sharedPtr->m_test.m_xInstances);
        EXPECT_EQ(0, m_sharedPtr->m_test.m_yInstances);
    }

    TEST_F(SmartPtr, SharedPtrResetIntWithDeleter)
    {
        AZStd::shared_ptr<int> pi;

        pi.reset(static_cast<int*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        m_sharedPtr->m_test.deleted = &pi;

        int m = 0;
        pi.reset(&m, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
        EXPECT_TRUE(pi ? true : false);
        EXPECT_TRUE(!!pi);
        EXPECT_TRUE(pi.get() == &m);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        pi.reset(static_cast<int*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &m);
        EXPECT_FALSE(pi);
        EXPECT_TRUE(!pi);
        EXPECT_TRUE(pi.get() == nullptr);
        EXPECT_TRUE(pi.use_count() == 1);
        EXPECT_TRUE(pi.unique());

        pi.reset();
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrResetClassWithDeleter)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<X> px;

        px.reset(static_cast<X*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        m_sharedPtr->m_test.deleted = &px;

        X x(m_sharedPtr->m_test);
        px.reset(&x, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == &x);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        px.reset(static_cast<X*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &x);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        Y y(m_sharedPtr->m_test);
        px.reset(&y, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(px.get() == &y);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        px.reset(static_cast<Y*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &y);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        px.reset();
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrResetVoidClassWithDeleter)
    {
        using X = SharedPtr::test::X;
        using Y = SharedPtr::test::Y;
        AZStd::shared_ptr<void> pv;

        pv.reset(static_cast<X*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        m_sharedPtr->m_test.deleted = &pv;

        X x(m_sharedPtr->m_test);
        pv.reset(&x, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == &x);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        pv.reset(static_cast<X*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &x);
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        Y y(m_sharedPtr->m_test);
        pv.reset(&y, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
        EXPECT_TRUE(pv ? true : false);
        EXPECT_TRUE(!!pv);
        EXPECT_TRUE(pv.get() == &y);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        pv.reset(static_cast<Y*>(nullptr), SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == &y);
        EXPECT_FALSE(pv);
        EXPECT_TRUE(!pv);
        EXPECT_TRUE(pv.get() == nullptr);
        EXPECT_TRUE(pv.use_count() == 1);
        EXPECT_TRUE(pv.unique());

        pv.reset();
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrResetIncompleteNullWithDeleter)
    {
        using incomplete = SharedPtr::incomplete;
        AZStd::shared_ptr<incomplete> px;

        incomplete* p0 = nullptr;

        px.reset(p0, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        m_sharedPtr->m_test.deleted = &px;
        px.reset(p0, SharedPtr::test::deleter_void(m_sharedPtr->m_test.deleted));
        EXPECT_TRUE(m_sharedPtr->m_test.deleted == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrGetPointerEmpty)
    {
        struct X {};
        AZStd::shared_ptr<X> px;
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerNull)
    {
        struct X {};
        AZStd::shared_ptr<X> px(static_cast<X*>(nullptr));
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerCheckedDeleterNull)
    {
        struct X {};
        AZStd::shared_ptr<X> px(static_cast<X*>(nullptr), AZStd::checked_deleter<X>());
        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_FALSE(px);
        EXPECT_TRUE(!px);

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerNewClass)
    {
        struct X {};
        X* p = new X;
        AZStd::shared_ptr<X> px(p);
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(&*px == px.get());
        EXPECT_TRUE(px.operator ->() == px.get());

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrGetPointerCheckedDeleterNewClass)
    {
        struct X {};
        X* p = new X;
        AZStd::shared_ptr<X> px(p, AZStd::checked_deleter<X>());
        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px ? true : false);
        EXPECT_TRUE(!!px);
        EXPECT_TRUE(&*px == px.get());
        EXPECT_TRUE(px.operator ->() == px.get());

        EXPECT_TRUE(get_pointer(px) == px.get());
    }

    TEST_F(SmartPtr, SharedPtrUseCountNullClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px(static_cast<X*>(nullptr));
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        AZStd::shared_ptr<X> px2(px);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(!px2.unique());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(!px.unique());
    }

    TEST_F(SmartPtr, SharedPtrUseCountNewClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px(new X);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        AZStd::shared_ptr<X> px2(px);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(!px2.unique());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(!px.unique());
    }

    TEST_F(SmartPtr, SharedPtrUseCountCheckedDeleterNewClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px(new X, AZStd::checked_deleter<X>());
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px.unique());

        AZStd::shared_ptr<X> px2(px);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(!px2.unique());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(!px.unique());
    }

    TEST_F(SmartPtr, SharedPtrSwapEmptyClass)
    {
        struct X {};
        AZStd::shared_ptr<X> px;
        AZStd::shared_ptr<X> px2;

        px.swap(px2);

        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px2.get() == nullptr);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px2.get() == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrSwapNewClass)
    {
        struct X {};
        X* p = new X;
        AZStd::shared_ptr<X> px;
        AZStd::shared_ptr<X> px2(p);
        AZStd::shared_ptr<X> px3(px2);

        px.swap(px2);

        EXPECT_TRUE(px.get() == p);
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.get() == nullptr);
        EXPECT_TRUE(px3.get() == p);
        EXPECT_TRUE(px3.use_count() == 2);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == nullptr);
        EXPECT_TRUE(px2.get() == p);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(px3.get() == p);
        EXPECT_TRUE(px3.use_count() == 2);
    }

    TEST_F(SmartPtr, SharedPtrSwap2NewClasses)
    {
        struct X {};
        X* p1 = new X;
        X* p2 = new X;
        AZStd::shared_ptr<X> px(p1);
        AZStd::shared_ptr<X> px2(p2);
        AZStd::shared_ptr<X> px3(px2);

        px.swap(px2);

        EXPECT_TRUE(px.get() == p2);
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.get() == p1);
        EXPECT_TRUE(px2.use_count() == 1);
        EXPECT_TRUE(px3.get() == p2);
        EXPECT_TRUE(px3.use_count() == 2);

        using std::swap;
        swap(px, px2);

        EXPECT_TRUE(px.get() == p1);
        EXPECT_TRUE(px.use_count() == 1);
        EXPECT_TRUE(px2.get() == p2);
        EXPECT_TRUE(px2.use_count() == 2);
        EXPECT_TRUE(px3.get() == p2);
        EXPECT_TRUE(px3.use_count() == 2);
    }

    TEST_F(SmartPtr, SharedPtrCompareEmptyClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px;
        EXPECT_TRUE(px == px);
        EXPECT_TRUE(!(px != px));
        EXPECT_TRUE(!(px < px));

        AZStd::shared_ptr<A> px2;

        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(px == px2);
        EXPECT_TRUE(!(px != px2));
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareCopyOfEmptyClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px;
        AZStd::shared_ptr<A> px2(px);

        EXPECT_TRUE(px2 == px2);
        EXPECT_TRUE(!(px2 != px2));
        EXPECT_TRUE(!(px2 < px2));

        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(px == px2);
        EXPECT_TRUE(!(px != px2));
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareNewClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px;
        AZStd::shared_ptr<A> px2(new A);

        EXPECT_TRUE(px2 == px2);
        EXPECT_TRUE(!(px2 != px2));
        EXPECT_TRUE(!(px2 < px2));

        EXPECT_TRUE(px.get() != px2.get());
        EXPECT_TRUE(px != px2);
        EXPECT_TRUE(!(px == px2));
        EXPECT_TRUE(px < px2 || px2 < px);
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareNewClasses)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px(new A);
        AZStd::shared_ptr<A> px2(new A);

        EXPECT_TRUE(px.get() != px2.get());
        EXPECT_TRUE(px != px2);
        EXPECT_TRUE(!(px == px2));
        EXPECT_TRUE(px < px2 || px2 < px);
        EXPECT_TRUE(!(px < px2 && px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrCompareCopiedNewClass)
    {
        using A = SharedPtr::test::A;
        AZStd::shared_ptr<A> px(new A);
        AZStd::shared_ptr<A> px2(px);

        EXPECT_TRUE(px2 == px2);
        EXPECT_TRUE(!(px2 != px2));
        EXPECT_TRUE(!(px2 < px2));

        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(px == px2);
        EXPECT_TRUE(!(px != px2));
        EXPECT_TRUE(!(px < px2 || px2 < px));
    }

    TEST_F(SmartPtr, SharedPtrComparePolymorphicClasses)
    {
        using A = SharedPtr::test::A;
        using B = SharedPtr::test::B;
        using C = SharedPtr::test::C;
        AZStd::shared_ptr<A> px(new A);
        AZStd::shared_ptr<B> py(new B);
        AZStd::shared_ptr<C> pz(new C);

        EXPECT_NE(px.get(), pz.get());
        EXPECT_TRUE(px != pz);
        EXPECT_TRUE(!(px == pz));

        EXPECT_TRUE(py.get() != pz.get());
        EXPECT_TRUE(py != pz);
        EXPECT_TRUE(!(py == pz));

        EXPECT_TRUE(px < py || py < px);
        EXPECT_TRUE(px < pz || pz < px);
        EXPECT_TRUE(py < pz || pz < py);

        EXPECT_TRUE(!(px < py && py < px));
        EXPECT_TRUE(!(px < pz && pz < px));
        EXPECT_TRUE(!(py < pz && pz < py));

        AZStd::shared_ptr<void> pvx(px);

        EXPECT_TRUE(pvx == pvx);
        EXPECT_TRUE(!(pvx != pvx));
        EXPECT_TRUE(!(pvx < pvx));

        AZStd::shared_ptr<void> pvy(py);
        AZStd::shared_ptr<void> pvz(pz);

        EXPECT_TRUE(pvx < pvy || pvy < pvx);
        EXPECT_TRUE(pvx < pvz || pvz < pvx);
        EXPECT_TRUE(pvy < pvz || pvz < pvy);

        EXPECT_TRUE(!(pvx < pvy && pvy < pvx));
        EXPECT_TRUE(!(pvx < pvz && pvz < pvx));
        EXPECT_TRUE(!(pvy < pvz && pvz < pvy));
    }

    TEST_F(SmartPtr, SharedPtrComparePolymorphicWithEmpty)
    {
        using A = SharedPtr::test::A;
        using B = SharedPtr::test::B;
        using C = SharedPtr::test::C;
        AZStd::shared_ptr<C> pz(new C);
        AZStd::shared_ptr<A> px(pz);

        EXPECT_TRUE(px == px);
        EXPECT_TRUE(!(px != px));
        EXPECT_TRUE(!(px < px));

        AZStd::shared_ptr<B> py(pz);

        EXPECT_EQ(px.get(), pz.get());
        EXPECT_TRUE(px == pz);
        EXPECT_TRUE(!(px != pz));

        EXPECT_TRUE(py.get() == pz.get());
        EXPECT_TRUE(py == pz);
        EXPECT_TRUE(!(py != pz));

        EXPECT_TRUE(!(px < py || py < px));
        EXPECT_TRUE(!(px < pz || pz < px));
        EXPECT_TRUE(!(py < pz || pz < py));

        AZStd::shared_ptr<void> pvx(px);
        AZStd::shared_ptr<void> pvy(py);
        AZStd::shared_ptr<void> pvz(pz);

        // pvx and pvy aren't equal...
        EXPECT_TRUE(pvx.get() != pvy.get());
        EXPECT_TRUE(pvx != pvy);
        EXPECT_TRUE(!(pvx == pvy));

        // ... but they share ownership ...
        EXPECT_TRUE(!(pvx < pvy || pvy < pvx));

        // ... with pvz
        EXPECT_TRUE(!(pvx < pvz || pvz < pvx));
        EXPECT_TRUE(!(pvy < pvz || pvz < pvy));
    }

    TEST_F(SmartPtr, SharedPtrStaticPointerCastNull)
    {
        struct X {};
        AZStd::shared_ptr<void> pv;

        AZStd::shared_ptr<int> pi = AZStd::static_pointer_cast<int>(pv);
        EXPECT_TRUE(pi.get() == nullptr);

        AZStd::shared_ptr<X> px = AZStd::static_pointer_cast<X>(pv);
        EXPECT_TRUE(px.get() == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrStaticPointerCastNewIntToVoid)
    {
        AZStd::shared_ptr<int> pi(new int);
        AZStd::shared_ptr<void> pv(pi);

        AZStd::shared_ptr<int> pi2 = AZStd::static_pointer_cast<int>(pv);
        EXPECT_TRUE(pi.get() == pi2.get());
        EXPECT_TRUE(!(pi < pi2 || pi2 < pi));
        EXPECT_TRUE(pi.use_count() == 3);
        EXPECT_TRUE(pv.use_count() == 3);
        EXPECT_TRUE(pi2.use_count() == 3);
    }

    TEST_F(SmartPtr, SharedPtrStaticPointerCastPolymorphic)
    {
        struct X {};
        struct Y : public X {};
        AZStd::shared_ptr<X> px(new Y);

        AZStd::shared_ptr<Y> py = AZStd::static_pointer_cast<Y>(px);
        EXPECT_TRUE(px.get() == py.get());
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(py.use_count() == 2);

        AZStd::shared_ptr<X> px2(py);
        EXPECT_TRUE(!(px < px2 || px2 < px));
    }

    // Test smart pointer interactions with EBus
    struct SmartPtrTestBusInterface : public AZ::EBusTraits
    {
        virtual void TakesSharedPtrByCopy(AZStd::shared_ptr<int> sharedPtrByCopy) = 0;
    };
    using SmartPtrTestBus = AZ::EBus<SmartPtrTestBusInterface>;

    class TakesSharedPtrHandler : public SmartPtrTestBus::Handler
    {
    public:
        TakesSharedPtrHandler()
        {
            SmartPtrTestBus::Handler::BusConnect();
        }

        void TakesSharedPtrByCopy(AZStd::shared_ptr<int> sharedPtrByCopy) override
        {
            EXPECT_NE(nullptr, sharedPtrByCopy.get());
        }
    };

    // Ensure that if a shared_ptr RValue is passed into an EBus function
    // the first handler doesn't steal the contents.
    TEST_F(SmartPtr, SharedPtrPassToBusByRValue_AllHandlersReceiveValue)
    {
        TakesSharedPtrHandler handler1;
        TakesSharedPtrHandler handler2;

        SmartPtrTestBus::Broadcast(&SmartPtrTestBus::Events::TakesSharedPtrByCopy, AZStd::make_shared<int>(9));
    }

    TEST_F(SmartPtr, SharedPtrVoidConstPointerCastVoid)
    {
        AZStd::shared_ptr<void const volatile> px;

        AZStd::shared_ptr<void> px2 = AZStd::const_pointer_cast<void>(px);
        EXPECT_TRUE(px2.get() == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrIntConstPointerCastInt)
    {
        AZStd::shared_ptr<int const volatile> px;

        AZStd::shared_ptr<int> px2 = AZStd::const_pointer_cast<int>(px);
        EXPECT_TRUE(px2.get() == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrClassConstPointerCastClass)
    {
        using X = SharedPtr::test::X;
        AZStd::shared_ptr<X const volatile> px;

        AZStd::shared_ptr<X> px2 = AZStd::const_pointer_cast<X>(px);
        EXPECT_TRUE(px2.get() == nullptr);
    }

    TEST_F(SmartPtr, SharedPtrVoidVolatileConstPointerCastVoid)
    {
        AZStd::shared_ptr<void const volatile> px(new int);

        AZStd::shared_ptr<void> px2 = AZStd::const_pointer_cast<void>(px);
        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(!(px < px2 || px2 < px));
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.use_count() == 2);
    }

    TEST_F(SmartPtr, SharedPtrIntVolatileConstPointerCastInt)
    {
        AZStd::shared_ptr<int const volatile> px(new int);

        AZStd::shared_ptr<int> px2 = AZStd::const_pointer_cast<int>(px);
        EXPECT_TRUE(px.get() == px2.get());
        EXPECT_TRUE(!(px < px2 || px2 < px));
        EXPECT_TRUE(px.use_count() == 2);
        EXPECT_TRUE(px2.use_count() == 2);
    }

#if !defined(AZ_NO_RTTI)
    TEST_F(SmartPtr, DynamicPointerUpCastNull)
    {
        using SharedPtr::test::X;
        using SharedPtr::test::Y;
        AZStd::shared_ptr<X> pv;
        AZStd::shared_ptr<Y> pw = AZStd::dynamic_pointer_cast<Y>(pv);
        EXPECT_TRUE(pw.get() == 0);
    }

    TEST_F(SmartPtr, DynamicPointerCastUpCastThenDowncastNull)
    {
        using SharedPtr::test::X;
        using SharedPtr::test::Y;
        AZStd::shared_ptr<X> pv(static_cast<X*>(0));

        AZStd::shared_ptr<Y> pw = AZStd::dynamic_pointer_cast<Y>(pv);
        EXPECT_TRUE(pw.get() == 0);

        AZStd::shared_ptr<X> pv2(pw);
        EXPECT_TRUE(pv < pv2 || pv2 < pv);
    }

    TEST_F(SmartPtr, DynamicPointerCastNullDerivedUpcastToDerived)
    {
        using SharedPtr::test::X;
        using SharedPtr::test::Y;
        AZStd::shared_ptr<X> pv(static_cast<Y*>(0));

        AZStd::shared_ptr<Y> pw = AZStd::dynamic_pointer_cast<Y>(pv);
        EXPECT_TRUE(pw.get() == 0);

        AZStd::shared_ptr<X> pv2(pw);
        EXPECT_TRUE(pv < pv2 || pv2 < pv);
    }
#endif

    TEST_F(SmartPtr, IntrusivePtr_ConstructMoveDerived)
    {
        bool derivedIsDeleted = false;
        AZStd::intrusive_ptr<RefCountedSubclass> derived = new RefCountedSubclass{&derivedIsDeleted};
        RefCountedSubclass* originalDerived = derived.get();

        AZStd::intrusive_ptr<RefCountedClass> base{ AZStd::move(derived) };

        EXPECT_TRUE(base.get() == originalDerived);
        EXPECT_TRUE(derived.get() == nullptr);
        EXPECT_FALSE(derivedIsDeleted);
    }

    TEST_F(SmartPtr, IntrusivePtr_AssignMoveDerived)
    {
        bool derivedIsDeleted = false;
        AZStd::intrusive_ptr<RefCountedSubclass> derived = new RefCountedSubclass{&derivedIsDeleted};
        RefCountedSubclass* originalDerived = derived.get();

        bool originalBaseIsDeleted = false;
        AZStd::intrusive_ptr<RefCountedClass> base = new RefCountedClass{&originalBaseIsDeleted};

        base = AZStd::move(derived);

        EXPECT_TRUE(base.get() == originalDerived);
        EXPECT_TRUE(originalBaseIsDeleted);
        EXPECT_TRUE(derived.get() == nullptr);
        EXPECT_FALSE(derivedIsDeleted); 
    }

    TEST_F(SmartPtr, IntrusivePtr_DynamicCast)
    {

        AZStd::intrusive_ptr<RefCountedClass> basePointer = new RefCountedSubclass;

        AZStd::intrusive_ptr<RefCountedSubclass> correctCast = azdynamic_cast<RefCountedSubclass *>(basePointer);
        AZStd::intrusive_ptr<RefCountedSubclassB> wrongCast = azdynamic_cast<RefCountedSubclassB *>(basePointer);

        EXPECT_TRUE(correctCast.get() == basePointer.get());
        EXPECT_TRUE(wrongCast.get() == nullptr);
        EXPECT_EQ(2, basePointer->use_count());
    }

} // namespace UnitTest
