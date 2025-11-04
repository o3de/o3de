/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestTypes.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/time.h>
#include <AzRHI/Containers/Handle.h>
#include <AzRHI/Containers/HandleMap.h>
#include <AzRHI/Containers/ChunkedVector.h>

namespace UnitTest
{
    struct IntContainer
    {
        const AZ::u32 DefaultValue = 1234;

        IntContainer(AZ::u32& totalCount, AZ::u32 value)
            : m_value{ value }
            , m_totalCount{ &totalCount }
        {
            totalCount++;
        }

        IntContainer(const IntContainer& rhs)
            : m_value{ rhs.m_value }
            , m_totalCount{ rhs.m_totalCount }
        {
            (*m_totalCount)++;
        }

        IntContainer(IntContainer&& rhs)
            : m_value{ rhs.m_value }
            , m_totalCount{ rhs.m_totalCount }
        {
            rhs.m_totalCount = nullptr;
        }

        ~IntContainer()
        {
            if (m_totalCount)
            {
                --(*m_totalCount);
            }
        }

        AZ::u32* m_totalCount = nullptr;
        AZ::u32 m_value = DefaultValue;
    };

    class ChunkedVectorTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void run()
        {
            static const size_t CapacityLogSize = 16; // 65535 elements.
            static const size_t Capacity = (1 << CapacityLogSize);

            AZ::u32 totalCount = 0;

            AzRHI::ChunkedVector<IntContainer, AZStd::allocator, CapacityLogSize> v;

            //////////////////////////////////////////////////////////////////////////
            // Test push back (copy construction)

            for (AZ::u32 i = 0; i < Capacity; ++i)
            {
                IntContainer value(totalCount, i);
                v.push_back(value);
            }

            ASSERT_TRUE(v.size() == Capacity);
            ASSERT_TRUE(v.capacity() == Capacity);

            for (size_t i = 0; i < Capacity; ++i)
            {
                ASSERT_TRUE(v[i].m_value == i);
            }

            for (size_t i = 0; i < Capacity; ++i)
            {
                v.pop_back();
            }

            ASSERT_TRUE(v.size() == 0);
            ASSERT_TRUE(v.capacity() == Capacity);
            ASSERT_TRUE(totalCount == 0);

            //////////////////////////////////////////////////////////////////////////
            // Test push back (move construction)

            for (AZ::u32 i = 0; i < Capacity; ++i)
            {
                v.push_back(IntContainer(totalCount, i));
            }

            ASSERT_TRUE(v.size() == Capacity);

            for (size_t i = 0; i < Capacity; ++i)
            {
                ASSERT_TRUE(v[i].m_value == i);
            }

            for (size_t i = 0; i < Capacity; ++i)
            {
                v.pop_back();
            }

            ASSERT_TRUE(v.size() == 0);
            ASSERT_TRUE(v.capacity() == Capacity);
            ASSERT_TRUE(totalCount == 0);

            ///////////////////////////////////////////////////////////////////////////
            // Test emplacement

            for (AZ::u32 i = 0; i < Capacity; ++i)
            {
                v.emplace_back(totalCount, i * 2);
            }

            for (size_t i = 0; i < Capacity; ++i)
            {
                ASSERT_TRUE(v[i].m_value == (i * 2));
            }

            v.clear();

            ASSERT_TRUE(v.size() == 0);
            ASSERT_TRUE(v.capacity() == Capacity);
            ASSERT_TRUE(totalCount == 0);
        }
    };

    TEST_F(ChunkedVectorTest, Test)
    {
        run();
    }

    class HandleMapTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        HandleMapTest()
            : LeakDetectionFixture()
        {}

        void run()
        {
            AZ::u32 totalCount = 0;

            using TestHandle = AzRHI::Handle<AzRHI::HandleFormats::Index16Generation16>;

            AzRHI::HandleMap<IntContainer, TestHandle> m;

            AZStd::unordered_map<TestHandle, AZ::u32> m_allocations;

            AZ::SimpleLcgRandom random(AZStd::GetTimeNowMicroSecond());

            const AZ::u32 IterationCount = 100000;

            for (AZ::u32 i = 0; i < IterationCount; ++i)
            {
                // Add
                if (random.GetRandom() % 2 == 0)
                {
                    AZ::u32 randomValue = random.GetRandom();

                    TestHandle handle = m.emplace(totalCount, randomValue);
                    if (handle.IsValid())
                    {
                        auto result = m_allocations.emplace(handle, randomValue);
                        ASSERT_TRUE(result.second); // we should not have a duplicate.
                    }
                    else
                    {
                        // We should be full.
                        ASSERT_TRUE(m.size() == TestHandle::Traits::HandleCountMax);
                    }
                }
                // Remove
                else
                {
                    if (m_allocations.size())
                    {
                        auto it = m_allocations.begin();
                        AZ::u32 index = (random.GetRandom() % m_allocations.size());

                        while (index)
                        {
                            ++it;
                            --index;
                        }

                        m.remove(it->first);
                        m_allocations.erase(it);
                    }
                }

                ASSERT_TRUE(m.size() == m_allocations.size());
                ASSERT_TRUE(totalCount == m_allocations.size());

                // Test an access
                if (m_allocations.size())
                {
                    auto it = m_allocations.begin();
                    AZ::u32 index = (random.GetRandom() % m_allocations.size());

                    while (index)
                    {
                        ++it;
                        --index;
                    }

                    TestHandle handle = it->first;
                    AZ::u32 value = it->second;

                    const IntContainer* container = m[handle];
                    ASSERT_TRUE(container);
                    ASSERT_TRUE(container->m_value == value);
                }
            }
        }
    };

    TEST_F(HandleMapTest, Test)
    {
        run();
    }
}
