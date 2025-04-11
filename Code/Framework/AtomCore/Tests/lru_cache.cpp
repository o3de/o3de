/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/std/containers/lru_cache.h>

#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZStd;

namespace UnitTest
{
    using HashedContainers = LeakDetectionFixture;

    TEST_F(HashedContainers, LRUCacheBasic)
    {
        lru_cache<int, int> intint_cache;
        EXPECT_EQ(intint_cache.capacity(), 0);
        EXPECT_EQ(intint_cache.empty(), true);
        EXPECT_EQ(intint_cache.size(), 0);
        EXPECT_EQ(intint_cache.begin(), intint_cache.end());
        EXPECT_EQ(intint_cache.rbegin(), intint_cache.rend());

        // should assert since capacity is 0.
        AZ_TEST_START_ASSERTTEST;
        intint_cache.insert(0, 0);
        AZ_TEST_STOP_ASSERTTEST(1);

        intint_cache.set_capacity(10);
        EXPECT_EQ(intint_cache.capacity(), 10);

        int i = 0;
        for (; i < 10; ++i)
        {
            intint_cache.insert(i, 2 * i);
        }
        EXPECT_EQ(intint_cache.size(), 10);

        // We should now have [0, 1, 2, 3, 4, 5, 6, 7, 8, 9], with 9 as the most recent (i.e. at begin()).
        i = 0;
        for (auto it = intint_cache.rbegin(); it != intint_cache.rend(); ++it, ++i)
        {
            EXPECT_EQ(it->first, i);
            EXPECT_EQ(it->second, 2 * i);
        }

        EXPECT_EQ(intint_cache.get(9)->first, 9);
        EXPECT_EQ(intint_cache.get(9)->second, 9 * 2);

        // Bump 2 to most recent.
        EXPECT_EQ(intint_cache.get(2)->first, 2);
        EXPECT_EQ(intint_cache.get(2)->second, 2 * 2);

        // Make sure it's most recent.
        EXPECT_EQ(intint_cache.begin()->first, 2);

        for (i = 10; i < 20; ++i)
        {
            intint_cache.insert(i, 2 * i);
        }
        EXPECT_EQ(intint_cache.size(), 10);

        // We should now have [10, 11, 12, 13, 14, 15, 16, 17, 18, 19], with 19 as the most recent (i.e. at begin()).
        i = 10;
        for (auto it = intint_cache.rbegin(); it != intint_cache.rend(); ++it, ++i)
        {
            EXPECT_EQ(it->first, i);
            EXPECT_EQ(it->second, 2 * i);
        }

        intint_cache.set_capacity(1);
        EXPECT_EQ(intint_cache.size(), 1);
        EXPECT_EQ(intint_cache.capacity(), 1);
        EXPECT_EQ(intint_cache.begin()->first, 19);

        intint_cache.set_capacity(8);
        for (i = 0; i < 8; ++i)
        {
            intint_cache.insert(i, 2 * i);
        }

        {
            auto it = intint_cache.get(5);
            EXPECT_EQ(it, intint_cache.begin());
            EXPECT_EQ(it->first, 5);
            EXPECT_EQ(it->second, 10);
        }

        // Test adding the same key 10 times.
        for (i = 0; i < 10; ++i)
        {
            intint_cache.insert(0, 0);
        }

        // the first element should be 0, the rest should be shifted.
        EXPECT_EQ(intint_cache.begin()->first, 0);
        EXPECT_EQ(intint_cache.begin()->second, 0);

        // Asset the second element is (5, 10) the previously added element.
        {
            auto it = intint_cache.begin();
            it++;
            EXPECT_EQ(it->first, 5);
            EXPECT_EQ(it->second, 10);
        }
    }

    TEST_F(HashedContainers, LRUCacheMoveConstruct)
    {
        using PtrType = AZStd::unique_ptr<int>;
        lru_cache<int, PtrType> intintptr_cache(10);

        int i = 0;
        for (; i < 10; ++i)
        {
            intintptr_cache.emplace(i, new int(2 * i));
        }
        EXPECT_EQ(intintptr_cache.size(), 10);

        i = 0;
        for (auto it = intintptr_cache.rbegin(); it != intintptr_cache.rend(); ++it, ++i)
        {
            EXPECT_EQ(it->first, i);
            EXPECT_EQ(*(it->second), i * 2);
        }
    }

    TEST_F(HashedContainers, LRUCacheRefCount)
    {
        class X : public AZStd::intrusive_base
        {
        public:
            X(uint32_t value) : m_value{value} {}

            uint32_t m_value;
        };

        const int TestValue = 123;

        using PtrType = AZStd::intrusive_ptr<X>;
        lru_cache<int, PtrType> intintptr_cache(10);

        PtrType p(new X(TestValue));
        intintptr_cache.emplace(0, p);

        auto beginIt = intintptr_cache.begin();
        EXPECT_EQ(beginIt->second->m_value, TestValue);
        EXPECT_EQ(p->use_count(), 2);

        int i = 0;
        for (; i < 10; ++i)
        {
            intintptr_cache.emplace(i, p);
        }

        // Should have all 10 references + the one we hold.
        EXPECT_EQ(p->use_count(), 11);

        intintptr_cache.clear();
        EXPECT_EQ(p->use_count(), 1);
    }
}
