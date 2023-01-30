/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/sort.h>
#include <AzCore/UnitTest/TestTypes.h>

using namespace AZ;

// TickBus handler with customizable tick-order.
// When ticked it pushes its tick-order into a list.
struct OrderedTicker : public TickBus::Handler
{
    int m_order = TICK_DEFAULT; ///< Relative order on TickBus
    AZStd::vector<int>* m_targetList = nullptr; ///< OnTick, push order into this list

    ///////////////////////////////////////////////////////////////////////////
    // TickBus
    int GetTickOrder() override { return m_order; }

    void OnTick(float /*deltaTime*/, ScriptTimePoint /*time*/) override
    {
        if (m_targetList)
        {
            m_targetList->push_back(m_order);
        }
    }
    ///////////////////////////////////////////////////////////////////////////
};


class OrderedTickBus : public UnitTest::LeakDetectionFixture
{};

TEST_F(OrderedTickBus, OnTick_HandlersFireInSortedOrder)
{
    // arbitrary unsorted order for each handler
    AZStd::vector<int> unsortedHandlerOrder = { 7, 5, 6, 3, 2, 9, 1, 0, 4, 8 };

    // handlers will add their order to this list in OnTick()
    AZStd::vector<int> actualTickOrder;

    // create OrderedTickers
    AZStd::list<OrderedTicker> tickers;
    for (int order : unsortedHandlerOrder)
    {
        OrderedTicker& ticker = tickers.emplace_back();
        ticker.m_order = order;
        ticker.m_targetList = &actualTickOrder;
        ticker.TickBus::Handler::BusConnect();
    }

    // Tick!
    TickBus::Broadcast(&TickBus::Events::OnTick, 0.f, ScriptTimePoint{});

    // this is the order they should have fired in
    AZStd::vector<int> sortedOrder = unsortedHandlerOrder;
    AZStd::sort(sortedOrder.begin(), sortedOrder.end());

    // check the order they actually fired in
    EXPECT_EQ(actualTickOrder, sortedOrder);
}
