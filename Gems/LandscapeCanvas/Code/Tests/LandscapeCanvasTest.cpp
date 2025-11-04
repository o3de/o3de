/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <Source/LandscapeCanvasSystemComponent.h>

class LandscapeCanvasTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup a system allocator

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

TEST_F(LandscapeCanvasTest, LandscapeCanvasSystemComponentCreatesAndDestroysSuccessfully)
{
    LandscapeCanvas::LandscapeCanvasSystemComponent test;
}

TEST_F(LandscapeCanvasTest, LandscapeCanvasSystemComponentActivatesAndDeactivatesSuccessfully)
{
    auto entity = AZStd::make_unique<AZ::Entity>();
    entity->CreateComponent<LandscapeCanvas::LandscapeCanvasSystemComponent>();

    entity->Init();
    EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

    entity->Activate();
    EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());

    entity->Deactivate();
    EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
