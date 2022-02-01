/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/ranges/all_view.h>
#include <AzCore/std/ranges/ranges_adaptor.h>
#include <AzCore/std/ranges/zip_view.h>
#include <AzCore/std/string/string_view.h>

namespace UnitTest
{
    class RangesViewTestFixture
        : public ScopedAllocatorSetupFixture
    {};

    TEST_F(RangesViewTestFixture, AllRangeAdaptor_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        auto testAllView = AZStd::ranges::views::all(testString);
        EXPECT_EQ(testString.size(), testAllView.size());
        EXPECT_EQ(testString.data(), testAllView.data());
        EXPECT_EQ(testString.begin(), testAllView.begin());
        EXPECT_EQ(testString.end(), testAllView.end());
        EXPECT_EQ(testString.empty(), testAllView.empty());
        EXPECT_EQ(testString.front(), testAllView.front());
        EXPECT_EQ(testString.back(), testAllView.back());
        EXPECT_EQ(testString[5], testAllView[5]);

        auto testAllViewChain = testString | AZStd::ranges::views::all;
        EXPECT_EQ(testString.size(), testAllViewChain.size());
        EXPECT_EQ(testString.data(), testAllViewChain.data());
        EXPECT_EQ(testString.begin(), testAllViewChain.begin());
        EXPECT_EQ(testString.end(), testAllViewChain.end());
        EXPECT_EQ(testString.empty(), testAllViewChain.empty());
        EXPECT_EQ(testString.front(), testAllViewChain.front());
        EXPECT_EQ(testString.back(), testAllViewChain.back());
        EXPECT_EQ(testString[5], testAllViewChain[5]);
    }

    TEST_F(RangesViewTestFixture, RefView_CanWrapStringView_Succeeds)
    {
        AZStd::string_view testString{ "Hello World" };
        AZStd::ranges::ref_view refView(testString);
        EXPECT_EQ(testString.size(), refView.size());
        EXPECT_EQ(testString.data(), refView.data());
        EXPECT_EQ(testString.begin(), refView.begin());
        EXPECT_EQ(testString.end(), refView.end());
        EXPECT_EQ(testString.empty(), refView.empty());
        EXPECT_EQ(testString.front(), refView.front());
        EXPECT_EQ(testString.back(), refView.back());
        EXPECT_EQ(testString[5], refView[5]);
    }

    TEST_F(RangesViewTestFixture, OwningView_CanWrapStringView_Succeeds)
    {
        AZStd::string_view sourceView{ "Hello World" };
        AZStd::string_view testString{ sourceView };
        AZStd::ranges::owning_view owningView(AZStd::move(testString));
        EXPECT_TRUE(testString.empty());
        EXPECT_FALSE(owningView.empty());
        EXPECT_EQ(sourceView.size(), owningView.size());
        EXPECT_EQ(sourceView.data(), owningView.data());
        EXPECT_EQ(sourceView.begin(), owningView.begin());
        EXPECT_EQ(sourceView.end(), owningView.end());
        EXPECT_EQ(sourceView.empty(), owningView.empty());
        EXPECT_EQ(sourceView.front(), owningView.front());
        EXPECT_EQ(sourceView.back(), owningView.back());
        EXPECT_EQ(sourceView[5], owningView[5]);
    }

    TEST_F(RangesViewTestFixture, ZipView_CompilesWithRange_Succeeds)
    {
        AZStd::string_view sourceView{ "Hello World" };
        AZStd::ranges::zip_view zipView(AZStd::move(sourceView));
        auto zipItTuple = zipView.begin();
        EXPECT_EQ('H', AZStd::get<0>(*zipItTuple));

        AZStd::list<int> sourceList{1, 2, 3, 4, 5};
        AZStd::ranges::zip_view zipView2(AZStd::move(sourceList));
    }
}
