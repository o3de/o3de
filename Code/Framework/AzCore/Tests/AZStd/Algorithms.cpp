/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/utils.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/sort.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace UnitTest
{
#if !AZ_UNIT_TEST_SKIP_STD_ALGORITHMS_TESTS
    class Algorithms
        : public LeakDetectionFixture
    {
    };

    /**
     *
     */
    class AlgorithmsFindTest
        : public LeakDetectionFixture
    {
    public:
        static bool SearchCompare(int i1, int i2) { return (i1 == i2); }

        static bool IsFive(int i) { return i == 5; }
        static bool IsNinetyNine(int i) { return i == 99; }
        static bool IsOne(int i) { return i == 1; }
        static bool IsLessThanTen(int i) { return i < 10; }

        template<class T>
        static void Find_With_OneFiveRestOnes(const T& iterable)
        {
            AZ_TEST_ASSERT(AZStd::find(iterable.begin(), iterable.end(), 5) != iterable.end());
            AZ_TEST_ASSERT(*AZStd::find(iterable.begin(), iterable.end(), 5) == 5);
            AZ_TEST_ASSERT(AZStd::find(iterable.begin(), iterable.end(), 99) == iterable.end());
        }

        template<class T>
        static void FindIf_With_OneFiveRestOnes(const T& iterable)
        {
            AZ_TEST_ASSERT(AZStd::find_if(iterable.begin(), iterable.end(), IsFive) != iterable.end());
            AZ_TEST_ASSERT(*AZStd::find_if(iterable.begin(), iterable.end(), IsFive) == 5);
            AZ_TEST_ASSERT(AZStd::find_if(iterable.begin(), iterable.end(), IsNinetyNine) == iterable.end());
        }

        template<class T>
        static void FindIfNot_With_OneFiveRestOnes(const T& iterable)
        {
            AZ_TEST_ASSERT(AZStd::find_if_not(iterable.begin(), iterable.end(), IsOne) != iterable.end());
            AZ_TEST_ASSERT(*AZStd::find_if_not(iterable.begin(), iterable.end(), IsOne) == 5);
            AZ_TEST_ASSERT(AZStd::find_if_not(iterable.begin(), iterable.end(), IsLessThanTen) == iterable.end());
        }

        void run()
        {
            // search (default compare)
            AZStd::array<int, 9> searchArray = {
                {10, 20, 30, 40, 50, 60, 70, 80, 90}
            };
            AZStd::array<int, 4> searchDefault = {
                {40, 50, 60, 70}
            };
            AZStd::array<int, 9>::iterator it = AZStd::search(searchArray.begin(), searchArray.end(), searchDefault.begin(), searchDefault.end());
            AZ_TEST_ASSERT(it != searchArray.end());
            AZ_TEST_ASSERT(AZStd::distance(searchArray.begin(), it) == 3);

            AZStd::array<int, 4> searchCompare = {
                {20, 30, 50}
            };
            it = AZStd::search(searchArray.begin(), searchArray.end(), searchCompare.begin(), searchCompare.end(), SearchCompare);
            AZ_TEST_ASSERT(it == searchArray.end());

            // find, find_if, find_if_not
            AZStd::vector<int> emptyVector;
            AZStd::array<int, 1> singleFiveArray = {
                {5}
            };
            AZStd::array<int, 3> fiveAtStartArray = {
                {5, 1, 1}
            };
            AZStd::array<int, 3> fiveInMiddleArray = {
                {1, 5, 1}
            };
            AZStd::array<int, 3> fiveAtEndArray = {
                {1, 1, 5}
            };

            AZ_TEST_ASSERT(AZStd::find(emptyVector.begin(), emptyVector.end(), 5) == emptyVector.end());
            Find_With_OneFiveRestOnes(singleFiveArray);
            Find_With_OneFiveRestOnes(fiveAtStartArray);
            Find_With_OneFiveRestOnes(fiveInMiddleArray);
            Find_With_OneFiveRestOnes(fiveAtEndArray);

            AZ_TEST_ASSERT(AZStd::find_if(emptyVector.begin(), emptyVector.end(), IsFive) == emptyVector.end());
            FindIf_With_OneFiveRestOnes(singleFiveArray);
            FindIf_With_OneFiveRestOnes(fiveAtStartArray);
            FindIf_With_OneFiveRestOnes(fiveInMiddleArray);
            FindIf_With_OneFiveRestOnes(fiveAtEndArray);

            AZ_TEST_ASSERT(AZStd::find_if_not(emptyVector.begin(), emptyVector.end(), IsFive) == emptyVector.end());
            FindIfNot_With_OneFiveRestOnes(singleFiveArray);
            FindIfNot_With_OneFiveRestOnes(fiveAtStartArray);
            FindIfNot_With_OneFiveRestOnes(fiveInMiddleArray);
            FindIfNot_With_OneFiveRestOnes(fiveAtEndArray);
        }
    };

    TEST_F(AlgorithmsFindTest, Test)
    {
        run();
    }

    /**
     *
     */
    static bool IsFive(int i) { return i == 5; }
    TEST_F(Algorithms, Compare)
    {
        AZStd::vector<int> emptyVector;
        AZStd::array<int, 3> allFivesArray = {
            {5, 5, 5}
        };
        AZStd::array<int, 3> noFivesArray  = {
            {0, 1, 2}
        };
        AZStd::array<int, 3> oneFiveArray  = {
            {4, 5, 6}
        };

        // all_of
        AZ_TEST_ASSERT(AZStd::all_of(emptyVector.begin(), emptyVector.end(), IsFive) == true);
        AZ_TEST_ASSERT(AZStd::all_of(allFivesArray.begin(), allFivesArray.end(), IsFive) == true);
        AZ_TEST_ASSERT(AZStd::all_of(noFivesArray.begin(), noFivesArray.end(), IsFive) == false);
        AZ_TEST_ASSERT(AZStd::all_of(oneFiveArray.begin(), oneFiveArray.end(), IsFive) == false);

        // any_of
        AZ_TEST_ASSERT(AZStd::any_of(emptyVector.begin(), emptyVector.end(), IsFive) == false);
        AZ_TEST_ASSERT(AZStd::any_of(allFivesArray.begin(), allFivesArray.end(), IsFive) == true);
        AZ_TEST_ASSERT(AZStd::any_of(noFivesArray.begin(), noFivesArray.end(), IsFive) == false);
        AZ_TEST_ASSERT(AZStd::any_of(oneFiveArray.begin(), oneFiveArray.end(), IsFive) == true);

        // none_of
        AZ_TEST_ASSERT(AZStd::none_of(emptyVector.begin(), emptyVector.end(), IsFive) == true);
        AZ_TEST_ASSERT(AZStd::none_of(allFivesArray.begin(), allFivesArray.end(), IsFive) == false);
        AZ_TEST_ASSERT(AZStd::none_of(noFivesArray.begin(), noFivesArray.end(), IsFive) == true);
        AZ_TEST_ASSERT(AZStd::none_of(oneFiveArray.begin(), oneFiveArray.end(), IsFive) == false);
    }

    /**
     *
     */
    TEST_F(Algorithms, Heap)
    {
        AZStd::array<int, 5> elementsSrc = { { 10, 20, 30, 5, 15 } };
        using ElementsType = AZStd::vector<int>;
        ElementsType elements(elementsSrc.begin(), elementsSrc.begin() + 5);

        AZStd::make_heap(elements.begin(), elements.end());
        ElementsType expected = { { 30, 20, 10, 5, 15 } };
        AZ_TEST_ASSERT(elements == expected);

        AZStd::pop_heap(elements.begin(), elements.end());
        elements.pop_back();
        expected = { { 20, 15, 10, 5 } };
        AZ_TEST_ASSERT(elements == expected);

        elements.push_back(99);
        AZStd::push_heap(elements.begin(), elements.end());
        expected = { { 99, 20, 10, 5, 15 } };
        AZ_TEST_ASSERT(elements == expected);

        AZStd::sort_heap(elements.begin(), elements.end());
        expected = { { 5, 10, 15, 20, 99 } };
        AZ_TEST_ASSERT(elements == expected);

        // Using comp
        AZStd::make_heap(elements.begin(), elements.end(), AZStd::greater<int>());
        expected = { { 5, 10, 15, 20, 99} };
        AZ_TEST_ASSERT(elements == expected);

        AZStd::pop_heap(elements.begin(), elements.end(), AZStd::greater<int>());
        elements.pop_back();
        expected = { { 10, 20, 15, 99 } };
        AZ_TEST_ASSERT(elements == expected);

        elements.push_back(100);
        AZStd::push_heap(elements.begin(), elements.end(), AZStd::greater<int>());
        expected = { { 10, 20, 15, 99, 100 } };
        AZ_TEST_ASSERT(elements == expected);

        AZStd::sort_heap(elements.begin(), elements.end(), AZStd::greater<int>());
        expected = { { 100, 99, 20, 15, 10 } };
        AZ_TEST_ASSERT(elements == expected);
    }

    TEST_F(Algorithms, InsertionSort)
    {
        AZStd::array<int, 10> elementsSrc = {
            { 10, 2, 6, 3, 5, 8, 7, 9, 1, 4 }
        };

        // Insertion sort
        AZStd::array<int, 10> elements1(elementsSrc);
        AZStd::insertion_sort(elements1.begin(), elements1.end());
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_LT(elements1[i - 1], elements1[i]);
        }
        insertion_sort(elements1.begin(), elements1.end(), AZStd::greater<int>());
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_GT(elements1[i - 1], elements1[i]);
        }
    }

    TEST_F(Algorithms, InsertionSort_SharedPtr)
    {
        AZStd::array<AZStd::shared_ptr<int>, 10> elementsSrc = {
        { 
            AZStd::make_shared<int>(10), AZStd::make_shared<int>(2), AZStd::make_shared<int>(6), AZStd::make_shared<int>(3),
            AZStd::make_shared<int>(5), AZStd::make_shared<int>(8), AZStd::make_shared<int>(7), AZStd::make_shared<int>(9),
            AZStd::make_shared<int>(1), AZStd::make_shared<int>(4) }
        };

        // Insertion sort
        auto compareLesser = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
        {
            return *lhs < *rhs;
        };
        AZStd::array<AZStd::shared_ptr<int>, 10> elements1(elementsSrc);
        AZStd::insertion_sort(elements1.begin(), elements1.end(), compareLesser);
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_LT(*elements1[i - 1], *elements1[i]);
        }

        auto compareGreater = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
        {
            return *lhs > *rhs;
        };
        insertion_sort(elements1.begin(), elements1.end(), compareGreater);
        for (size_t i = 1; i < elements1.size(); ++i)
        {
            EXPECT_GT(*elements1[i - 1], *elements1[i]);
        }
    }

    TEST_F(Algorithms, Sort)
    {
        AZStd::vector<int> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
                case 0:
                    vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                    break;
                case 1:
                    vectorSize = 32;     // exact size
                    break;
                case 2:
                    vectorSize = 64;     // double
                    break;
                case 3:
                    vectorSize = 100;     // just more
                    break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(i);
            }

            // Normal sort test
            AZStd::sort(sortTest.begin(), sortTest.end());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(sortTest[i - 1], sortTest[i]);
            }
            AZStd::sort(sortTest.begin(), sortTest.end(), AZStd::greater<int>());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(sortTest[i - 1], sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, Sort_SharedPtr)
    {
        AZStd::vector<AZStd::shared_ptr<int>> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
            case 0:
                vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                break;
            case 1:
                vectorSize = 32;     // exact size
                break;
            case 2:
                vectorSize = 64;     // double
                break;
            case 3:
                vectorSize = 100;     // just more
                break;
            }

            sortTest.clear();
            for (int i = vectorSize - 1; i >= 0; --i)
            {
                sortTest.push_back(AZStd::make_shared<int>(i));
            }

            // Normal sort test
            auto compareLesser = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
            {
                return *lhs < *rhs;
            };
            sort(sortTest.begin(), sortTest.end(), compareLesser);
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(*sortTest[i - 1], *sortTest[i]);
            }

            auto compareGreater = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
            {
                return *lhs > *rhs;
            };
            sort(sortTest.begin(), sortTest.end(), compareGreater);
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(*sortTest[i - 1], *sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, StableSort)
    {
        AZStd::vector<int> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
                case 0:
                    vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                    break;
                case 1:
                    vectorSize = 32;     // exact size
                    break;
                case 2:
                    vectorSize = 64;     // double
                    break;
                case 3:
                    vectorSize = 100;     // just more
                    break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(i);
            }

            // Stable sort test
            AZStd::stable_sort(sortTest.begin(), sortTest.end());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(sortTest[i - 1], sortTest[i]);
            }
            AZStd::stable_sort(sortTest.begin(), sortTest.end(), AZStd::greater<int>());
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(sortTest[i - 1], sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, StableSort_SharedPtr)
    {
        AZStd::vector<AZStd::shared_ptr<int>> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
            case 0:
                vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                break;
            case 1:
                vectorSize = 32;     // exact size
                break;
            case 2:
                vectorSize = 64;     // double
                break;
            case 3:
                vectorSize = 100;     // just more
                break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(AZStd::make_shared<int>(i));
            }

            // Stable sort test
            auto compareLesser = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
            {
                return *lhs < *rhs;
            };
            AZStd::stable_sort(sortTest.begin(), sortTest.end(), compareLesser);
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_LT(*sortTest[i - 1], *sortTest[i]);
            }

            auto compareGreater = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
            {
                return *lhs > *rhs;
            };
            AZStd::stable_sort(sortTest.begin(), sortTest.end(), compareGreater);
            for (size_t i = 1; i < sortTest.size(); ++i)
            {
                EXPECT_GT(*sortTest[i - 1], *sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, StableSort_AlreadySorted)
    {
        AZStd::vector<uint64_t> testVec;
        for (int i = 0; i < 33; ++i)
        {
            testVec.push_back(i);
        }

        auto expectedResultVec = testVec;
        AZStd::stable_sort(testVec.begin(), testVec.end(), AZStd::less<uint64_t>());
        EXPECT_EQ(expectedResultVec, testVec);
    }

    TEST_F(Algorithms, PartialSort)
    {
        AZStd::vector<int> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
                case 0:
                    vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                    break;
                case 1:
                    vectorSize = 32;     // exact size
                    break;
                case 2:
                    vectorSize = 64;     // double
                    break;
                case 3:
                    vectorSize = 100;     // just more
                    break;
            }

            sortTest.clear();
            for (int i = vectorSize-1; i >= 0; --i)
            {
                sortTest.push_back(i);
            }

            // partial_sort test
            int sortSize = vectorSize / 2;
            AZStd::partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end());
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_LT(sortTest[i - 1], sortTest[i]);
            }
            AZStd::partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end(), AZStd::greater<int>());
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_GT(sortTest[i - 1], sortTest[i]);
            }
        }
    }

    TEST_F(Algorithms, PartialSort_SharedPtr)
    {
        AZStd::vector<AZStd::shared_ptr<int>> sortTest;
        for (int iSizeTest = 0; iSizeTest < 4; ++iSizeTest)
        {
            int vectorSize = 0;
            switch (iSizeTest)
            {
            case 0:
                vectorSize = 15;     // less than insertion sort threshold (32 at the moment)
                break;
            case 1:
                vectorSize = 32;     // exact size
                break;
            case 2:
                vectorSize = 64;     // double
                break;
            case 3:
                vectorSize = 100;     // just more
                break;
            }

            sortTest.clear();
            for (int i = vectorSize - 1; i >= 0; --i)
            {
                sortTest.push_back(AZStd::make_shared<int>(i));
            }

            // partial_sort test
            auto compareLesser = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
            {
                return *lhs < *rhs;
            };
            int sortSize = vectorSize / 2;
            AZStd::partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end(), compareLesser);
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_LT(*sortTest[i - 1], *sortTest[i]);
            }

            auto compareGreater = [](const AZStd::shared_ptr<int>& lhs, const AZStd::shared_ptr<int>& rhs) -> bool
            {
                return *lhs > *rhs;
            };
            AZStd::partial_sort(sortTest.begin(), sortTest.begin() + sortSize, sortTest.end(), compareGreater);
            for (int i = 1; i < sortSize; ++i)
            {
                EXPECT_GT(*sortTest[i - 1], *sortTest[i]);
            }
        }
    }

    /**
     * Endian swap test.
     */
    TEST_F(Algorithms, EndianSwap)
    {
        AZStd::array<char, 10> charArr  = {
            {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
        };
        AZStd::array<short, 10> shortArr = {
            {0x0f01, 0x0f02, 0x0f03, 0x0f04, 0x0f05, 0x0f06, 0x0f07, 0x0f08, 0x0f09, 0x0f0a}
        };
        AZStd::array<int, 10> intArr = {
            {0x0d0e0f01, 0x0d0e0f02, 0x0d0e0f03, 0x0d0e0f04, 0x0d0e0f05, 0x0d0e0f06, 0x0d0e0f07, 0x0d0e0f08, 0x0d0e0f09, 0x0d0e0f0a}
        };
        AZStd::array<AZ::s64, 10> int64Arr = {
            {AZ_INT64_CONST(0x090a0b0c0d0e0f01), AZ_INT64_CONST(0x090a0b0c0d0e0f02), AZ_INT64_CONST(0x090a0b0c0d0e0f03), AZ_INT64_CONST(0x090a0b0c0d0e0f04),
             AZ_INT64_CONST(0x090a0b0c0d0e0f05), AZ_INT64_CONST(0x090a0b0c0d0e0f06), AZ_INT64_CONST(0x090a0b0c0d0e0f07),
             AZ_INT64_CONST(0x090a0b0c0d0e0f08), AZ_INT64_CONST(0x090a0b0c0d0e0f09), AZ_INT64_CONST(0x090a0b0c0d0e0f0a)}
        };

        AZStd::endian_swap(charArr.begin(), charArr.end());
        for (char i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(charArr[i - 1] == i);
        }

        AZStd::endian_swap(shortArr.begin(), shortArr.end());
        for (short i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(shortArr[i - 1] == (i << 8 | 0x000f));
        }

        AZStd::endian_swap(intArr.begin(), intArr.end());
        for (int i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(intArr[i - 1] == (i << 24 | 0x000f0e0d));
        }

        AZStd::endian_swap(int64Arr.begin(), int64Arr.end());
        for (AZ::s64 i = 1; i <= 10; ++i)
        {
            AZ_TEST_ASSERT(int64Arr[static_cast<AZStd::size_t>(i - 1)] == (i << 56 | AZ_INT64_CONST(0x000f0e0d0c0b0a09)));
        }
    }

    TEST_F(Algorithms, CopyBackwardFastCopy)
    {
        AZStd::array<int, 3> src = {{ 1, 2, 3 }};
        AZStd::array<int, 3> dest;

        AZStd::copy_backward(src.begin(), src.end(), dest.end());
        
        EXPECT_EQ(1, dest[0]);
        EXPECT_EQ(2, dest[1]);
        EXPECT_EQ(3, dest[2]);
    }

    TEST_F(Algorithms, CopyBackwardStandardCopy)
    {
        // List is not contiguous, and is therefore unable to perform a fast copy
        AZStd::list<int> src = { 1, 2, 3 };
        AZStd::array<int, 3> dest;

        AZStd::copy_backward(src.begin(), src.end(), dest.end());
        
        EXPECT_EQ(1, dest[0]);
        EXPECT_EQ(2, dest[1]);
        EXPECT_EQ(3, dest[2]);
    }

    TEST_F(Algorithms, ReverseCopy)
    {
        AZStd::array<int, 3> src = {{ 1, 2, 3 }};
        AZStd::array<int, 3> dest;

        AZStd::reverse_copy(src.begin(), src.end(), dest.begin());
        EXPECT_EQ(3, dest[0]);
        EXPECT_EQ(2, dest[1]);
        EXPECT_EQ(1, dest[2]);
    }

    TEST_F(Algorithms, MinMaxElement)
    {
        AZStd::initializer_list<uint32_t> emptyData{};
        AZStd::array<uint32_t, 1> singleElementData{ { 313 } };
        AZStd::array<uint32_t, 3> unorderedData{ { 5, 3, 2} };
        AZStd::array<uint32_t, 5> multiMinMaxData{ { 5, 3, 2, 5, 2 } };

        // Empty container test
        {
            AZStd::pair<const uint32_t*, const uint32_t*> minMaxPair = AZStd::minmax_element(emptyData.begin(), emptyData.end());
            EXPECT_EQ(minMaxPair.first, minMaxPair.second);
            EXPECT_EQ(emptyData.end(), minMaxPair.first);
        }

        {
            // Single element container test
            AZStd::pair<uint32_t*, uint32_t*> minMaxPair = AZStd::minmax_element(singleElementData.begin(), singleElementData.end());
            EXPECT_EQ(minMaxPair.first, minMaxPair.second);
            EXPECT_NE(singleElementData.end(), minMaxPair.second);
            EXPECT_EQ(313, *minMaxPair.first);
            EXPECT_EQ(313, *minMaxPair.second);

            // Unordered container test
            minMaxPair = AZStd::minmax_element(unorderedData.begin(), unorderedData.end());
            EXPECT_NE(unorderedData.end(), minMaxPair.first);
            EXPECT_NE(unorderedData.end(), minMaxPair.second);
            EXPECT_EQ(2, *minMaxPair.first);
            EXPECT_EQ(5, *minMaxPair.second);

            // Multiple min and max elements in same container test
            minMaxPair = AZStd::minmax_element(multiMinMaxData.begin(), multiMinMaxData.end());
            EXPECT_NE(multiMinMaxData.end(), minMaxPair.first);
            EXPECT_NE(multiMinMaxData.end(), minMaxPair.second);
            // The smallest element should correspond to the first '2' within the multiMinMaxData container
            EXPECT_EQ(multiMinMaxData.begin() + 2, minMaxPair.first);
            // The greatest element should correspond to the second '5' within the multiMinMaxData container
            EXPECT_EQ(multiMinMaxData.begin() + 3, minMaxPair.second);
            EXPECT_EQ(2, *minMaxPair.first);
            EXPECT_EQ(5, *minMaxPair.second);

            // Custom comparator test
            minMaxPair = AZStd::minmax_element(unorderedData.begin(), unorderedData.end(), AZStd::greater<uint32_t>());
            EXPECT_NE(unorderedData.end(), minMaxPair.first);
            EXPECT_NE(unorderedData.end(), minMaxPair.second);
            EXPECT_EQ(5, *minMaxPair.first);
            EXPECT_EQ(2, *minMaxPair.second);
        }
    }

    TEST_F(Algorithms, MinMax)
    {
        AZStd::initializer_list<uint32_t> singleElementData{ 908 };
        AZStd::initializer_list<uint32_t> unorderedData{ 5, 3, 2 };
        AZStd::initializer_list<uint32_t> multiMinMaxData{ 7, 10, 552, 57234, 224, 57234, 7, 238 };

        // Initializer list test
        {
            // Single element container test
            AZStd::pair<uint32_t, uint32_t> minMaxPair = AZStd::minmax(singleElementData);
            EXPECT_EQ(908, minMaxPair.first);
            EXPECT_EQ(908, minMaxPair.second);

            // Unordered container test
            minMaxPair = AZStd::minmax(unorderedData);
            EXPECT_EQ(2, minMaxPair.first);
            EXPECT_EQ(5, minMaxPair.second);

            // Multiple min and max elements in same container test
            minMaxPair = AZStd::minmax(multiMinMaxData);
            EXPECT_EQ(7, minMaxPair.first);
            EXPECT_EQ(57234, minMaxPair.second);

            // Custom comparator test
            minMaxPair = AZStd::minmax(unorderedData, AZStd::greater<uint32_t>());
            EXPECT_EQ(5, minMaxPair.first);
            EXPECT_EQ(2, minMaxPair.second);
        }

        // Two parameter test
        {
            // Sanity test
            AZStd::pair<uint32_t, uint32_t> minMaxPair = AZStd::minmax(7000, 6999);
            EXPECT_EQ(6999, minMaxPair.first);
            EXPECT_EQ(7000, minMaxPair.second);

            // Customer comparator Test
            minMaxPair = AZStd::minmax(9001, 9000, AZStd::greater<uint32_t>());
            EXPECT_EQ(9001, minMaxPair.first);
            EXPECT_EQ(9000, minMaxPair.second);
        }
    }

    TEST_F(Algorithms, IsSorted)
    {
        AZStd::array<int, 10> container1 = {{ 1, 2, 3, 4, 4, 5, 5, 5, 10, 20 }};

        bool isFullContainer1Sorted = AZStd::is_sorted(container1.begin(), container1.end());
        EXPECT_TRUE(isFullContainer1Sorted);

        bool isPartialContainer1Sorted = AZStd::is_sorted(container1.begin()+1, container1.begin()+5);
        EXPECT_TRUE(isPartialContainer1Sorted);

        bool isRangeLengthOneSorted = AZStd::is_sorted(container1.begin(), container1.begin());
        EXPECT_TRUE(isRangeLengthOneSorted);

        AZStd::array<int, 10> container2 = {{ 1, 2, 3, 4, 4, 5, 5, 5, 10, 9 }};

        bool isFullContainer2Sorted = AZStd::is_sorted(container2.begin(), container2.end());
        EXPECT_FALSE(isFullContainer2Sorted);

        // this range is container2[1] up to and including container2[8] (range doesn't include element pointed to by last iterator)
        bool isPartialContainer2Sorted = AZStd::is_sorted(container2.begin()+1, container2.begin()+9);
        EXPECT_TRUE(isPartialContainer2Sorted);

        // this range is container2[8] up to and including container2[9]
        bool isVeryEndOfContainer2Sorted = AZStd::is_sorted(container2.begin()+8, container2.end());
        EXPECT_FALSE(isVeryEndOfContainer2Sorted);
    }

    TEST_F(Algorithms, IsSorted_Comp)
    {
        auto compareLessThan = [](const int& lhs, const int& rhs) -> bool
        {
            return lhs < rhs;
        };

        auto compareGreaterThan = [](const int& lhs, const int& rhs) -> bool
        {
            return lhs > rhs;
        };

        AZStd::array<int, 10> container1 = {{ 1, 2, 3, 4, 4, 5, 5, 5, 10, 20 }};

        bool isFullContainer1SortedLt = AZStd::is_sorted(container1.begin(), container1.end(), compareLessThan);
        EXPECT_TRUE(isFullContainer1SortedLt);

        bool isFullContainer1SortedGt = AZStd::is_sorted(container1.begin(), container1.end(), compareGreaterThan);
        EXPECT_FALSE(isFullContainer1SortedGt);

        bool isPartialContainer1SortedLt = AZStd::is_sorted(container1.begin()+1, container1.begin()+5, compareLessThan);
        EXPECT_TRUE(isPartialContainer1SortedLt);

        bool isRangeLengthOneSortedLt = AZStd::is_sorted(container1.begin(), container1.begin(), compareLessThan);
        EXPECT_TRUE(isRangeLengthOneSortedLt);

        AZStd::array<int, 10> container2 = {{ 1, 2, 3, 4, 4, 5, 5, 5, 10, 9 }};

        bool isFullContainer2SortedLt = AZStd::is_sorted(container2.begin(), container2.end(), compareLessThan);
        EXPECT_FALSE(isFullContainer2SortedLt);

        // this range is container2[1] up to and including container2[8] (range doesn't include element pointed to by last iterator)
        bool isPartialContainer2SortedLt = AZStd::is_sorted(container2.begin()+1, container2.begin()+9, compareLessThan);
        EXPECT_TRUE(isPartialContainer2SortedLt);

        // this range is container2[8] up to and including container2[9]
        bool isVeryEndOfContainer2SortedLt = AZStd::is_sorted(container2.begin()+8, container2.end(), compareLessThan);
        EXPECT_FALSE(isVeryEndOfContainer2SortedLt);

        AZStd::array<int, 10> container3 = {{ 9, 10, 5, 5, 5, 4, 4, 3, 2, 1 }};

        bool isFullContainer3SortedLt = AZStd::is_sorted(container3.begin(), container3.end(), compareLessThan);
        EXPECT_FALSE(isFullContainer3SortedLt);

        bool isFullContainer3SortedGt = AZStd::is_sorted(container3.begin(), container3.end(), compareGreaterThan);
        EXPECT_FALSE(isFullContainer3SortedGt);

        // this range is container3[1] up to and including container3[8] (range doesn't include element pointed to by last iterator)
        bool isPartialContainer3SortedLt = AZStd::is_sorted(container3.begin()+1, container3.begin()+9, compareLessThan);
        EXPECT_FALSE(isPartialContainer3SortedLt);
        bool isPartialContainer3SortedGt = AZStd::is_sorted(container3.begin()+1, container3.begin()+9, compareGreaterThan);
        EXPECT_TRUE(isPartialContainer3SortedGt);

        // this range is container3[0] up to and including container3[1]
        bool isVeryStartOfContainer3SortedLt = AZStd::is_sorted(container3.begin(), container3.begin()+2, compareLessThan);
        EXPECT_TRUE(isVeryStartOfContainer3SortedLt);
        bool isVeryStartOfContainer3SortedGt = AZStd::is_sorted(container3.begin(), container3.begin()+2, compareGreaterThan);
        EXPECT_FALSE(isVeryStartOfContainer3SortedGt);
    }

    TEST_F(Algorithms, Unique)
    {
        AZStd::vector<int> container1 = {{ 1, 2, 3, 4, 4, 5, 5, 5, 10, 20 }};

        auto iterBeyondEndOfUniques = AZStd::unique(container1.begin(), container1.end());

        size_t numberOfUniques = iterBeyondEndOfUniques - container1.begin();
        EXPECT_EQ(numberOfUniques, 7);

        container1.erase(iterBeyondEndOfUniques, container1.end());
        EXPECT_EQ(container1.size(), 7);

        AZStd::vector<int> container2 = {{ 1, 2, 3, 4, 4, 5, 2, 5, 5, 5 }};

        auto iterBeyondEndOfUniques2 = AZStd::unique(container2.begin(), container2.end());

        size_t numberOfUniques2 = iterBeyondEndOfUniques2 - container2.begin();
        EXPECT_EQ(numberOfUniques2, 7);

        container2.erase(iterBeyondEndOfUniques2, container2.end());
        EXPECT_EQ(container2.size(), 7);
    }

    TEST_F(Algorithms, Unique_BinaryPredicate)
    {
        auto isEquivalent = [](const int& lhs, const int& rhs) -> bool
        {
            return lhs == rhs;
        };

        AZStd::vector<int> container1 = {{ 1, 2, 3, 4, 4, 5, 5, 5, 10, 20 }};

        auto iterBeyondEndOfUniques = AZStd::unique(container1.begin(), container1.end(), isEquivalent);

        container1.erase(iterBeyondEndOfUniques, container1.end());
        EXPECT_EQ(container1.size(), 7);

        AZStd::vector<int> container2 = {{ 1, 2, 3, 4, 4, 5, 2, 5, 5, 5 }};

        auto iterBeyondEndOfUniques2 = AZStd::unique(container2.begin(), container2.end(), isEquivalent);

        container2.erase(iterBeyondEndOfUniques2, container2.end());
        EXPECT_EQ(container2.size(), 7);
    }

    TEST_F(Algorithms, SetDifference)
    {
        AZStd::set<int> setA { 3, 5, 8, 10 };
        AZStd::set<int> setB { 1, 2, 3, 8 };
        AZStd::set<int> setC { 7, 8, 9, 10, 11, 12 };

        // Continue with tests: A - B, B - A, A - C, C - A, B - C, C - B

        // A - B = 5, 10
        AZStd::set<int> remainder;
        AZStd::set_difference(setA.begin(), setA.end(), setB.begin(), setB.end(), AZStd::inserter(remainder, remainder.begin()));
        EXPECT_TRUE(remainder == AZStd::set<int>({ 5, 10 }));

        // B - A = 1, 2
        remainder.clear();
        AZStd::set_difference(setB.begin(), setB.end(), setA.begin(), setA.end(), AZStd::inserter(remainder, remainder.begin()));
        EXPECT_TRUE(remainder == AZStd::set<int>({ 1, 2 }));

        // A - C = 3, 5
        remainder.clear();
        AZStd::set_difference(setA.begin(), setA.end(), setC.begin(), setC.end(), AZStd::inserter(remainder, remainder.begin()));
        EXPECT_TRUE(remainder == AZStd::set<int>({ 3, 5 }));

        // C - A = 7, 9, 11, 12
        remainder.clear();
        AZStd::set_difference(setC.begin(), setC.end(), setA.begin(), setA.end(), AZStd::inserter(remainder, remainder.begin()));
        EXPECT_TRUE(remainder == AZStd::set<int>({ 7, 9, 11, 12 }));

        // B - C = 1, 2, 3
        remainder.clear();
        AZStd::set_difference(setB.begin(), setB.end(), setC.begin(), setC.end(), AZStd::inserter(remainder, remainder.begin()));
        EXPECT_TRUE(remainder == AZStd::set<int>({ 1, 2, 3 }));

        // C - B = 7, 9, 10, 11, 12
        remainder.clear();
        AZStd::set_difference(setC.begin(), setC.end(), setB.begin(), setB.end(), AZStd::inserter(remainder, remainder.begin()));
        EXPECT_TRUE(remainder == AZStd::set<int>({ 7, 9, 10, 11, 12 }));
    }

    TEST_F(Algorithms, Mismatch_CallWithoutExplicitSecondLastIterator_ReturnsMismatch)
    {
        AZStd::string container1 = "@assets1@/folder";
        AZStd::string container2 = "@assets2@/folder";

        {
            auto[container1Iter, container2Iter] = AZStd::mismatch(container1.begin(), container1.end(), container2.begin());
            ASSERT_NE(container1.end(), container1Iter);
            ASSERT_NE(container2.end(), container2Iter);
            EXPECT_EQ('1', *container1Iter);
            EXPECT_EQ('2', *container2Iter);
        }

        auto CaseInsensitivePathSearch = [](const char lhs, const char rhs) -> bool
        {
            constexpr AZStd::string_view pathSeparators{ "/\\" };
            const bool lhsIsSeparator = pathSeparators.find_first_of(lhs) != AZStd::string_view::npos;
            const bool rhsIsSeparator = pathSeparators.find_first_of(lhs) != AZStd::string_view::npos;
            return (lhsIsSeparator && rhsIsSeparator) || tolower(lhs) == tolower(rhs);
        };

        AZStd::string containerCaseInsenstive1 = "@assets1@/folder";
        AZStd::string containerCaseInsenstive2 = "@assEts1@\\foldVer";
        {
            auto[container1Iter, container2Iter] = AZStd::mismatch(containerCaseInsenstive1.begin(), containerCaseInsenstive1.end(), containerCaseInsenstive2.begin(), CaseInsensitivePathSearch);
            ASSERT_NE(containerCaseInsenstive1.end(), container1Iter);
            ASSERT_NE(containerCaseInsenstive2.end(), container2Iter);
            EXPECT_EQ('e', *container1Iter);
            EXPECT_EQ('V', *container2Iter);
        }

        // constexpr test
        {
            constexpr AZStd::string_view path1 = "@assets1@";
            constexpr AZStd::string_view path2 = "@assets2@";
            {
                constexpr auto pathPair = AZStd::mismatch(AZStd::begin(path1), AZStd::end(path1), AZStd::begin(path2));
                static_assert(path1.end() != pathPair.first);
                static_assert(path2.end() != pathPair.second);
                static_assert('1' == *(pathPair.first));
                static_assert('2' == *pathPair.second);
            }
        }
    }

    TEST_F(Algorithms, Mismatch_CallWithExplicitSecondLastIterator_ReturnsMismatch)
    {
        AZStd::string container1 = "@assets1@/folder";
        AZStd::string container2 = "@assets2@/folder";

        {
            auto[container1Iter, container2Iter] = AZStd::mismatch(container1.begin(), container1.end(), container2.begin(), container2.begin() + 5);
            EXPECT_EQ(container1.begin() + 5, container1Iter);
            EXPECT_EQ(container2.begin() + 5, container2Iter);
            EXPECT_EQ('t', *container1Iter);
            EXPECT_EQ('t', *container2Iter);
        }

        auto CaseInsensitivePathSearch = [](const char lhs, const char rhs) -> bool
        {
            constexpr AZStd::string_view pathSeparators{ "/\\" };
            const bool lhsIsSeparator = pathSeparators.find_first_of(lhs) != AZStd::string_view::npos;
            const bool rhsIsSeparator = pathSeparators.find_first_of(lhs) != AZStd::string_view::npos;
            return (lhsIsSeparator && rhsIsSeparator) || tolower(lhs) == tolower(rhs);
        };

        AZStd::string containerCaseInsenstive1 = "@assets1@/folder";
        AZStd::string containerCaseInsenstive2 = "@assEts1@\\foldVer";
        {
            auto[container1Iter, container2Iter] = AZStd::mismatch(containerCaseInsenstive1.begin(), containerCaseInsenstive1.end(),
                containerCaseInsenstive2.begin(), containerCaseInsenstive2.begin() + 11,
                CaseInsensitivePathSearch);
            ASSERT_EQ(containerCaseInsenstive1.begin() + 11, container1Iter);
            ASSERT_EQ(containerCaseInsenstive2.begin() + 11, container2Iter);
            EXPECT_EQ('o', *container1Iter);
            EXPECT_EQ('o', *container2Iter);
        }
    }

    TEST_F(Algorithms, Equal)
    {
        AZStd::vector<int> container1 = { 1, 2, 3, 4, 5 };
        AZStd::vector<int> container2 = { 11, 12, 13, 14, 15 };

        EXPECT_TRUE(AZStd::equal(container1.begin(), container1.end(), container1.begin()));
        EXPECT_FALSE(AZStd::equal(container1.begin(), container1.end(), container2.begin()));
        EXPECT_FALSE(AZStd::equal(container1.begin(), container1.end(), container2.begin(), container2.end()));

        auto compare = [](int lhs, int rhs) -> bool
        {
            return lhs == rhs;
        };

        EXPECT_TRUE(AZStd::equal(container1.begin(), container1.end(), container1.begin(), compare));
        EXPECT_FALSE(AZStd::equal(container1.begin(), container1.end(), container2.begin(), compare));
        EXPECT_FALSE(AZStd::equal(container1.begin(), container1.end(), container2.begin(), container2.end(), compare));
    }

    TEST_F(Algorithms, MinMax_Compile_WhenUsedInConstexpr)
    {
        static_assert(AZStd::GetMin(7, 10) == 7);
        static_assert(AZStd::min(11, 6) == 6);
        static_assert(AZStd::GetMax(5, 12) == 12);
        static_assert(AZStd::max(13, 4) == 13);
        static_assert(AZStd::max(13, 4) == 13);
        constexpr AZStd::pair<int, int> resultPair1 = AZStd::minmax(7, 8, [](int lhs, int rhs) { return lhs < rhs; });
        static_assert(resultPair1.first == 7);
        static_assert(resultPair1.second == 8);
        constexpr AZStd::pair<int, int> resultPair2 = AZStd::minmax(14, 5);
        static_assert(resultPair2.first == 5);
        static_assert(resultPair2.second == 14);

        static constexpr AZStd::array<int, 6> testList = { { 7, 43, 42, 0, -9, 500 } };
        constexpr AZStd::pair<const int*, const int*> testPair1 = AZStd::minmax_element(testList.begin(), testList.end(), [](int lhs, int rhs) { return rhs < lhs; });
        static_assert(*testPair1.first == 500);
        static_assert(*testPair1.second == -9);
        constexpr AZStd::pair<const int*, const int*> testPair2 = AZStd::minmax_element(testList.begin(), testList.end());
        static_assert(*testPair2.first == -9);
        static_assert(*testPair2.second == 500);
        constexpr AZStd::pair<int, int> testPair3 = AZStd::minmax({ 7, 43, 42, 0, -9, 500 }, [](int lhs, int rhs) { return rhs < lhs; });
        static_assert(testPair3.first == 500);
        static_assert(testPair3.second == -9);
        constexpr AZStd::pair<int, int> testPair4 = AZStd::minmax({ 7, 43, 42, 0, -9, 500 });
        static_assert(testPair4.first == -9);
        static_assert(testPair4.second == 500);

        static_assert(AZStd::clamp(15, 0, 10) == 10);
    }

    TEST_F(Algorithms, ForEach_Compile_WhenUsedInConstexpr)
    {
        auto CreateArray = []() constexpr -> AZStd::array<int, 3>
        {
            AZStd::array<int, 3> localArray = { {1, 2, 3} };
            AZStd::for_each(localArray.begin(), localArray.end(), [](int& element) { ++element; });
            return localArray;
        };
        constexpr AZStd::array<int, 3> testArray = CreateArray();
        static_assert(testArray[0] == 2);
        static_assert(testArray[1] == 3);
        static_assert(testArray[2] == 4);
    }


    TEST_F(Algorithms, CountIf_Compile_WhenUsedInConstexpr)
    {
        constexpr AZStd::array<int, 3> testList = { { 1, 2, 3 } };

        constexpr ptrdiff_t testResult = AZStd::count_if(testList.begin(), testList.end(), [](int element) { return element < 3; });
        static_assert(testResult == 2);
    }

    TEST_F(Algorithms, FindFunctions_Compile_WhenUsedInConstexpr)
    {
        static constexpr AZStd::array<int, 3> testList = { { 1, 2, 3 } };

        constexpr const int* findIter1 = AZStd::find(testList.begin(), testList.end(), 2);
        static_assert(*findIter1 == 2);

        constexpr const int* findIter2 = AZStd::find_if(testList.begin(), testList.end(), [](int element) { return element == 3; });
        static_assert(*findIter2 == 3);

        constexpr const int* findIter3 = AZStd::find_if_not(testList.begin(), testList.end(), [](int element) { return element == 1; });
        static_assert(*findIter3 == 2);

        static constexpr AZStd::array<int, 6> testList2 = { { 7, 2, 3, 3, 4, 4 } };
        constexpr const int* findIter4 = AZStd::adjacent_find(testList2.begin(), testList2.end());
        static_assert(*findIter4 == 3);
        constexpr const int* findIter5 = AZStd::adjacent_find(testList2.begin(), testList2.end(), [](int first, int next) { return first != 3 && first == next; });
        static_assert(*findIter5 == 4);

        static constexpr AZStd::array<int, 4> testList3 = { {10, 11, 12, 13 } };
        static constexpr AZStd::array<int, 4> findList = { { 96, 17, -13, 11 } };
        constexpr const int* findIter6 = AZStd::find_first_of(testList3.begin(), testList3.end(), findList.begin(), findList.end());
        static_assert(*findIter6 == 11);

        constexpr const int* findIter7 = AZStd::find_first_of(testList3.begin(), testList3.end(), findList.begin(), findList.end(),
            [](int sourceElement, int findElement) {return sourceElement != findElement; });
        static_assert(*findIter7 == 10);
    }

    TEST_F(Algorithms, AllOf_AnyOf_NoneOf_Compile_WhenUsedInConstexpr)
    {
        static constexpr AZStd::array<int, 3> testList = { { 1, 2, 3 } };
        static_assert(!AZStd::all_of(testList.begin(), testList.end(), [](int element) { return element < 2; }));
        static_assert(AZStd::any_of(testList.begin(), testList.end(), [](int element) { return element < 2; }));
        static_assert(!AZStd::none_of(testList.begin(), testList.end(), [](int element) { return element < 2; }));
    }

    TEST_F(Algorithms, Transform_Compile_WhenUsedInConstexpr)
    {
        auto TransformInitList = []() constexpr
        {
            constexpr AZStd::array<int, 3> testList = { { 1, 2, 3 } };
            AZStd::array<int, 3> localArray = {};
            AZStd::transform(testList.begin(), testList.end(), localArray.begin(), [](int element) { return element + 3; });
            return localArray;
        };

        constexpr AZStd::array<int, 3> resultArray = TransformInitList();
        static_assert(resultArray[0] == 4);
        static_assert(resultArray[1] == 5);
        static_assert(resultArray[2] == 6);
    }

    TEST_F(Algorithms, Replace_Compile_WhenUsedInConstexpr)
    {
        auto ReplaceIf = []() constexpr
        {
            AZStd::array<int, 4> localArray = { { 6, 2, -423, 0 } };
            AZStd::replace_if(localArray.begin(), localArray.end(), [](int element) { return element < 0; }, 7);
            return localArray;
        };

        constexpr AZStd::array<int, 4> resultArray1 = ReplaceIf();
        static_assert(resultArray1[0] == 6);
        static_assert(resultArray1[1] == 2);
        static_assert(resultArray1[2] == 7);
        static_assert(resultArray1[3] == 0);

        auto ReplaceCopyIf = []() constexpr
        {
            AZStd::array<int, 4> localArray = { { 6, 2, -423, 0 } };
            AZStd::array<int, 4> outputArray = {};
            AZStd::replace_copy_if(localArray.begin(), localArray.end(), outputArray.begin(), [](int element) { return element > 0; }, 10);
            return outputArray;
        };

        constexpr AZStd::array<int, 4> resultArray2 = ReplaceCopyIf();
        static_assert(resultArray2[0] == 10);
        static_assert(resultArray2[1] == 10);
        static_assert(resultArray2[2] == -423);
        static_assert(resultArray2[3] == 0);
    }

    TEST_F(Algorithms, Remove_Compile_WhenUsedInConstexpr)
    {
        auto RemoveIf = []() constexpr
        {
            AZStd::array<int, 4> localArray = { { 6, 2, -423, 0 } };
            AZStd::remove_if(localArray.begin(), localArray.end(), [](int element) { return element < 0; });
            return localArray;
        };

        constexpr AZStd::array<int, 4> resultArray1 = RemoveIf();
        static_assert(resultArray1[0] == 6);
        static_assert(resultArray1[1] == 2);
        static_assert(resultArray1[2] == 0);

        auto RemoveCopyIf = []() constexpr
        {
            AZStd::array<int, 4> localArray = { { 6, 2, -423, 0 } };
            AZStd::array<int, 4> outputArray = { };
            AZStd::remove_copy_if(localArray.begin(), localArray.end(), outputArray.begin(), [](int element) { return element > 0; });
            return outputArray;
        };

        constexpr AZStd::array<int, 4> resultArray2 = RemoveCopyIf();
        static_assert(resultArray2[0] == -423);
    }

    TEST_F(Algorithms, Generate_Compile_WhenUsedInConstexpr)
    {
        auto GenerateTest = []() constexpr -> AZStd::array<int, 3>
        {
            AZStd::array<int, 3> localArray{};
            auto GeneratorFunc = [counter = 7]() constexpr mutable
            {
                return counter++;
            };
            AZStd::generate(localArray.begin(), localArray.end(), GeneratorFunc);
            return localArray;
        };

        constexpr AZStd::array<int, 3> resultArray1 = GenerateTest();
        static_assert(resultArray1[0] == 7);
        static_assert(resultArray1[1] == 8);
        static_assert(resultArray1[2] == 9);

        auto GenerateNTest = []() constexpr -> AZStd::array<int, 3>
        {
            AZStd::array<int, 3> localArray{};
            auto GeneratorFunc = [counter = 7]() constexpr mutable
            {
                return counter++;
            };
            AZStd::generate_n(localArray.begin(), 2, GeneratorFunc);
            return localArray;
        };

        constexpr AZStd::array<int, 3> resultArray2 = GenerateNTest();
        static_assert(resultArray2[0] == 7);
        static_assert(resultArray2[1] == 8);
        static_assert(resultArray2[2] == 0);
    }

    TEST_F(Algorithms, HeapFunctions_Compile_WhenUsedInConstexpr)
    {
        auto MakeHeap = []() constexpr -> AZStd::array<int, 5>
        {
            AZStd::array<int, 5> localArray = { {3, 2, 17, 0, 72} };
            AZStd::make_heap(localArray.begin(), localArray.end());
            return localArray;
        };

        constexpr AZStd::array<int, 5> testArray1 = MakeHeap();
        static_assert(testArray1[0] == 72);
        static_assert(testArray1[1] == 3);
        static_assert(testArray1[2] == 17);
        static_assert(testArray1[3] == 0);
        static_assert(testArray1[4] == 2);

        auto PopHeap = [](AZStd::array<int, 5> sourceArray) constexpr -> AZStd::array<int, 5>
        {
            AZStd::pop_heap(sourceArray.begin(), sourceArray.end());
            return sourceArray;
        };

        constexpr AZStd::array<int, 5> testArray2 = PopHeap(testArray1);
        static_assert(testArray2[0] == 17);
        static_assert(testArray2[1] == 3);
        static_assert(testArray2[2] == 2);
        static_assert(testArray2[3] == 0);

        auto PushHeap = [](AZStd::array<int, 5> sourceArray) constexpr -> AZStd::array<int, 5>
        {
            sourceArray[4] = 5;
            AZStd::push_heap(sourceArray.begin(), sourceArray.end());
            return sourceArray;
        };

        constexpr AZStd::array<int, 5> testArray3 = PushHeap(testArray2);
        static_assert(testArray3[0] == 17);
        static_assert(testArray3[1] == 5);
        static_assert(testArray3[2] == 2);
        static_assert(testArray3[3] == 0);
        static_assert(testArray3[4] == 3);

        auto SortHeap = [](AZStd::array<int, 5> sourceArray) constexpr->AZStd::array<int, 5>
        {
            AZStd::sort_heap(sourceArray.begin(), sourceArray.end());
            return sourceArray;
        };

        constexpr AZStd::array<int, 5> testArray4 = SortHeap(testArray3);
        static_assert(testArray4[0] == 0);
        static_assert(testArray4[1] == 2);
        static_assert(testArray4[2] == 3);
        static_assert(testArray4[3] == 5);
        static_assert(testArray4[4] == 17);
    }

    TEST_F(Algorithms, UpperBound_LowerBound__Compile_WhenUsedInConstexpr)
    {
        constexpr AZStd::array<int, 6> testList = { { 1, 2, 3, 5, 6, 7 } };
        auto LowerBound = [](int lhs, int paritionValue) constexpr -> bool
        {
            return lhs < paritionValue;
        };
        static_assert(*AZStd::lower_bound(testList.begin(), testList.end(), 4 ) == 5);
        static_assert(*AZStd::lower_bound(testList.begin(), testList.end(), 3, LowerBound) == 3);

        auto UpperBound = [](int partitionValue, int rhs) constexpr -> bool
        {
            return partitionValue <= rhs;
        };

        static_assert(*AZStd::upper_bound(testList.begin(), testList.end(), 4) == 5);
        static_assert(*AZStd::upper_bound(testList.begin(), testList.end(), 4, UpperBound) == 5);
    }

    TEST_F(Algorithms, Unique_Compile_WhenUsedInConstexpr)
    {
        auto TestUnique = []() constexpr
        {
            AZStd::array<int, 6> localArray{ { 1, 2, 2, 5, 5, 6} };
            AZStd::unique(localArray.begin(), localArray.end());
            return localArray;
        };

        constexpr AZStd::array<int, 6> resultArray = TestUnique();
        static_assert(resultArray[0] == 1);
        static_assert(resultArray[1] == 2);
        static_assert(resultArray[2] == 5);
        static_assert(resultArray[3] == 6);
    }

    TEST_F(Algorithms, LexicographicalCompare_Compile_WhenUsedInConstexpr)
    {
        constexpr const char* testString1{ "Test1" };
        constexpr const char* testString2{ "Test2" };
        static_assert(AZStd::lexicographical_compare(testString1, testString1 + 5, testString2, testString2 + 5));
        static_assert(!AZStd::lexicographical_compare(testString1, testString1 + 5, testString2, testString2 + 5, [](char lhs, char rhs) { return rhs < lhs; }));
    }
#endif // AZ_UNIT_TEST_SKIP_STD_ALGORITHMS_TESTS

}
