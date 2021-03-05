/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/MemoryComponent.h>
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
