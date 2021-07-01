/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    using namespace AZStd;

    class ArrayView : public AllocatorsTestFixture
    {
    protected:
        template<typename T>
        void ExpectEqual(initializer_list<T> expectedValues, array_view<T> arrayView)
        {
            EXPECT_EQ(false, arrayView.empty());
            EXPECT_EQ(expectedValues.size(), arrayView.size());

            typename AZStd::vector<T>::const_iterator iterator = arrayView.begin();

            for (int i = 0; i < expectedValues.size(); ++i, ++iterator)
            {
                EXPECT_EQ(expectedValues.begin()[i], arrayView[i]);
                EXPECT_EQ(expectedValues.begin()[i], *iterator);
            }

            EXPECT_EQ(iterator, arrayView.end());
        }
    };

    TEST_F(ArrayView, DefaultConstructor)
    {
        array_view<bool>   defaultView;

        EXPECT_EQ(nullptr, defaultView.begin());
        EXPECT_EQ(nullptr, defaultView.end());
        EXPECT_EQ(0,       defaultView.size());
        EXPECT_EQ(true,    defaultView.empty());
    }

    TEST_F(ArrayView, PointerConstructor1)
    {
        int originalValues[4] = { 2,3,4,5 };
        array_view<int> view(originalValues, AZ_ARRAY_SIZE(originalValues));

        ExpectEqual({ 2,3,4,5 }, view);

        EXPECT_EQ(originalValues, view.begin());
        EXPECT_EQ(&originalValues[4], view.end());
    }

    TEST_F(ArrayView, PointerConstructor2)
    {
        int originalValues[3] = { 6,7,8 };
        array_view<int> view(originalValues, &originalValues[3]);

        ExpectEqual({ 6,7,8 }, view);

        EXPECT_EQ(originalValues, view.begin());
        EXPECT_EQ(&originalValues[3], view.end());
    }

    TEST_F(ArrayView, ArrayConstructor)
    {
        array<int, 4> originalValues = { 9,10,11,12 };
        array_view<int> view(originalValues);

        ExpectEqual({ 9,10,11,12 }, view);

        EXPECT_EQ(originalValues.begin(), view.begin());
        EXPECT_EQ(originalValues.end(), view.end());
    }


    TEST_F(ArrayView, VectorConstructor)
    {
        vector<int> originalValues = { 13,14,15,16,17,18 };
        array_view<int> view(originalValues);

        ExpectEqual({ 13,14,15,16,17,18 }, view);

        EXPECT_EQ(originalValues.begin(), view.begin());
        EXPECT_EQ(originalValues.end(), view.end());
    }

    TEST_F(ArrayView, FixedVectorConstructor)
    {
        fixed_vector<int, 10> originalValues = { 17,18,19 }; // Note that even though the fixed_vector capacity is 10, it's size is 3, so the view size will be 3 as well
        array_view<int> view(originalValues);

        ExpectEqual({ 17,18,19 }, view);

        EXPECT_EQ(originalValues.begin(), view.begin());
        EXPECT_EQ(originalValues.end(), view.end());
    }

    TEST_F(ArrayView, CopyConstructor)
    {
        fixed_vector<int, 2> originalValues = { 27,28 };

        array_view<int> view1(originalValues);
        array_view<int> view2(view1);

        ExpectEqual({ 27,28 }, view2);

        EXPECT_EQ(view1.begin(), view2.begin());
        EXPECT_EQ(view1.end(), view2.end());
    }

    TEST_F(ArrayView, MoveConstructor)
    {
        int originalValues[] = { 29,30,31 };
        array_view<int> view1(originalValues, AZ_ARRAY_SIZE(originalValues));
        array_view<int> view2(AZStd::move(view1));

        ExpectEqual({ 29,30,31 }, view2);

        EXPECT_EQ(originalValues, view2.begin());
        EXPECT_EQ(&originalValues[3], view2.end());

        // This isn't strictly necessary but is a good way to make sure the move 
        // constructor actually exists and it itn't just calling the copy constructor
#if AZ_DEBUG_BUILD // The pointers are only cleared in debug
        EXPECT_EQ(nullptr, view1.begin());
        EXPECT_EQ(nullptr, view1.end());
#endif
    }

    TEST_F(ArrayView, AssignmentOperator)
    {
        fixed_vector<int, 4> originalValues = { 32,33,34,35 };

        array_view<int> view1(originalValues);
        array_view<int> view2;

        view2 = view1;

        ExpectEqual({ 32,33,34,35 }, view2);

        EXPECT_EQ(view1.begin(), view2.begin());
        EXPECT_EQ(view1.end(), view2.end());
    }

    TEST_F(ArrayView, MoveAssignmentOperator)
    {
        int originalValues[] = { 36,37,38,39,40 };
        array_view<int> view1(originalValues, AZ_ARRAY_SIZE(originalValues));
        array_view<int> view2;
        view2 = AZStd::move(view1);

        ExpectEqual({ 36,37,38,39,40 }, view2);

        EXPECT_EQ(originalValues, view2.begin());
        EXPECT_EQ(&originalValues[5], view2.end());

        // This isn't strictly necessary but is a good way to make sure the move 
        // assignment operator actually exists and it itn't just calling the norm
        // assignment operator
#if AZ_DEBUG_BUILD // The pointers are only cleared in debug
        EXPECT_EQ(nullptr, view1.begin());
        EXPECT_EQ(nullptr, view1.end());
#endif
    }

    TEST_F(ArrayView, Erase)
    {
        fixed_vector<int, 4> originalValues = { 1,2,3,4 };

        array_view<int> view(originalValues);
        view.erase();

        EXPECT_EQ(nullptr, view.begin());
        EXPECT_EQ(nullptr, view.end());
        EXPECT_EQ(0, view.size());
        EXPECT_EQ(true, view.empty());
    }

    TEST_F(ArrayView, BeginAndEnd)
    {
        fixed_vector<int, 4> originalValues = { 1,2,3,4 };

        array_view<int> view(originalValues);

        EXPECT_EQ(1, view.begin()[0]);
        EXPECT_EQ(4, view.end()[-1]);
        EXPECT_EQ(1, view.cbegin()[0]);
        EXPECT_EQ(4, view.cend()[-1]);
        EXPECT_EQ(4, view.rbegin()[0]);
        EXPECT_EQ(1, view.rend()[-1]);
        EXPECT_EQ(4, view.crbegin()[0]);
        EXPECT_EQ(1, view.crend()[-1]);
    }

    TEST_F(ArrayView, ImplicitConstruction)
    {
        // This test verifies that we can pass in various non-array_view types
        // into functions that take an array_view

        // The compile cannot detect the correct template type so that has to be specified explicitly

        ExpectEqual<int>({ 1,2,3 }, vector<int>({ 1,2,3 }));
        ExpectEqual<int>({ 1,2,3 }, fixed_vector<int, 3>({ 1,2,3 }));
        ExpectEqual<int>({ 1,2,3 }, array<int, 3>({ 1,2,3 }));
    }

    void CheckComparisonOperators(bool areEqual, array_view<int> a, array_view<int> b)
    {
        EXPECT_EQ(areEqual, a == b);

        // For less/greater operators, the exact order doesn't really matter;
        // We just check for internal consistency
        if (areEqual)
        {
            EXPECT_EQ(false, a != b);
            EXPECT_EQ(false, a < b);
            EXPECT_EQ(false, a > b);
            EXPECT_EQ(true,  a <= b);
            EXPECT_EQ(true,  a >= b);
        }
        else
        {
            EXPECT_EQ(true, a != b);

            EXPECT_EQ(a > b, a >= b);
            EXPECT_EQ(a < b, a <= b);

            EXPECT_NE(a > b, a < b);
            EXPECT_NE(a >= b, a <= b);
            EXPECT_NE(a >= b, a < b);
            EXPECT_NE(a > b, a <= b);
            EXPECT_NE(a <= b, a > b);
            EXPECT_NE(a < b, a >= b);
        }
    }

    TEST_F(ArrayView, ComparisonOperators)
    {
        int arrayA[] = { 1,2,3 };
        int arrayB[] = { 1,2,3 };

        array_view<int> arrayA_view(arrayA, 3);
        array_view<int> arrayB_view(arrayB, 3);
        array_view<int> arrayA_otherView(arrayA, 3);
        // view of a sub-array aligned to the beginning of the array
        array_view<int> arrayA_headView(arrayA, 2);
        array_view<int> arrayB_headView(arrayB, 2);
        // view of a sub-array aligned to the end of the array
        array_view<int> arrayA_tailView(&arrayA[1], 2);
        array_view<int> arrayB_tailView(&arrayB[1], 2);
        // view of a sub-array in the middle of the array
        array_view<int> arrayA_centerView(&arrayA[1], 1);
        array_view<int> arrayB_centerView(&arrayB[1], 1);

        // Same view
        CheckComparisonOperators(true, arrayA_view, arrayA_view);

        // Different view, same array 
        CheckComparisonOperators(true, arrayA_view, arrayA_otherView);
        CheckComparisonOperators(true, arrayA_otherView, arrayA_view);

        // Different arrays
        CheckComparisonOperators(false, arrayA_view, arrayB_view);
        CheckComparisonOperators(false, arrayB_view, arrayA_view);

        // Same arrays, but one is a just a subset of the array
        CheckComparisonOperators(false, arrayA_view, arrayA_headView);
        CheckComparisonOperators(false, arrayA_view, arrayA_tailView);
        CheckComparisonOperators(false, arrayA_view, arrayA_centerView);
        CheckComparisonOperators(false, arrayA_headView, arrayA_view);
        CheckComparisonOperators(false, arrayA_tailView, arrayA_view);
        CheckComparisonOperators(false, arrayA_centerView, arrayA_view);

        // Different arrays, different lengths
        CheckComparisonOperators(false, arrayA_view, arrayB_headView);
        CheckComparisonOperators(false, arrayB_view, arrayA_headView);
        CheckComparisonOperators(false, arrayB_headView, arrayA_view);
        CheckComparisonOperators(false, arrayA_headView, arrayB_view);
    }

    TEST_F(ArrayView, AssertOutOfBounds)
    {
        array_view<int> view({ 1,2,3,4 });

        AZ_TEST_START_TRACE_SUPPRESSION;

        view[4];
        view[5];

        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

}
