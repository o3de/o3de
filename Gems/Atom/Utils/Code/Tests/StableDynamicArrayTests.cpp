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
#include <Atom/Utils/StableDynamicArray.h>

namespace UnitTest
{
    // Fixture that creates a bare-bones app

    class StableDynamicArrayTests
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
            handles = AZStd::vector<AZ::StableDynamicArray<TestItem>::Handle>(); // force memory deallocation.
            
            LeakDetectionFixture::TearDown();
        }

        struct TestItem
        {
            TestItem() = default;
            TestItem(uint32_t value) : index(value) {}
            uint32_t index = 0;
        };

        static constexpr uint32_t s_testCount = 1000000;

        AZStd::vector<AZ::StableDynamicArray<TestItem>::Handle> handles;
    };

    TEST_F(StableDynamicArrayTests, insert_erase)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.insert({});
            handle->index = i;
            handles.push_back(AZStd::move(handle));
        }

        EXPECT_EQ(testArray.size(), s_testCount);

        StableDynamicArrayMetrics metrics = testArray.GetMetrics();
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

    TEST_F(StableDynamicArrayTests, emplace_Free)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.emplace(i);
            handles.push_back(AZStd::move(handle));
        }

        StableDynamicArrayMetrics metrics = testArray.GetMetrics();
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

    TEST_F(StableDynamicArrayTests, ReleaseEmptyPages)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;

        // Test removing items at the end

        // fill with items
        TestItem item; // test lvalue insert
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            item.index = i;
            StableDynamicArray<TestItem>::Handle handle = testArray.insert(item);
            handles.push_back(AZStd::move(handle));
        }

        StableDynamicArrayMetrics metrics1 = testArray.GetMetrics();
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

        StableDynamicArrayMetrics metrics2 = testArray.GetMetrics();
        size_t endReducedPageCount = metrics2.m_elementsPerPage.size();

        // There should be fewer pages now than before
        EXPECT_LT(endReducedPageCount, fullPageCount);


        // Test removing All the items

        handles.clear(); // cleanup remaining handles.

        // release all the pages.
        testArray.ReleaseEmptyPages();

        StableDynamicArrayMetrics metrics3 = testArray.GetMetrics();
        size_t emptyPageCount = metrics3.m_elementsPerPage.size();

        // there should be 0 pages now.
        EXPECT_EQ(emptyPageCount, 0);


        // Test removing items from the beginning

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.emplace(i);
            handles.push_back(AZStd::move(handle));
        }

        // remove the first half of the elements
        for (uint32_t i = 0; i < s_testCount / 2; ++i)
        {
            handles.at(i).Free();
        }

        // release the pages at the beginning that are now empty
        testArray.ReleaseEmptyPages();

        StableDynamicArrayMetrics metrics4 = testArray.GetMetrics();
        size_t beginReducedPageCount = metrics4.m_elementsPerPage.size();

        // There should be fewer pages now than before
        EXPECT_LT(beginReducedPageCount, fullPageCount);

        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicArrayTests, CheckForHolesBetweenPages)
    {
        constexpr size_t pageSize = 64;
        using namespace AZ;
        AZ::StableDynamicArray<TestItem, pageSize> testArray;

        // fill with 10 pages of items
        TestItem item; // test lvalue insert
        for (uint32_t i = 0; i < pageSize * 10; ++i)
        {
            item.index = i;
            StableDynamicArray<TestItem>::Handle handle = testArray.insert(item);
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
                replacementItem.index = 0;
                StableDynamicArray<TestItem>::Handle handle = testArray.insert(replacementItem);
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
        handles.push_back(testArray.emplace(item));
        markFirstPageAsEmpty();

        // Test defragment
        testArray.DefragmentHandle(handles.back());
        markFirstPageAsEmpty();

        // Test erase
        testArray.erase(handles.back());

        handles.clear();
    }

    TEST_F(StableDynamicArrayTests, DefragmentHandle)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;
        StableDynamicArrayMetrics metrics;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.emplace(i);
            handle->index = i;
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

        // The the defragmented handles should still have valid weak handles
        for (StableDynamicArray<TestItem>::Handle& handle : handles)
        {
            if (handle.IsValid())
            {
                StableDynamicArrayWeakHandle<TestItem> weakHandle = handle.GetWeakHandle();
                // The weak handle should be referring to the same data as the owning handle
                EXPECT_EQ(handle->index, weakHandle->index);
            }
        }

        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicArrayTests, Iterator)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.emplace(i);
            handle->index = i;
            handles.push_back(AZStd::move(handle));
        }

        // make sure the iterator hits each item
        size_t index = 0;
        bool success = true;

        for (TestItem& item : testArray)
        {
            success = success && (item.index == index);
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
        for (TestItem& item : testArray)
        {
            success = success && (item.index == index);
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
        for (TestItem& item : testArray)
        {
            success = success && (item.index == index);
            index += 2;
        }
        EXPECT_TRUE(success);
        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicArrayTests, ConstIterator)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;

        // fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.emplace(i);
            handles.push_back(AZStd::move(handle));
        }

        // make sure the const iterator hits each item
        size_t index = 0;
        bool success = true;

        for (auto it = testArray.cbegin(); it != testArray.cend(); ++it)
        {
            success = success && (it->index == index);
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
            success = success && (it->index == index);
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
            success = success && (it->index == index);
            index += 2;
        }
        EXPECT_TRUE(success);
        handles.clear(); // cleanup remaining handles.
    }

    TEST_F(StableDynamicArrayTests, PageIterator)
    {
        using namespace AZ;
        AZ::StableDynamicArray<TestItem> testArray;

        // Fill with items
        for (uint32_t i = 0; i < s_testCount; ++i)
        {
            StableDynamicArray<TestItem>::Handle handle = testArray.emplace(i);
            handle->index = i;
            handles.push_back(AZStd::move(handle));
        }

        // Make sure the page iterators hits each item

        size_t index = 0;
        bool success = true;
        auto pageIterators = testArray.GetParallelRanges();

        for (auto iteratorPair : pageIterators)
        {
            for (auto iterator = iteratorPair.m_begin; iterator != iteratorPair.m_end; ++iterator)
            {
                TestItem& item = *iterator;
                success = success && (item.index == index);
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
            for (auto iterator = iteratorPair.m_begin; iterator != iteratorPair.m_end; ++iterator)
            {
                TestItem& item = *iterator;
                success = success && (item.index == index);
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
            for (auto iterator = iteratorPair.m_begin; iterator != iteratorPair.m_end; ++iterator)
            {
                TestItem& item = *iterator;
                success = success && (item.index == index);
                index += 2;
            }
        }
        EXPECT_TRUE(success);
        handles.clear(); // cleanup remaining handles.
    }



    // Fixture for testing handles and ensuring the correct number of objects are created, modified, and/or destroyed
    class StableDynamicArrayHandleTests
        : public UnitTest::LeakDetectionFixture
    {
        friend class StableDynamicArrayOwner;
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

    int StableDynamicArrayHandleTests::s_testItemsConstructed = 0;
    int StableDynamicArrayHandleTests::s_testItemsDestructed = 0;
    int StableDynamicArrayHandleTests::s_testItemsModified = 0;

    // Class used to test that the right number of items are created, modified, and destroyed
    // Follows similar pattern to what a FeatureProcessor might do
    class StableDynamicArrayOwner
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
                StableDynamicArrayHandleTests::s_testItemsConstructed++;
            }

            virtual ~TestItemImplementation()
            {
                StableDynamicArrayHandleTests::s_testItemsDestructed++;
            }

            void SetValue(int value) override
            {
                m_value = value;
                StableDynamicArrayHandleTests::s_testItemsModified++;
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

        AZ::StableDynamicArrayHandle<TestItemImplementation> AcquireItem(uint32_t value)
        {
            return m_testArray.emplace(value);
        }

        void ReleaseItem(AZ::StableDynamicArrayHandle<TestItemInterface>& interfaceHandle)
        {
            AZ::StableDynamicArrayHandle<TestItemImplementation> temp(AZStd::move(interfaceHandle));
            ReleaseItem(temp);
        }

        void ReleaseItem(AZ::StableDynamicArrayHandle<TestItemImplementation>& handle)
        {
            m_testArray.erase(handle);
        }

    private:
        AZ::StableDynamicArray<TestItemImplementation> m_testArray;
    };

    using TestItemInterfaceHandle = AZ::StableDynamicArrayHandle<StableDynamicArrayOwner::TestItemInterface>;
    using TestItemHandle = AZ::StableDynamicArrayHandle<StableDynamicArrayOwner::TestItemImplementation>;
    using TestItemWeakHandle = AZ::StableDynamicArrayWeakHandle<StableDynamicArrayOwner::TestItemImplementation>;
    using TestItemHandleSibling = AZ::StableDynamicArrayHandle<StableDynamicArrayOwner::TestItemImplementation2>;
    using TestItemHandleUnrelated = AZ::StableDynamicArrayHandle<StableDynamicArrayOwner::TestItemImplementationUnrelated>;

    // This class runs several scenarios around transferring ownership from one handle to another
    template<typename SourceTestItemType, typename DestinationTestItemType>
    class MoveTests
    {
        using SourceHandle = AZ::StableDynamicArrayHandle<SourceTestItemType>;
        using DestinationHandle = AZ::StableDynamicArrayHandle<DestinationTestItemType>;
    public:
        MoveTests() 
        { 
            AZ_Assert(SourceTestItemType::RTTI_IsContainType(DestinationTestItemType::RTTI_Type()) || DestinationTestItemType::RTTI_IsContainType(SourceTestItemType::RTTI_Type()), "These tests expect the transfer of ownership from one handle to the other will succeed, and should only be called with compatible types.");
        }

        void MoveValidSourceToNullDestination_ExpectMoveToSucceed()
        {
            {
                StableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                DestinationHandle destination = AZStd::move(source);

                // Source handle should be invalid after move, destination handle should be valid
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), true);
                EXPECT_EQ(destination.IsNull(), false);

                // The destination handle should have the value that came from the source handle
                EXPECT_EQ(destination->GetValue(), 123);

                // The destination handle should be pointing to real data that can be modified
                destination->SetValue(789);
                EXPECT_EQ(destination->GetValue(), 789);

                // One item was constructed, none destructed, one modified
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 1);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 0);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsModified, 1);
            }
            EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, StableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveValidSourceToValidDestination_ExpectMoveToSucceed()
        {
            {
                StableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                DestinationHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Source handle should be invalid after move, destination handle should be valid
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), true);
                EXPECT_EQ(destination.IsNull(), false);

                // The destination handle should have the value that came from the source handle
                EXPECT_EQ(destination->GetValue(), 123);

                // The destination handle should be pointing to real data that can be modified
                destination->SetValue(789);
                EXPECT_EQ(destination->GetValue(), 789);

                // Two items were constructed, one destructed, one modified
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 1);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsModified, 1);
            }
            EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, StableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveNullSourceToValidDestination_ExpectMoveToSucceed()
        {
            {
                StableDynamicArrayOwner owner;
                
                SourceHandle source;
                DestinationHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Both handles should be invalid after move
                EXPECT_EQ(source.IsValid(), false);
                EXPECT_EQ(source.IsNull(), true);
                EXPECT_EQ(destination.IsValid(), false);
                EXPECT_EQ(destination.IsNull(), true);

                // One item was constructed and destructed
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 1);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 1);
            }
            EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, StableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByOwner_ExpectMoveToSucceed()
        {
            {
                StableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                DestinationHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Attempting to release the invalid source handle should be a no-op
                owner.ReleaseItem(source);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 1);

                // Releasing the valid destination handle should succeed
                owner.ReleaseItem(destination);
                EXPECT_FALSE(destination.IsValid());
                EXPECT_TRUE(destination.IsNull());
                // One item was constructed and destructed
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 2);
            }
            EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, StableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed()
        {
            {
                StableDynamicArrayOwner owner;

                SourceHandle source = owner.AcquireItem(123);
                DestinationHandle destination = owner.AcquireItem(456);
                destination = AZStd::move(source);

                // Attempting to release the invalid source handle should be a no-op
                source.Free();
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 1);

                // Releasing the valid destination handle should succeed
                destination.Free();
                EXPECT_FALSE(destination.IsValid());
                EXPECT_TRUE(destination.IsNull());
                // One item was constructed and destructed
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 2);
            }
            EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, StableDynamicArrayHandleTests::s_testItemsDestructed);
        }

        void MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed()
        {
            {
                StableDynamicArrayOwner owner;
                {
                    DestinationHandle destination = owner.AcquireItem(456);
                    {
                        SourceHandle source = owner.AcquireItem(123);
                        destination = AZStd::move(source);
                    }
                    // Letting the invalid source item go out of scope should be a no-op 
                    EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                    EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 1);
                    EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsModified, 0);
                }

                // Releasing the valid destination handle by letting it go out of scope should succeed
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsDestructed, 2);
                EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsModified, 0);
            }
            EXPECT_EQ(StableDynamicArrayHandleTests::s_testItemsConstructed, StableDynamicArrayHandleTests::s_testItemsDestructed);
        }
    };

    // Move TestItem->TestItem
    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidTestItemHandleToNullTestItemHandle_SourceTestItemMovedToDestination)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToNullDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleasedThenSourceTestItemMoved)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromNullTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveNullSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByOwner_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByOwner_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByLettingHandleGoOutOfScope_FromValidTestItemHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed();
    }

    // Move TestItem->Interface
    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidTestItemHandleToNullInterfaceHandle_SourceTestItemMovedToDestination)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveValidSourceToNullDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidTestItemHandleToValidInterfaceHandle_DestinationTestItemReleasedThenSourceTestItemMoved)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveValidSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromNullTestItemHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveNullSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByOwner_FromValidTestItemHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveHandleAndReleaseByOwner_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_FromValidTestItemHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByLettingHandleGoOutOfScope_FromValidTestItemHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemImplementation, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed();
    }

    // Move Interface->TestItem
    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidInterfaceHandleToNullTestItemHandle_SourceTestItemMovedToDestination)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToNullDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidInterfaceHandleToValidTestItemHandle_DestinationTestItemReleasedThenSourceTestItemMoved)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveValidSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromNullInterfaceHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveNullSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByOwner_FromValidInterfaceHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByOwner_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_FromValidInterfaceHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByLettingHandleGoOutOfScope_FromValidInterfaceHandleToValidTestItemHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemImplementation> moveTest;
        moveTest.MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed();
    }

    // Move Interface->Interface
    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidInterfaceHandleToNullInterfaceHandle_SourceTestItemMovedToDestination)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveValidSourceToNullDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromValidInterfaceHandleToValidInterfaceHandle_DestinationTestItemReleasedThenSourceTestItemMoved)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveValidSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_FromNullInterfaceHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveNullSourceToValidDestination_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByOwner_FromValidInterfaceHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveHandleAndReleaseByOwner_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_FromValidInterfaceHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveHandleAndReleaseByCallingFreeDirectlyOnHandle_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandleAndReleaseByLettingHandleGoOutOfScope_FromValidInterfaceHandleToValidInterfaceHandle_DestinationTestItemReleased)
    {
        MoveTests<StableDynamicArrayOwner::TestItemInterface, StableDynamicArrayOwner::TestItemInterface> moveTest;
        moveTest.MoveHandleAndReleaseByLettingHandleGoOutOfScope_ExpectMoveToSucceed();
    }

    TEST_F(StableDynamicArrayHandleTests, MoveHandle_SelfAssignment_DoesNotModifyHandle)
    {
        StableDynamicArrayOwner owner;
        TestItemHandle handle = owner.AcquireItem(1);
        int testValue = 12;
        handle->SetValue(testValue);

        // Self assignment should not invalidate the handle
        handle = AZStd::move(handle);
        EXPECT_TRUE(handle.IsValid());
        EXPECT_FALSE(handle.IsNull());
        EXPECT_EQ(handle->GetValue(), testValue);
    }

    TEST_F(StableDynamicArrayHandleTests, WeakHandle_GetDataFromOwner_CanAccessData)
    {
        StableDynamicArrayOwner owner;
        TestItemHandle handle = owner.AcquireItem(1);
        TestItemWeakHandle weakHandle = handle.GetWeakHandle();

        int testValue = 12;
        weakHandle->SetValue(testValue);
        EXPECT_EQ(handle->GetValue(), testValue);
        EXPECT_EQ(weakHandle->GetValue(), testValue);
        EXPECT_EQ((*weakHandle).GetValue(), testValue);
    }

    //
    // Invalid cases
    //

    TEST_F(StableDynamicArrayHandleTests, MoveHandleBetweenDifferentTypes_FromInterfaceToASiblingHandle_AssertsAndLeavesBothHandlesInvalid)
    {
        {
            StableDynamicArrayOwner owner;

            // The the underlying type that the interface handle refers to is a TestItemImplementation
            TestItemInterfaceHandle interfaceHandle = owner.AcquireItem(1);

            AZ_TEST_START_ASSERTTEST;
            // The interface handle is referring to a TestItemImplementation, so you should not be able to move it to a handle to a TestItemImplementation2
            TestItemHandleSibling testItemHandle2FromInterface = AZStd::move(interfaceHandle);
            AZ_TEST_STOP_ASSERTTEST(1);
            EXPECT_FALSE(interfaceHandle.IsValid());
            EXPECT_TRUE(interfaceHandle.IsNull());
            EXPECT_FALSE(testItemHandle2FromInterface.IsValid());
            EXPECT_TRUE(testItemHandle2FromInterface.IsNull());
        }
        EXPECT_EQ(s_testItemsConstructed, s_testItemsDestructed);
    }


}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
