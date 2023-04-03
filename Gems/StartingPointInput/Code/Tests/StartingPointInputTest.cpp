/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <InputEventMap.h>
#include <InputEventGroup.h>
#include <InputEventBindings.h>

class StartingPointInputTest
    : public ::testing::Test
{
};

TEST_F(StartingPointInputTest, CopyingInputEventGroupDoesDeepCopy)
{
    struct TestInputEventBindings : public StartingPointInput::InputEventBindings
    {
        void AddInputEventGroup(StartingPointInput::InputEventGroup eventGroup)
        {
            m_inputEventGroups.push_back(eventGroup);
        }

        AZStd::vector<StartingPointInput::InputEventGroup> GetInputEventGroups() const
        {
            return m_inputEventGroups;
        }
    };

    struct TestInputEventMap : public StartingPointInput::InputEventMap
    {
        void Activate([[maybe_unused]] const StartingPointInput::InputEventNotificationId& channel) {}
        void Deactivate([[maybe_unused]] const StartingPointInput::InputEventNotificationId& channel) {}
        int m_testData = -1;
    };

    struct TestInputEventGroup : public StartingPointInput::InputEventGroup
    {
        void AddInputEventMap(StartingPointInput::InputEventMap* inputEventMap)
        {
            m_inputHandlers.push_back(inputEventMap);
        }

        bool IsSame(const TestInputEventGroup& other)
        {
            if (m_inputHandlers.size() != other.m_inputHandlers.size()) { return false; }
            for (size_t i = 0; i < m_inputHandlers.size(); ++i)
            {
                if (m_inputHandlers[i] != other.m_inputHandlers[i]) { return false; }
            }
            return true;
        }

        bool IsSameData(const TestInputEventGroup& other)
        {
            if (m_inputHandlers.size() != other.m_inputHandlers.size()) { return false; }
            for (size_t i = 0; i < m_inputHandlers.size(); ++i)
            {
                TestInputEventMap* lhs = static_cast<TestInputEventMap*>(m_inputHandlers[i]);
                TestInputEventMap* rhs = static_cast<TestInputEventMap*>(other.m_inputHandlers[i]);
                if (lhs->m_testData != rhs->m_testData) { return false; }
            }
            return true;
        }
    };


    TestInputEventBindings testBindings1;
    TestInputEventBindings testBindings2;

    TestInputEventGroup testEventGroup1;
    TestInputEventGroup testEventGroup2;
    TestInputEventMap testInput1;
    TestInputEventMap testInput2;

    // Set up the test case with some pieces of data for each input.
    testInput1.m_testData = 5;
    testInput2.m_testData = 37;

    // Assign those separate input sub components to the groups.
    testEventGroup1.AddInputEventMap(&testInput1);
    testEventGroup2.AddInputEventMap(&testInput2);

    // Set up the bindings that will be swapped.
    testBindings1.AddInputEventGroup(testEventGroup1);
    testBindings2.AddInputEventGroup(testEventGroup2);
    
    // Perform the swap, which is the primary thing being tested here.
    testBindings1.Swap(&testBindings2);

    // After the swap, make sure they aren't the same.
    // This just checks pointers, and not the actual data.
    EXPECT_FALSE(testEventGroup1.IsSame(testEventGroup2));

    // Verify the data is different
    EXPECT_FALSE(testEventGroup1.IsSameData(testEventGroup2));

    // Make sure the swapped data matches the control data.
    AZStd::vector<StartingPointInput::InputEventGroup> swappedGroup1 = testBindings1.GetInputEventGroups();
    AZStd::vector<StartingPointInput::InputEventGroup> swappedGroup2 = testBindings2.GetInputEventGroups();
    ASSERT_TRUE(swappedGroup1.size() == 1);
    ASSERT_TRUE(swappedGroup2.size() == 1);
    EXPECT_TRUE(testEventGroup1.IsSameData(*(static_cast<TestInputEventGroup*>(&swappedGroup2[0]))));
    EXPECT_TRUE(testEventGroup2.IsSameData(*(static_cast<TestInputEventGroup*>(&swappedGroup1[0]))));
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
