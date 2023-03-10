/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Component/ComponentApplication.h>
#include <Atom/Utils/StableDynamicStructOfArrays.h>

namespace UnitTest
{
    enum TestStableDynamicStructOfArraysRows
    {
        TestItemIndex = 0,
        UInt32Index
    };

    // Fixture that creates a bare-bones app
    constexpr size_t TestElementsPerPage = 512;
    using StructOfArraysTestAllocator = AZStd::allocator;

    struct StableDynamicStructOfArraysTestsTestItem
    {
        StableDynamicStructOfArraysTestsTestItem() = default;
        StableDynamicStructOfArraysTestsTestItem(uint32_t index, float value)
            : m_index(index)
            , m_value(value)
        {
        }
        uint32_t m_index = 0;
        float m_value = 0.0f;
    };

    using TestArrayType = AZ::StableDynamicStructOfArrays<TestElementsPerPage, StructOfArraysTestAllocator, StableDynamicStructOfArraysTestsTestItem, uint32_t>;
    class StableDynamicStructOfArraysTests
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            handles.reserve(s_testCount);
        }

        void TearDown() override
        {
            handles = AZStd::vector<TestArrayType::Handle>(); // force memory deallocation.

            LeakDetectionFixture::TearDown();
        }

        using TestItem = StableDynamicStructOfArraysTestsTestItem;

        static constexpr uint32_t s_testCount = 1000000;

        AZStd::vector<TestArrayType::Handle> handles;
    };

    TEST_F(StableDynamicStructOfArraysTests, insert_erase)
    {
        using namespace AZ;
        TestArrayType testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayType::Handle handle = testArray.insert(TestItem{ i, static_cast<float>(i) }, i);
            TestItem& testItem = handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>();
            testItem.m_index = i;
            testItem.m_value = static_cast<float>(i);
            handle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() = i;
            handles.push_back(AZStd::move(handle));
        }

        EXPECT_EQ(testArray.size(), s_testCount);

        StableDynamicStructOfArraysMetrics metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount);

        // remove half of the elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }

        EXPECT_EQ(testArray.size(), s_testCount / 2);

        metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount / 2);

        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicStructOfArraysTests, emplace_Free)
    {
        using namespace AZ;
        TestArrayType testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayType::Handle handle = testArray.emplace(AZStd::forward_as_tuple(i, static_cast<float>(i)), AZStd::forward_as_tuple(i));
            handles.push_back(AZStd::move(handle));
        }

        StableDynamicStructOfArraysMetrics metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount);

        // remove half of the elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            handles.at(i).Free();
        }
        metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount / 2);

        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicStructOfArraysTests, ReleaseEmptyPages)
    {
        using namespace AZ;
        TestArrayType testArray;

        // Test removing items at the end

        // fill with items
        TestItem item; // test lvalue insert
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            item.m_index = i;
            TestArrayType::Handle handle = testArray.insert(item, i);
            handles.push_back(AZStd::move(handle));
        }

        StableDynamicStructOfArraysMetrics metrics1 = testArray.GetMetrics();
        size_t fullPageCount = metrics1.m_elementsPerPage.size();

        // remove the last half of the elements
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.pop_back();
        }

        // release the pages at the end that are now empty
        testArray.ReleaseEmptyPages();

        // Defragmenting a handle should still work after releasing empty pages
        testArray.DefragmentHandle(handles.back());

        StableDynamicStructOfArraysMetrics metrics2 = testArray.GetMetrics();
        size_t endReducedPageCount = metrics2.m_elementsPerPage.size();

        // There should be fewer pages now than before
        EXPECT_LT(endReducedPageCount, fullPageCount);


        // Test removing All the items

        handles.clear(); // cleanup remaining handles.

        // release all the pages.
        testArray.ReleaseEmptyPages();

        StableDynamicStructOfArraysMetrics metrics3 = testArray.GetMetrics();
        size_t emptyPageCount = metrics3.m_elementsPerPage.size();

        // there should be 0 pages now.
        EXPECT_EQ(emptyPageCount, 0);


        // Test removing items from the beginning

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayType::Handle handle = testArray.emplace(AZStd::forward_as_tuple(i, static_cast<float>(i)), AZStd::forward_as_tuple(i));
            handles.push_back(AZStd::move(handle));
        }

        // remove the first half of the elements
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.at(i).Free();
        }

        // release the pages at the beginning that are now empty
        testArray.ReleaseEmptyPages();

        StableDynamicStructOfArraysMetrics metrics4 = testArray.GetMetrics();
        size_t beginReducedPageCount = metrics4.m_elementsPerPage.size();

        // There should be fewer pages now than before
        EXPECT_LT(beginReducedPageCount, fullPageCount);

        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicStructOfArraysTests, CheckForHolesBetweenPages)
    {
        constexpr size_t pageSize = 64;
        using namespace AZ;
        AZ::StableDynamicStructOfArrays<pageSize, StructOfArraysTestAllocator, TestItem, uint32_t> testArray;

        // fill with 10 pages of items
        TestItem item; // test lvalue insert
        for (uint32_t i = 0; i < pageSize * 10; ++i)
        {
            item.m_index = i;
            StableDynamicStructOfArrays<pageSize, StructOfArraysTestAllocator, TestItem, uint32_t>::Handle handle = testArray.insert(item, i);
            handles.push_back(AZStd::move(handle));
        }

        // Create a hole between the pages by releaseing items in a page
        for (uint32_t i = pageSize * 5; i < pageSize * 6; ++i)
        {
            handles.at(i).Free();
        }
        testArray.ReleaseEmptyPages();

        // Use this lambda to force the test array to think the first page may be empty
        auto markFirstPageAsEmpty = [&]()
        {
            // Also free an element in the first page, so that the first empty page is at the beginning
            testArray.erase(handles.at(0));
            // Fill the first page back up, so that any further operations will be forced to
            // iterate past the hole in search of the next available page
            {
                TestItem replacementItem;
                replacementItem.m_index = 0;
                StableDynamicStructOfArrays<pageSize, StructOfArraysTestAllocator, TestItem, uint32_t>::Handle handle = testArray.insert(replacementItem, 0);
                handles.at(0) = AZStd::move(handle);
            }
        };

        markFirstPageAsEmpty();

        // Each of these operations will attempt to iterate over all the pages
        // This test is validating that they do not crash because they are properly checking for holes
        testArray.ReleaseEmptyPages();
        markFirstPageAsEmpty();

        testArray.GetParallelRanges();
        testArray.GetMetrics();

        // Test insert
        handles.push_back(testArray.emplace(AZStd::forward_as_tuple(item), AZStd::forward_as_tuple(0)));
        markFirstPageAsEmpty();

        // Test defragment
        testArray.DefragmentHandle(handles.back());
        markFirstPageAsEmpty();

        // Test erase
        testArray.erase(handles.back());

        handles.clear();
    }

    TEST_F(StableDynamicStructOfArraysTests, DefragmentHandle)
    {
        using namespace AZ;
        TestArrayType testArray;
        StableDynamicStructOfArraysMetrics metrics;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicStructOfArrays<TestElementsPerPage, StructOfArraysTestAllocator, TestItem, uint32_t>::Handle handle =
                testArray.emplace(AZStd::forward_as_tuple(i, static_cast<float>(i)), AZStd::forward_as_tuple(i));
            TestItem& testItem = handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>();
            testItem.m_index = i;
            testItem.m_value = static_cast<float>(i);
            handle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() = i;
            handles.push_back(AZStd::move(handle));
        }

        metrics = testArray.GetMetrics();
        size_t pageCount1 = metrics.m_elementsPerPage.size();

        // remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            handles.at(i).Free();
        }

        // release shouldn't be able to do anything since every other element was removed
        testArray.ReleaseEmptyPages();

        metrics = testArray.GetMetrics();
        size_t pageCount2 = metrics.m_elementsPerPage.size();
        EXPECT_EQ(pageCount1, pageCount2);

        // compact the elements
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            testArray.DefragmentHandle(handles.at(i));
        }

        // now that the elements are compacted we should be able to remove some pages
        testArray.ReleaseEmptyPages();

        metrics = testArray.GetMetrics();
        size_t pageCount3 = metrics.m_elementsPerPage.size();
        EXPECT_LT(pageCount3, pageCount2);

        // The the defragmented handles should still have valid weak handles, as long as they are made after the defragmentation
        for (TestArrayType::Handle& handle : handles)
        {
            if (handle.IsValid())
            {
                TestArrayType::WeakHandle weakHandle = handle.GetWeakHandle();
                // The weak handle should be referring to the same data as the owning handle
                EXPECT_EQ(
                    handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index,
                    weakHandle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index);

                EXPECT_EQ(
                    handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value,
                    weakHandle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value);

                EXPECT_EQ(
                    handle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(),
                    weakHandle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>());
            }
        }

        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicStructOfArraysTests, Iterator)
    {
        using namespace AZ;
        TestArrayType testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayType::Handle handle = testArray.emplace(AZStd::forward_as_tuple(i, static_cast<float>(i)), AZStd::forward_as_tuple(i));
            handles.push_back(AZStd::move(handle));
        }

        // make sure the iterator hits each item
        size_t index = 0;
        bool success = true;

        for (auto& item : testArray)
        {
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index == index);
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value == static_cast<float>(index));
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
            ++index;
        }

        EXPECT_TRUE(success);

        success = true;
        // remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            handles.at(i).Free();
        }

        // now the iterator should hit every other item (starting at 1 since 0 was freed).
        index = 1;
        for (auto& item : testArray)
        {
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index == index);
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value == static_cast<float>(index));
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
            index += 2;
        }
        EXPECT_TRUE(success);

        // remove the first half completely so there are a bunch of empty pages to skip
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.at(i).Free();
        }

        // now the iterator should hit every other item after s_testCount / 2.
        success = true;
        index = s_testCount / 2 + 1;
        for (auto& item : testArray)
        {
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index == index);
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value == static_cast<float>(index));
            success = success && (item.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
            index += 2;
        }
        EXPECT_TRUE(success);
        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicStructOfArraysTests, ConstIterator)
    {
        using namespace AZ;
        TestArrayType testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayType::Handle handle = testArray.emplace(AZStd::forward_as_tuple(i, static_cast<float>(i)), AZStd::forward_as_tuple(i));
            handles.push_back(AZStd::move(handle));
        }

        // make sure the const iterator hits each item
        size_t index = 0;
        bool success = true;

        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index == index);
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value == static_cast<float>(index));
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
            ++index;
        }

        EXPECT_TRUE(success);

        success = true;
        // remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            handles.at(i).Free();
        }

        // now the iterator should hit every other item (starting at 1 since 0 was freed).
        index = 1;
        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index == index);
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value == static_cast<float>(index));
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
            index += 2;
        }
        EXPECT_TRUE(success);

        // remove the first half completely so there are a bunch of empty pages to skip
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.at(i).Free();
        }

        // now the iterator should hit every other item after s_testCount / 2.
        success = true;
        index = s_testCount / 2 + 1;
        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_index == index);
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().m_value == static_cast<float>(index));
            success = success && (it.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
            index += 2;
        }
        EXPECT_TRUE(success);
        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicStructOfArraysTests, PageIterator)
    {
        using namespace AZ;
        TestArrayType testArray;

        // Fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayType::Handle handle = testArray.emplace(AZStd::forward_as_tuple(i, static_cast<float>(i)), AZStd::forward_as_tuple(i));
            handles.push_back(AZStd::move(handle));
        }

        // Make sure the page iterators hits each item

        size_t index = 0;
        bool success = true;
        auto pageIterators = testArray.GetParallelRanges();

        for (auto iteratorPair : pageIterators)
        {
            for (auto iterator = iteratorPair.first; iterator != iteratorPair.second; ++iterator)
            {
                TestItem& item = iterator.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>();
                success = success && (item.m_index == index);
                success = success && (item.m_value == static_cast<float>(index));
                success = success && (iterator.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
                ++index;
            }
        }
        EXPECT_TRUE(success);

        success = true;
        // Remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            handles.at(i).Free();
        }

        // Now the page iterators should hit every other item (starting at 1 since 0 was freed).
        index = 1;
        pageIterators = testArray.GetParallelRanges();
        for (auto iteratorPair : pageIterators)
        {
            for (auto iterator = iteratorPair.first; iterator != iteratorPair.second; ++iterator)
            {
                TestItem& item = iterator.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>();
                success = success && (item.m_index == index);
                success = success && (item.m_value == static_cast<float>(index));
                success = success && (iterator.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
                index += 2;
            }
        }
        EXPECT_TRUE(success);

        // Remove the first half completely so there are a bunch of empty pages to skip
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.at(i).Free();
        }

        // Now the page iterators should hit every other item after s_testCount / 2.
        // By this passing, it provesthe first few page iterrators begin and end are equal (as they should be for empty pages).
        success = true;
        index = s_testCount / 2 + 1;
        pageIterators = testArray.GetParallelRanges();

        for (auto iteratorPair : pageIterators)
        {
            for (auto iterator = iteratorPair.first; iterator != iteratorPair.second; ++iterator)
            {
                TestItem& item = iterator.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>();
                success = success && (item.m_index == index);
                success = success && (item.m_value == static_cast<float>(index));
                success = success && (iterator.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() == index);
                index += 2;
            }
        }
        EXPECT_TRUE(success);
        handles.clear(); // cleanup remaining handles.
    }



    // Fixture for testing handles and ensuring the correct number of objects are created, modified, and/or destroyed
    class StableDynamicStructOfArraysHandleTests
        : public UnitTest::LeakDetectionFixture
    {
        friend class StableDynamicStructOfArraysOwner;
        friend class MoveTest;
    public:
        void SetUp() override
        {
            s_testItemsConstructed = 0;
            s_testItemsDestructed = 0;
            s_testItemsModified = 0;
        }

        // Used to keep track of the number of times a constructor/destructor/function is called
        // to validate that TestItems are being properly created, destroyed, and modified even when accessed via an interface
        static int s_testItemsConstructed;
        static int s_testItemsDestructed;
        static int s_testItemsModified;
    };

    int StableDynamicStructOfArraysHandleTests::s_testItemsConstructed = 0;
    int StableDynamicStructOfArraysHandleTests::s_testItemsDestructed = 0;
    int StableDynamicStructOfArraysHandleTests::s_testItemsModified = 0;

    // Class used to test that the right number of items are created, modified, and destroyed
    // Follows similar pattern to what a FeatureProcessor might do
    class StableDynamicStructOfArraysOwner
    {

    public:
        class TestItemInterface
        {
        public:
            AZ_RTTI(TestItemInterface, "{96502D93-8FBC-4492-B3F8-9962D9E6A93B}");

            virtual void SetValue(int value) = 0;
            virtual int GetValue() const = 0;
        };

        class TestItemImplementation
            : public TestItemInterface
        {
        public:
            AZ_RTTI(TestItemImplementation, "{AFE3A7B6-2133-4206-BF91-0E1BB38FC2D1}", TestItemInterface);

            TestItemImplementation(int value)
                : m_value(value)
            {
                StableDynamicStructOfArraysHandleTests::s_testItemsConstructed++;
            }

            TestItemImplementation(const TestItemImplementation& testItem)
                : m_value(testItem.m_value)
            {
                StableDynamicStructOfArraysHandleTests::s_testItemsConstructed++;
            }

            virtual ~TestItemImplementation()
            {
                StableDynamicStructOfArraysHandleTests::s_testItemsDestructed++;
            }

            void SetValue(int value) override
            {
                m_value = value;
                StableDynamicStructOfArraysHandleTests::s_testItemsModified++;
            }

            int GetValue() const override
            {
                return m_value;
            }

        private:
            int m_value = 0;
        };
        using OwnerTestArrayType = AZ::StableDynamicStructOfArrays<TestElementsPerPage, StructOfArraysTestAllocator, StableDynamicStructOfArraysOwner::TestItemImplementation, uint32_t>;
    
        OwnerTestArrayType::Handle AcquireItem(int value, int otherValue)
        {
            return m_testArray.emplace(AZStd::forward_as_tuple(value), AZStd::forward_as_tuple(otherValue));
        }

        void ReleaseItem(OwnerTestArrayType::Handle& handle)
        {
            m_testArray.erase(handle);
        }

    public:
        OwnerTestArrayType m_testArray;
    };

    // This class runs several scenarios around transferring ownership from one handle to another
    template<typename SourceTestItemType>
    class SoAMoveTests
    {
        using SourceHandle = StableDynamicStructOfArraysOwner::OwnerTestArrayType::Handle;
    public:

        void MoveValidSourceToNullDestination_ExpectMoveToSucceed()
        {
            {
                StableDynamicStructOfArraysOwner owner;

                SourceHandle source = owner.AcquireItem(123, 124);
                SourceHandle destination = AZStd::move(source);

                // Source handle should be invalid after move, destination handle should be valid
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), true);
                EXPECT_EQ(destination.IsNull(), false);

                // The destination handle should have the value that came from the source handle
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), 123);
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), 124);

                // The destination handle should be pointing to real data that can be modified
                destination.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().SetValue(789);
                destination.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() = 788;
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), 789);
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), 788);

                // One item was constructed, none destructed, one modified
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 1);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 0);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsModified, 1);
            }
            EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, StableDynamicStructOfArraysHandleTests::s_testItemsDestructed);
        }

        void MoveValidSourceToValidDestination_ExpectMoveToSucceed()
        {
            {
                StableDynamicStructOfArraysOwner owner;

                SourceHandle source = owner.AcquireItem(123, 124);
                SourceHandle destination = owner.AcquireItem(456, 457);
                destination = AZStd::move(source);

                // Source handle should be invalid after move, destination handle should be valid
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), true);
                EXPECT_EQ(destination.IsNull(), false);

                // The destination handle should have the value that came from the source handle
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), 123);
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), 124);

                // The destination handle should be pointing to real data that can be modified
                destination.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().SetValue(789);
                destination.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() = 788;
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), 789);
                EXPECT_EQ(destination.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), 788);

                // Two items were constructed, one destructed, one modified
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 1);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsModified, 1);
            }
            EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, StableDynamicStructOfArraysHandleTests::s_testItemsDestructed);
        }

        void MoveNullSourceToValidDestination_ExpectMoveToSucceed()
        {
            {
                StableDynamicStructOfArraysOwner owner;
                
                SourceHandle source;
                SourceHandle destination = owner.AcquireItem(456, 457);
                destination = AZStd::move(source);

                // Both handles should be invalid after move
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), false);
                EXPECT_EQ(destination.IsNull(), true);

                // One item was constructed and destructed
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 1);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 1);
            }
            EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, StableDynamicStructOfArraysHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByOwner_ExpectMoveToSucceed()
        {
            {
                StableDynamicStructOfArraysOwner owner;

                SourceHandle source = owner.AcquireItem(123, 124);
                SourceHandle destination = owner.AcquireItem(456, 457);
                destination = AZStd::move(source);

                // Attempting to release the invalid source handle should be a no-op
                owner.ReleaseItem(source);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 1);

                // Releasing the valid destination handle should succeed
                owner.ReleaseItem(destination);
                EXPECT_FALSE(destination.IsValid());
                EXPECT_TRUE(destination.IsNull());
                // One item was constructed and destructed
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 2);
            }
            EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, StableDynamicStructOfArraysHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed()
        {
            {
                StableDynamicStructOfArraysOwner owner;

                SourceHandle source = owner.AcquireItem(123, 124);
                SourceHandle destination = owner.AcquireItem(456, 457);
                destination = AZStd::move(source);

                // Attempting to release the invalid source handle should be a no-op
                source.Free();
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 1);

                // Releasing the valid destination handle should succeed
                destination.Free();
                EXPECT_FALSE(destination.IsValid());
                EXPECT_TRUE(destination.IsNull());
                // One item was constructed and destructed
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 2);
            }
            EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, StableDynamicStructOfArraysHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed()
        {
            {
                StableDynamicStructOfArraysOwner owner;
                {
                    SourceHandle destination = owner.AcquireItem(456, 457);
                    {
                        SourceHandle source = owner.AcquireItem(123, 124);
                        destination = AZStd::move(source);
                    }
                    // Letting the invalid source item go out of scope should be a no-op 
                    EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                    EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 1);
                    EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsModified, 0);
                }

                // Releasing the valid destination handle by letting it go out of scope should succeed
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsDestructed, 2);
                EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsModified, 0);
            }
            EXPECT_EQ(StableDynamicStructOfArraysHandleTests::s_testItemsConstructed, StableDynamicStructOfArraysHandleTests::s_testItemsDestructed);
        }
    };

    // Move TestItem->TestItem
    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandle_FromValidSoATestItemHandleToNullSoATestItemHandle_SourceTestItemMovedToDestination)
    {
        SoAMoveTests<StableDynamicStructOfArraysOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToNullDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandle_FromValidSoATestItemHandleToValidSoATestItemHandle_DestinationTestItemReleasedThenSourceTestItemMoved)
    {
        SoAMoveTests<StableDynamicStructOfArraysOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandle_FromNullSoATestItemHandleToValidSoATestItemHandle_DestinationTestItemReleased)
    {
        SoAMoveTests<StableDynamicStructOfArraysOwner::TestItemImplementation> moveTest;
        moveTest.MoveNullSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandleAndReleaseByOwner_FromValidSoATestItemHandleToValidSoATestItemHandle_DestinationTestItemReleased)
    {
        SoAMoveTests<StableDynamicStructOfArraysOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByOwner_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_FromValidSoATestItemHandleToValidSoATestItemHandle_DestinationTestItemReleased)
    {
        SoAMoveTests<StableDynamicStructOfArraysOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandleAndReleaseByLettingHandleGoOutOfScope_FromValidSoATestItemHandleToValidSoATestItemHandle_DestinationTestItemReleased)
    {
        SoAMoveTests<StableDynamicStructOfArraysOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicStructOfArraysHandleTests, MoveHandle_SelfAssignment_DoesNotModifyHandle)
    {
        StableDynamicStructOfArraysOwner owner;
        StableDynamicStructOfArraysOwner::OwnerTestArrayType::Handle handle = owner.AcquireItem(1, 2);
        int testValue = 12;
        handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().SetValue(testValue);
        handle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() = testValue + 1;

        // Self assignment should not invalidate the handle
        handle = AZStd::move(handle);
        EXPECT_TRUE(handle.IsValid());
        EXPECT_FALSE(handle.IsNull());
        EXPECT_EQ(handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), testValue);
        EXPECT_EQ(handle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), testValue + 1);
    }
    
    TEST_F(StableDynamicStructOfArraysHandleTests, WeakHandle_GetDataFromOwner_CanAccessData)
    {
        StableDynamicStructOfArraysOwner owner;
        StableDynamicStructOfArraysOwner::OwnerTestArrayType::Handle handle = owner.AcquireItem(1, 2);
        StableDynamicStructOfArraysOwner::OwnerTestArrayType::WeakHandle weakHandle = handle.GetWeakHandle();

        int testValue = 12;
        weakHandle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().SetValue(testValue);
        weakHandle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>() = testValue + 1;

        // Validate the value referenced by the owning handle changed when the data was set via the weak handle
        EXPECT_EQ(handle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), testValue);
        EXPECT_EQ(handle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), testValue + 1);

        // Validate the value referenced by the weak handle changed when the data was set via the weak handle
        EXPECT_EQ(weakHandle.GetItem<TestStableDynamicStructOfArraysRows::TestItemIndex>().GetValue(), testValue);
        EXPECT_EQ(weakHandle.GetItem<TestStableDynamicStructOfArraysRows::UInt32Index>(), testValue + 1);

        // validate operator*
        EXPECT_EQ(AZStd::get<TestStableDynamicStructOfArraysRows::TestItemIndex>(*handle)->GetValue(), testValue);
        EXPECT_EQ(*AZStd::get<TestStableDynamicStructOfArraysRows::UInt32Index>(*handle), testValue + 1);
        EXPECT_EQ(AZStd::get<TestStableDynamicStructOfArraysRows::TestItemIndex>(*weakHandle)->GetValue(), testValue);
        EXPECT_EQ(*AZStd::get<TestStableDynamicStructOfArraysRows::UInt32Index>(*weakHandle), testValue + 1);
    }

}
