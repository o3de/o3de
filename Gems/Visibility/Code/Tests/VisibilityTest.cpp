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
#include "Visibility_precompiled.h"

#include <AzTest/AzTest.h>
#include <Mocks/ITimerMock.h>
#include <Mocks/ICryPakMock.h>
#include <Mocks/IConsoleMock.h>
#include <Mocks/ISystemMock.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/std/chrono/clocks.h>

#include <AzFramework/Components/TransformComponent.h>

#include <VisibilityGem.h>
#include <OccluderAreaComponent.h>
#include <PortalComponent.h>
#include <VisAreaComponent.h>

class VisibilityTest
    : public ::testing::Test
{
protected:

    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;

        AZ::ComponentApplication::StartupParameters appStartup;
        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new VisibilityGem);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
        m_systemEntity->Init();
        m_systemEntity->Activate();

        m_application.RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());
        m_application.RegisterComponentDescriptor(Visibility::OccluderAreaComponent::CreateDescriptor());
        m_application.RegisterComponentDescriptor(Visibility::PortalComponent::CreateDescriptor());
        m_application.RegisterComponentDescriptor(Visibility::VisAreaComponent::CreateDescriptor());
    }

    void TearDown() override
    {
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
    AZ::Entity* m_systemEntity;
};

#include <EditorOccluderAreaComponent.h>
#include <EditorPortalComponent.h>
#include <EditorVisAreaComponent.h>

TEST_F(VisibilityTest, Occluder_TestIntersect)
{
    AZ::Entity* testEntity = aznew AZ::Entity();
    ASSERT_TRUE(testEntity != nullptr);

    testEntity->CreateComponent<AzFramework::TransformComponent>();
    testEntity->CreateComponent<Visibility::EditorOccluderAreaComponent>();

    Visibility::EditorOccluderAreaComponent* oaComp = testEntity->FindComponent<Visibility::EditorOccluderAreaComponent>();
    ASSERT_TRUE(oaComp != nullptr);
    testEntity->Init();
    testEntity->Activate();

    // Test CCW tri intersection test
    const AZ::Vector3 src(0, 0, 10);
    const AZ::Vector3 dir(0, 0, -1);
    float distance;

    // Visibility Components do not make use of the ViewportInfo to determine
    // camera position etc.
    AzFramework::ViewportInfo viewportInfo{};

    bool didHit = oaComp->EditorSelectionIntersectRayViewport(viewportInfo, src, dir, distance);
    ASSERT_TRUE(didHit);
    ASSERT_NEAR(distance, 10, 0.1f); // Occluder is a flat plane

    // Test CW tri intersection test
    const AZ::Vector3 srcNeg(0, 0, -10);
    const AZ::Vector3 dirNeg(0, 0, 1);

    didHit = oaComp->EditorSelectionIntersectRayViewport(viewportInfo, srcNeg, dirNeg, distance);
    ASSERT_TRUE(didHit);
    ASSERT_NEAR(distance, 10, 0.1f); // Occluder is a flat plane

    // Test intersect failure
    const AZ::Vector3 badDir(100, 100, -1);
    didHit = oaComp->EditorSelectionIntersectRayViewport(viewportInfo, src, badDir, distance);
    ASSERT_FALSE(didHit);
}

TEST_F(VisibilityTest, Portal_TestIntersect)
{
    AZ::Entity* testEntity = aznew AZ::Entity();
    ASSERT_TRUE(testEntity != nullptr);

    testEntity->CreateComponent<AzFramework::TransformComponent>();
    testEntity->CreateComponent<Visibility::EditorPortalComponent>();

    Visibility::EditorPortalComponent* pComp = testEntity->FindComponent<Visibility::EditorPortalComponent>();
    ASSERT_TRUE(pComp != nullptr);
    testEntity->Init();
    testEntity->Activate();

    const AZ::Vector3 src(0, 0, 10);
    const AZ::Vector3 dir(0, 0, -1);
    float distance;

    // Visibility Components do not make use of the ViewportInfo to determine
    // camera position etc.
    AzFramework::ViewportInfo viewportInfo{};

    // Test intersect
    bool didHit = pComp->EditorSelectionIntersectRayViewport(viewportInfo, src, dir, distance);
    ASSERT_TRUE(didHit);
    ASSERT_NEAR(distance, 9, 0.1f); // Portal has a default height of 1

    // Test casting a ray from inside the geom
    const AZ::Vector3 internalSrc(0, 0, 0.5f);
    didHit = pComp->EditorSelectionIntersectRayViewport(viewportInfo, internalSrc, dir, distance);
    ASSERT_FALSE(didHit);

    // Test intersect failure
    const AZ::Vector3 badDir(100, 100, -1);
    didHit = pComp->EditorSelectionIntersectRayViewport(viewportInfo, src, badDir, distance);
    ASSERT_FALSE(didHit);
}

TEST_F(VisibilityTest, VisArea_TestIntersect)
{
    AZ::Entity* testEntity = aznew AZ::Entity();
    ASSERT_TRUE(testEntity != nullptr);

    testEntity->CreateComponent<AzFramework::TransformComponent>();
    testEntity->CreateComponent<Visibility::EditorVisAreaComponent>();
    testEntity->Init();
    testEntity->Activate();

    Visibility::EditorVisAreaComponent* vaComp = testEntity->FindComponent<Visibility::EditorVisAreaComponent>();
    ASSERT_TRUE(vaComp != nullptr);

    const AZ::Vector3 src(0, 0, 10);
    const AZ::Vector3 dir(0, 0, -1);
    float distance;

    // Visibility Components do not make use of the ViewportInfo to determine
    // camera position etc.
    AzFramework::ViewportInfo viewportInfo{};

    bool didHit = vaComp->EditorSelectionIntersectRayViewport(viewportInfo, src, dir, distance);
    ASSERT_TRUE(didHit);
    ASSERT_NEAR(distance, 5, 0.1f); // VisArea has a default height of 5

    const AZ::Vector3 badDir(100, 100, -1);
    didHit = vaComp->EditorSelectionIntersectRayViewport(viewportInfo, src, badDir, distance);
    ASSERT_FALSE(didHit);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
