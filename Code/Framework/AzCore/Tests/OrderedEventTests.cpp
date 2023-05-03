/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/OrderedEvent.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class OrderedEventTests
        : public LeakDetectionFixture
    {
    };

    TEST_F(OrderedEventTests, TestHasCallback)
    {
        AZ::OrderedEvent<int32_t> testEvent;
        AZ::OrderedEventHandler<int32_t> testHandler([]([[maybe_unused]] int32_t value) {});

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
        testHandler.Connect(testEvent);
        EXPECT_TRUE(testEvent.HasHandlerConnected());
    }

    TEST_F(OrderedEventTests, TestScopedConnect)
    {
        AZ::OrderedEvent<int32_t> testEvent;

        {
            AZ::OrderedEventHandler<int32_t> testHandler([]([[maybe_unused]] int32_t value) {});
            testHandler.Connect(testEvent);
            EXPECT_TRUE(testEvent.HasHandlerConnected());
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(OrderedEventTests, TestEvent)
    {
        int32_t invokedValue = 0;

        AZ::OrderedEvent<int32_t> testEvent;
        AZ::OrderedEventHandler<int32_t> testHandler([&invokedValue](int32_t value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(1);
        EXPECT_TRUE(invokedValue == 1);
        testEvent.Signal(-1);
        EXPECT_TRUE(invokedValue == -1);
    }

    TEST_F(OrderedEventTests, TestEventRValueParam)
    {
        int32_t invokedValue = 0;

        AZ::OrderedEvent<int32_t> testEvent;
        AZ::OrderedEventHandler<int32_t> testHandler([&invokedValue](int32_t value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TEST_F(OrderedEventTests, TestEventRefParam)
    {
        int32_t invokedValue = 0;

        AZ::OrderedEvent<int32_t&> testEvent;
        AZ::OrderedEventHandler<int32_t&> testHandler([&invokedValue](int32_t& value) { invokedValue = value++; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
        EXPECT_TRUE(value == 2);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 2);
        EXPECT_TRUE(value == 3);
    }

    TEST_F(OrderedEventTests, TestEventConstRefParam)
    {
        int32_t invokedValue = 0;

        AZ::OrderedEvent<const int32_t&> testEvent;
        AZ::OrderedEventHandler<const int32_t&> testHandler([&invokedValue](const int32_t& value) { invokedValue = value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TEST_F(OrderedEventTests, TestEventPointerParam)
    {
        int32_t invokedValue = 0;

        AZ::OrderedEvent<int32_t*> testEvent;
        AZ::OrderedEventHandler<int32_t*> testHandler([&invokedValue](int32_t* value) { invokedValue = (*value)++; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 1);
        EXPECT_TRUE(value == 2);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 2);
        EXPECT_TRUE(value == 3);
    }

    TEST_F(OrderedEventTests, TestEventConstPointerParam)
    {
        int32_t invokedValue = 0;

        AZ::OrderedEvent<const int32_t*> testEvent;
        AZ::OrderedEventHandler<const int32_t*> testHandler([&invokedValue](const int32_t* value) { invokedValue = *value; });

        testHandler.Connect(testEvent);

        int32_t value = 1;

        EXPECT_TRUE(invokedValue == 0);
        testEvent.Signal(&value);
        EXPECT_TRUE(invokedValue == 1);
    }

    TEST_F(OrderedEventTests, TestEventMultiParam)
    {
        int32_t invokedValue1 = 0;
        bool invokedValue2 = false;

        AZ::OrderedEvent<int32_t, bool> testEvent;
        AZ::OrderedEventHandler<int32_t, bool> testHandler([&invokedValue1, &invokedValue2](int32_t value1, bool value2) { invokedValue1 = value1; invokedValue2 = value2; });

        testHandler.Connect(testEvent);

        EXPECT_TRUE(invokedValue1 == 0);
        EXPECT_TRUE(invokedValue2 == false);
        testEvent.Signal(1, true);
        EXPECT_TRUE(invokedValue1 == 1);
        EXPECT_TRUE(invokedValue2 == true);
        testEvent.Signal(-1, false);
        EXPECT_TRUE(invokedValue1 == -1);
        EXPECT_TRUE(invokedValue2 == false);
    }

    TEST_F(OrderedEventTests, TestConnectDuringEvent)
    {
        AZ::OrderedEvent<int32_t> testEvent;

        {
            int32_t testHandler2Data = 0;
            AZ::OrderedEventHandler<int32_t> testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });

            AZ::OrderedEventHandler<int32_t> testHandler([&testHandler2, &testEvent]([[maybe_unused]] int32_t value) { testHandler2.Connect(testEvent); });
            testHandler.Connect(testEvent);

            testEvent.Signal(1);
            EXPECT_TRUE(testHandler2Data == 0);

            testHandler.Disconnect();
            EXPECT_TRUE(testEvent.HasHandlerConnected());

            testEvent.Signal(2);
            EXPECT_TRUE(testHandler2Data == 2);
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(OrderedEventTests, TestDisconnectDuringEvent)
    {
        AZ::OrderedEvent<int32_t> testEvent;

        {
            int32_t testHandler2Data = 0;
            AZ::OrderedEventHandler<int32_t> testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });
            AZ::OrderedEventHandler<int32_t> testHandler([&testHandler2]([[maybe_unused]] int32_t value) { testHandler2.Disconnect(); });

            testHandler2.Connect(testEvent);
            testHandler.Connect(testEvent);

            testEvent.Signal(1);
            EXPECT_TRUE(testHandler2Data == 1);
            EXPECT_TRUE(testEvent.HasHandlerConnected());

            testEvent.Signal(2);
            EXPECT_TRUE(testHandler2Data == 1);
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(OrderedEventTests, TestDisconnectDuringEventReversed)
    {
        AZ::OrderedEvent<int32_t> testEvent;

        // Same test as above, but connected using reversed ordering
        {
            int32_t testHandler2Data = 0;
            AZ::OrderedEventHandler<int32_t> testHandler2([&testHandler2Data](int32_t value) { testHandler2Data = value; });
            AZ::OrderedEventHandler<int32_t> testHandler([&testHandler2]([[maybe_unused]] int32_t value) { testHandler2.Disconnect(); });

            testHandler.Connect(testEvent);
            testHandler2.Connect(testEvent);

            testEvent.Signal(1);
            EXPECT_TRUE(testHandler2Data == 0);
            EXPECT_TRUE(testEvent.HasHandlerConnected());

            testEvent.Signal(2);
            EXPECT_TRUE(testHandler2Data == 0);
        }

        EXPECT_TRUE(!testEvent.HasHandlerConnected());
    }

    TEST_F(OrderedEventTests, CopyConstructorAndCopyAssignmentOperator_AreNotCallable)
    {
        static_assert(!AZStd::is_copy_constructible_v<AZ::OrderedEvent<int32_t>>, "AZ Events should not be copy constructible");
        static_assert(!AZStd::is_copy_assignable_v<AZ::OrderedEvent<int32_t>>, "AZ Events should not be copy assignable");
    }

    TEST_F(OrderedEventTests, HandlerMoveAssignment_ProperlyDisconnectsFromOldEvent)
    {
        AZ::OrderedEvent<> testEvent1;
        AZ::OrderedEvent<> testEvent2;
         
        AZ::OrderedEventHandler<> testHandler1([]() {});
        AZ::OrderedEventHandler<> testHandler2([]() {});

        testHandler1.Connect(testEvent1);
        testHandler2.Connect(testEvent2);

        EXPECT_TRUE(testEvent1.HasHandlerConnected());
        EXPECT_TRUE(testEvent2.HasHandlerConnected());

        testHandler1 = AZStd::move(testHandler2);
        EXPECT_FALSE(testEvent1.HasHandlerConnected());
        EXPECT_TRUE(testEvent2.HasHandlerConnected());
    }

    TEST_F(OrderedEventTests, HandlersInvoked_InCorrectOrder)
    {
        AZ::OrderedEvent<> testEvent;

        int invokedCount = 0;
        AZ::OrderedEventHandler<> testHandler1(
            [&invokedCount]()
            {
                invokedCount++;
                EXPECT_EQ(invokedCount, 4);
            }, 100);
        AZ::OrderedEventHandler<> testHandler2([&invokedCount]()
            {
                invokedCount++;
                EXPECT_EQ(invokedCount, 3);
            }, 200);
        AZ::OrderedEventHandler<> testHandler3([&invokedCount]()
            {
                invokedCount++;
                EXPECT_EQ(invokedCount, 2);
            }, 300);
        AZ::OrderedEventHandler<> testHandler4([&invokedCount]()
            {
                invokedCount++;
                EXPECT_EQ(invokedCount, 1);
            }, 400);

        testHandler1.Connect(testEvent);
        testHandler2.Connect(testEvent);
        testHandler3.Connect(testEvent);
        testHandler4.Connect(testEvent);

        testEvent.Signal();
    }
}
