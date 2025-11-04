/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class GraphModelPythonBindingsFixture
        : public ::testing::Test
    {
    };

    TEST_F(GraphModelPythonBindingsFixture, GraphModelGraphManagerRequests_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        ASSERT_TRUE(behaviorContext);

        auto graphManagerRequestBus = behaviorContext->m_ebuses.find("GraphManagerRequestBus");
        EXPECT_TRUE(behaviorContext->m_ebuses.end() != graphManagerRequestBus);

        auto* behaviorBus = graphManagerRequestBus->second;
        auto eventIt = behaviorBus->m_events.find("GetGraph");
        EXPECT_TRUE(behaviorBus->m_events.end() != eventIt);
    }

    TEST_F(GraphModelPythonBindingsFixture, GraphModelGraphControllerRequests_ApiExists)
    {
        AZ::BehaviorContext* behaviorContext(nullptr);
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        ASSERT_TRUE(behaviorContext);

        auto graphControllerRequestBus = behaviorContext->m_ebuses.find("GraphControllerRequestBus");
        EXPECT_TRUE(behaviorContext->m_ebuses.end() != graphControllerRequestBus);

        const AZStd::vector<AZStd::string> eventNames = {
            "AddNode",
            "RemoveNode",
            "AddConnection",
            "AddConnectionBySlotId",
            "RemoveConnection"
        };
        auto* behaviorBus = graphControllerRequestBus->second;
        for (const AZStd::string& name : eventNames)
        {
            auto eventIt = behaviorBus->m_events.find(name);
            EXPECT_TRUE(behaviorBus->m_events.end() != eventIt);
        }
    }
}
