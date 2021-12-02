/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    //
    // SpawnableEntityContainerView
    //

    class SpawnableEntityContainerViewTest : public ::testing::Test
    {
    protected:
        AZStd::array<AZ::Entity*, 4> m_values{ reinterpret_cast<AZ::Entity*>(1), reinterpret_cast<AZ::Entity*>(2),
                                               reinterpret_cast<AZ::Entity*>(3), reinterpret_cast<AZ::Entity*>(4) };
        AzFramework::SpawnableEntityContainerView m_view{ m_values.begin(), m_values.end() };
    };

    TEST_F(SpawnableEntityContainerViewTest, begin_Get_MatchesBeginOfArray)
    {
        EXPECT_EQ(m_view.begin(), m_values.begin());
    }

    TEST_F(SpawnableEntityContainerViewTest, end_Get_MatchesEndOfArray)
    {
        EXPECT_EQ(m_view.end(), m_values.end());
    }

    TEST_F(SpawnableEntityContainerViewTest, cbegin_Get_MatchesBeginOfArray)
    {
        EXPECT_EQ(m_view.cbegin(), m_values.cbegin());
    }

    TEST_F(SpawnableEntityContainerViewTest, cend_Get_MatchesEndOfArray)
    {
        EXPECT_EQ(m_view.cend(), m_values.cend());
    }

    TEST_F(SpawnableEntityContainerViewTest, IndexOperator_Get_MatchesThirdElement)
    {
        EXPECT_EQ(m_view[2], m_values[2]);
    }

    TEST_F(SpawnableEntityContainerViewTest, Size_Get_MatchesSizeOfArray)
    {
        EXPECT_EQ(m_view.size(), m_values.size());
    }

    TEST_F(SpawnableEntityContainerViewTest, empty_GetFromFilledArray_ReturnsFalse)
    {
        EXPECT_FALSE(m_view.empty());
    }

    TEST_F(SpawnableEntityContainerViewTest, empty_GetFromEmtpyView_ReturnsTrue)
    {
        AzFramework::SpawnableEntityContainerView view{ nullptr, nullptr };
        EXPECT_TRUE(view.empty());
    }


    //
    // SpawnableConstEntityContainerView
    //

    class SpawnableConstEntityContainerViewTest : public ::testing::Test
    {
    protected:
        AZStd::array<AZ::Entity*, 4> m_values{ reinterpret_cast<AZ::Entity*>(1), reinterpret_cast<AZ::Entity*>(2),
                                               reinterpret_cast<AZ::Entity*>(3), reinterpret_cast<AZ::Entity*>(4) };
        AzFramework::SpawnableConstEntityContainerView m_view{ m_values.begin(), m_values.end() };
    };

    TEST_F(SpawnableConstEntityContainerViewTest, begin_Get_MatchesBeginOfArray)
    {
        EXPECT_EQ(m_view.begin(), m_values.begin());
    }

    TEST_F(SpawnableConstEntityContainerViewTest, end_Get_MatchesEndOfArray)
    {
        EXPECT_EQ(m_view.end(), m_values.end());
    }

    TEST_F(SpawnableConstEntityContainerViewTest, cbegin_Get_MatchesBeginOfArray)
    {
        EXPECT_EQ(m_view.cbegin(), m_values.cbegin());
    }

    TEST_F(SpawnableConstEntityContainerViewTest, cend_Get_MatchesEndOfArray)
    {
        EXPECT_EQ(m_view.cend(), m_values.cend());
    }

    TEST_F(SpawnableConstEntityContainerViewTest, IndexOperator_Get_MatchesThirdElement)
    {
        EXPECT_EQ(m_view[2], m_values[2]);
    }

    TEST_F(SpawnableConstEntityContainerViewTest, size_Get_MatchesSizeOfArray)
    {
        EXPECT_EQ(m_view.size(), m_values.size());
    }

    TEST_F(SpawnableConstEntityContainerViewTest, empty_GetFromFilledArray_ReturnsFalse)
    {
        EXPECT_FALSE(m_view.empty());
    }

    TEST_F(SpawnableConstEntityContainerViewTest, empty_GetFromEmtpyView_ReturnsTrue)
    {
        AzFramework::SpawnableConstEntityContainerView view{ nullptr, nullptr };
        EXPECT_TRUE(view.empty());
    }
} // namespace UnitTest
