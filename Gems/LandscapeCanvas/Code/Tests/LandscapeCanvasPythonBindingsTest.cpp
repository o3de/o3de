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
#include <AzCore/UnitTest/TestTypes.h>

#include <Source/LandscapeCanvasSystemComponent.h>

class LandscapeCanvasPythonBindingsFixture
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;

        m_application.Create(appDesc);

        m_application.RegisterComponentDescriptor(LandscapeCanvas::LandscapeCanvasSystemComponent::CreateDescriptor());
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
};

TEST_F(LandscapeCanvasPythonBindingsFixture, LandscapeCanvasNodeFactoryRequests_ApiExists)
{
    AZ::BehaviorContext* behaviorContext(nullptr);
    AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
    ASSERT_TRUE(behaviorContext);

    auto nodeFactoryRequestBus = behaviorContext->m_ebuses.find("LandscapeCanvasNodeFactoryRequestBus");
    EXPECT_TRUE(behaviorContext->m_ebuses.end() != nodeFactoryRequestBus);

    auto landscapeCanvasRequestBus = behaviorContext->m_ebuses.find("LandscapeCanvasRequestBus");
    EXPECT_TRUE(behaviorContext->m_ebuses.end() != landscapeCanvasRequestBus);
}
