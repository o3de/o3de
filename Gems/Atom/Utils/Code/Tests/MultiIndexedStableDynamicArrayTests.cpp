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
#include <Atom/Utils/MultiIndexedStableDynamicArray.h>

namespace UnitTest
{
    enum TestMultiIndexedStableDynamicArrayRows
    {
        TestItemIndex = 0,
        UInt32Index
    };

    // Fixture that creates a bare-bones app
    constexpr size_t TestElementsPerPage = 512;
    using MultiIndexedTestAllocator = AZ::OSAllocator;

    struct MultiIndexedStableDynamicArrayTestsTestItem
    {
        MultiIndexedStableDynamicArrayTestsTestItem() = default;
        MultiIndexedStableDynamicArrayTestsTestItem(uint32_t value)
            : index(value)
        {
        }
        uint32_t index = 0;
    };

    using TestArrayHandle = AZ::MultiIndexedStableDynamicArray<TestElementsPerPage, MultiIndexedTestAllocator, MultiIndexedStableDynamicArrayTestsTestItem, uint32_t>::Handle;
    class MultiIndexedStableDynamicArrayTests
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

            handles.reserve(s_testCount);
        }

        void TearDown() override
        {
            for (TestArrayHandle& handle : handles)
            {
                handle.Free();
            }

            handles = AZStd::vector<TestArrayHandle>(); // force memory deallocation.

            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            LeakDetectionFixture::TearDown();
        }

        using TestItem = MultiIndexedStableDynamicArrayTestsTestItem;

        static constexpr uint32_t s_testCount = 1000000;

        AZStd::vector<AZ::MultiIndexedStableDynamicArray<TestElementsPerPage, MultiIndexedTestAllocator, TestItem, uint32_t>::Handle> handles;

        AZ::MultiIndexedStableDynamicArray<TestElementsPerPage, MultiIndexedTestAllocator, TestItem, uint32_t> testArray;
    };

    TEST_F(MultiIndexedStableDynamicArrayTests, insert_erase)
    {
        using namespace AZ;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle =
                testArray.insert({ i }, { i });
            handles.push_back(AZStd::move(handle));
        }

        EXPECT_EQ(testArray.size(), s_testCount);

        MultiIndexedStableDynamicArrayMetrics metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount);

        // remove half of the elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }

        EXPECT_EQ(testArray.size(), s_testCount / 2);

        metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount / 2);
    }

    TEST_F(MultiIndexedStableDynamicArrayTests, emplace_Free)
    {
        using namespace AZ;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle = testArray.insert(TestItem{ i }, i);
            handles.push_back(AZStd::move(handle));
        }

        MultiIndexedStableDynamicArrayMetrics metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount);

        // remove half of the elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }
        metrics = testArray.GetMetrics();
        EXPECT_EQ(metrics.m_totalElements, s_testCount / 2);
    }

    TEST_F(MultiIndexedStableDynamicArrayTests, ReleaseEmptyPages)
    {
        using namespace AZ;

        // Test removing items at the end

        // fill with items
        TestItem item; // test lvalue insert
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            item.index = i;
            TestArrayHandle handle = testArray.insert(item, i);
            handles.push_back(AZStd::move(handle));
        }

        MultiIndexedStableDynamicArrayMetrics metrics1 = testArray.GetMetrics();
        size_t fullPageCount = metrics1.m_elementsPerPage.size();

        // remove the last half of the elements
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.pop_back();
        }

        // release the pages at the end that are now empty
        testArray.ReleaseEmptyPages();

        MultiIndexedStableDynamicArrayMetrics metrics2 = testArray.GetMetrics();
        size_t endReducedPageCount = metrics2.m_elementsPerPage.size();

        // There should be fewer pages now than before
        EXPECT_LT(endReducedPageCount, fullPageCount);


        // Test removing All the items

        for (TestArrayHandle& handle : handles)
        {
            handle.Free();
        }

        handles.clear(); // cleanup remaining handles.

        // release all the pages.
        testArray.ReleaseEmptyPages();

        MultiIndexedStableDynamicArrayMetrics metrics3 = testArray.GetMetrics();
        size_t emptyPageCount = metrics3.m_elementsPerPage.size();

        // there should be 0 pages now.
        EXPECT_EQ(emptyPageCount, 0);


        // Test removing items from the beginning

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle = testArray.insert(i, i);
            handles.push_back(AZStd::move(handle));
        }

        // remove the first half of the elements
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            testArray.erase(handles.at(i));
        }

        // release the pages at the beginning that are now empty
        testArray.ReleaseEmptyPages();

        MultiIndexedStableDynamicArrayMetrics metrics4 = testArray.GetMetrics();
        size_t beginReducedPageCount = metrics4.m_elementsPerPage.size();

        // There should be fewer pages now than before
        EXPECT_LT(beginReducedPageCount, fullPageCount);
    }
    /*
    TEST_F(MultiIndexedStableDynamicArrayTests, DefragmentHandle)
    {
        using namespace AZ;
        AZ::MultiIndexedStableDynamicArray<TestElementsPerPage, MultiIndexedTestAllocator, TestItem, uint32_t> testArray;
        MultiIndexedStableDynamicArrayMetrics metrics;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle = testArray.insert(i, i);
            handles.push_back(AZStd::move(handle));
        }

        metrics = testArray.GetMetrics();
        size_t pageCount1 = metrics.m_elementsPerPage.size();

        // remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }

        // release shouldn't be able to do anything since ever other element was removed
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

        handles.clear(); // cleanup remaining handles.
    }
    */

    TEST_F(MultiIndexedStableDynamicArrayTests, Iterator)
    {
        using namespace AZ;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle = testArray.insert(i, i);
            handles.push_back(AZStd::move(handle));
        }

        // make sure the iterator hits each item
        size_t index = 0;
        bool success = true;

        for (TestArrayHandle item : testArray)
        {
            success = success && (item.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().index == index);
            ++index;
        }

        EXPECT_TRUE(success);

        success = true;
        // remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }

        // now the iterator should hit every other item (starting at 1 since 0 was freed).
        index = 1;
        for (TestArrayHandle item : testArray)
        {
            success = success && (item.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().index == index);
            index += 2;
        }
        EXPECT_TRUE(success);

        // remove the first half completely so there are a bunch of empty pages to skip
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            testArray.erase(handles.at(i));
        }

        // now the iterator should hit every other item after s_testCount / 2.
        success = true;
        index = s_testCount / 2 + 1;
        for (TestArrayHandle item : testArray)
        {
            success = success && (item.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().index == index);
            index += 2;
        }
        EXPECT_TRUE(success);
    }

    TEST_F(MultiIndexedStableDynamicArrayTests, ConstIterator)
    {
        using namespace AZ;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle = testArray.insert(i, i);
            handles.push_back(AZStd::move(handle));
        }

        // make sure the const iterator hits each item
        size_t index = 0;
        bool success = true;

        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().index == index);
            ++index;
        }

        EXPECT_TRUE(success);

        success = true;
        // remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }

        // now the iterator should hit every other item (starting at 1 since 0 was freed).
        index = 1;
        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().index == index);
            index += 2;
        }
        EXPECT_TRUE(success);

        // remove the first half completely so there are a bunch of empty pages to skip
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            testArray.erase(handles.at(i));
        }

        // now the iterator should hit every other item after s_testCount / 2.
        success = true;
        index = s_testCount / 2 + 1;
        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().index == index);
            index += 2;
        }
        EXPECT_TRUE(success);
    }

    TEST_F(MultiIndexedStableDynamicArrayTests, PageIterator)
    {
        using namespace AZ;

        // Fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            TestArrayHandle handle = testArray.insert(i, i);
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
                TestItem& item = iterator.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>();
                success = success && (item.index == index);
                ++index;
            }
        }
        EXPECT_TRUE(success);

        success = true;
        // Remove every other elements
        for (uint32_t i = 0; i < s_testCount; i += 2)
        {
            testArray.erase(handles.at(i));
        }

        // Now the page iterators should hit every other item (starting at 1 since 0 was freed).
        index = 1;
        pageIterators = testArray.GetParallelRanges();
        for (auto iteratorPair : pageIterators)
        {
            for (auto iterator = iteratorPair.first; iterator != iteratorPair.second; ++iterator)
            {
                TestItem& item = iterator.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>();
                success = success && (item.index == index);
                index += 2;
            }
        }
        EXPECT_TRUE(success);

        // Remove the first half completely so there are a bunch of empty pages to skip
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            testArray.erase(handles.at(i));
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
                TestItem& item = iterator.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>();
                success = success && (item.index == index);
                index += 2;
            }
        }
        EXPECT_TRUE(success);
    }



    // Fixture for testing handles and ensuring the correct number of objects are created, modified, and/or destroyed
    class MultiIndexedStableDynamicArrayHandleTests
        : public UnitTest::LeakDetectionFixture
    {
        friend class MultiIndexedStableDynamicArrayOwner;
        friend class MoveTest;
    public:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            s_testItemsConstructed = 0;
            s_testItemsDestructed = 0;
            s_testItemsModified = 0;
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        }
        // Used to keep track of the number of times a constructor/destructor/function is called
        // to validate that TestItems are being properly created, destroyed, and modified even when accessed via an interface
        static int s_testItemsConstructed;
        static int s_testItemsDestructed;
        static int s_testItemsModified;
    };

    int MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed = 0;
    int MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed = 0;
    int MultiIndexedStableDynamicArrayHandleTests::s_testItemsModified = 0;

    // Class used to test that the right number of items are created, modified, and destroyed
    // Follows similar pattern to what a FeatureProcessor might do
    class MultiIndexedStableDynamicArrayOwner
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
                MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed++;
            }

            virtual ~TestItemImplementation()
            {
                MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed++;
            }

            void SetValue(int value) override
            {
                m_value = value;
                MultiIndexedStableDynamicArrayHandleTests::s_testItemsModified++;
            }

            int GetValue() const override
            {
                return m_value;
            }

        private:
            int m_value = 0;
        };

        class TestItemImplementation2
            : public TestItemInterface
        {
        public:
            AZ_RTTI(TestItemImplementation2, "{F9B94C63-88C2-459C-B752-5963D263C97D}", TestItemInterface);

            TestItemImplementation2(int value)
                : m_value(value)
            {
            }

            virtual ~TestItemImplementation2()
            {
            }

            void SetValue(int value) override
            {
                m_value = value;
            }

            int GetValue() const override
            {
                return m_value;
            }

        private:
            int m_value = 0;
        };

        class TestItemImplementationUnrelated
        {
        public:
            AZ_RTTI(TestItemImplementationUnrelated, "{C583B659-E187-4355-82F9-310A97D4E35B}");

            TestItemImplementationUnrelated(int value)
                : m_value(value)
            {
            }

            virtual ~TestItemImplementationUnrelated()
            {
            }

            void SetValue(int value)
            {
                m_value = value;
            }

            int GetValue() const
            {
                return m_value;
            }

        private:
            int m_value = 0;
        };

        AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, TestItemImplementation> AcquireItem(
            uint32_t value)
        {
            return m_testArray.emplace(TestItemImplementation{ static_cast<int>(value) });
        }

        void ReleaseItem(
            AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, TestItemImplementation>& handle)
        {
            m_testArray.erase(handle);
        }

    private:
        AZ::MultiIndexedStableDynamicArray<TestElementsPerPage, MultiIndexedTestAllocator, TestItemImplementation> m_testArray;
    };

    using MultiIndexedTestItemInterfaceHandle = AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, MultiIndexedStableDynamicArrayOwner::TestItemInterface>;
    using MultiIndexedTestItemHandle = AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, MultiIndexedStableDynamicArrayOwner::TestItemImplementation>;
    using MultiIndexedTestItemHandleSibling = AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, MultiIndexedStableDynamicArrayOwner::TestItemImplementation2>;
    using MultiIndexedTestItemHandleUnrelated = AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, MultiIndexedStableDynamicArrayOwner::TestItemImplementationUnrelated>;

    // This class runs several scenarios around transferring ownership from one handle to another
    template<typename SourceTestItemType>
    class MultiIndexedMoveTests
    {
        using SourceHandle = AZ::MultiIndexedStableDynamicArrayHandle<TestElementsPerPage, MultiIndexedTestAllocator, SourceTestItemType>;
    public:

        void MoveValidSourceToNullDestination_ExpectMoveToSucceed()
        {
            {
                MultiIndexedStableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                SourceHandle destination = AZStd::move(source);

                // Source handle should be invalid after move, destination handle should be valid
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), true);
                EXPECT_EQ(destination.IsNull(), false);

                // The destination handle should have the value that came from the source handle
                SourceTestItemType& testItem = destination.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>();
                EXPECT_EQ(testItem.GetValue(), 123);

                // The destination handle should be pointing to real data that can be modified
                destination.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>() = 789;
                EXPECT_EQ(destination.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().GetValue(), 789);

                // One item was constructed, none destructed, one modified
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 1);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 0);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsModified, 1);
            }
            EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveValidSourceToValidDestination_ExpectMoveToSucceed()
        {
            {
                MultiIndexedStableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                SourceHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Source handle should be invalid after move, destination handle should be valid
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), true);
                EXPECT_EQ(destination.IsNull(), false);

                // The destination handle should have the value that came from the source handle
                EXPECT_EQ(destination.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().GetValue(), 123);

                // The destination handle should be pointing to real data that can be modified
                destination.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>() = 789;
                EXPECT_EQ(destination.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().GetValue(), 789);

                // Two items were constructed, one destructed, one modified
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 1);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsModified, 1);
            }
            EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveNullSourceToValidDestination_ExpectMoveToSucceed()
        {
            {
                MultiIndexedStableDynamicArrayOwner owner;
                
                SourceHandle source;
                SourceHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Both handles should be invalid after move
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), false);
                EXPECT_EQ(destination.IsNull(), true);

                // One item was constructed and destructed
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 1);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 1);
            }
            EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByOwner_ExpectMoveToSucceed()
        {
            {
                MultiIndexedStableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                SourceHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Attempting to release the invalid source handle should be a no-op
                owner.ReleaseItem(source);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 1);

                // Releasing the valid destination handle should succeed
                owner.ReleaseItem(destination);
                EXPECT_FALSE(destination.IsValid());
                EXPECT_TRUE(destination.IsNull());
                // One item was constructed and destructed
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 2);
            }
            EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed()
        {
            {
                MultiIndexedStableDynamicArrayOwner owner;
                {
                    SourceHandle destination = owner.AcquireItem(456);
                    {
                        SourceHandle source = owner.AcquireItem(123);
                        destination = AZStd::move(source);
                    }
                    // Letting the invalid source item go out of scope should be a no-op 
                    EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                    EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 1);
                    EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsModified, 0);
                }

                // Releasing the valid destination handle by letting it go out of scope should succeed
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed, 2);
                EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsModified, 0);
            }
            EXPECT_EQ(MultiIndexedStableDynamicArrayHandleTests::s_testItemsConstructed, MultiIndexedStableDynamicArrayHandleTests::s_testItemsDestructed);
        }
    };

    // Move TestItem->TestItem
    TEST_F(MultiIndexedStableDynamicArrayHandleTests, MoveHandle_FromValidTestItemHandleToNullTestItemHandle_SourceTestItemMovedToDestination)
    {
        MultiIndexedMoveTests<MultiIndexedStableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToNullDestination_ExpectMoveToSucceed();
    }

    TEST_F(MultiIndexedStableDynamicArrayHandleTests, MoveHandle_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleasedThenSourceTestItemMoved)
    {
        MultiIndexedMoveTests<MultiIndexedStableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(MultiIndexedStableDynamicArrayHandleTests, MoveHandle_FromNullTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MultiIndexedMoveTests<MultiIndexedStableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveNullSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(MultiIndexedStableDynamicArrayHandleTests, MoveHandleAndReleaseByOwner_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MultiIndexedMoveTests<MultiIndexedStableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByOwner_ExpectMoveToSucceed();
    }

    TEST_F(MultiIndexedStableDynamicArrayHandleTests, MoveHandleAndReleaseByLettingHandleGoOutOfScope_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MultiIndexedMoveTests<MultiIndexedStableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed();
    }

    TEST_F(MultiIndexedStableDynamicArrayHandleTests, MoveHandle_SelfAssignment_DoesNotModifyHandle)
    {
        MultiIndexedStableDynamicArrayOwner owner;
        MultiIndexedTestItemHandle handle = owner.AcquireItem(1);
        int testValue = 12;
        handle.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>() = testValue;

        // Self assignment should not invalidate the handle
        handle = AZStd::move(handle);
        EXPECT_TRUE(handle.IsValid());
        EXPECT_FALSE(handle.IsNull());
        EXPECT_EQ(handle.GetItem<TestMultiIndexedStableDynamicArrayRows::TestItemIndex>().GetValue(), testValue);
    }
}
