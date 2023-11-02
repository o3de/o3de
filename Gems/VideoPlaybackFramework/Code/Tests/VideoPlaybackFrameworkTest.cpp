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

#include <VideoPlaybackFrameworkModule.h>
#include <VideoPlaybackFrameworkSystemComponent.h>

TEST(VideoPlaybackFrameworkTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
    // appDesc.m_stackRecordLevels = 20; // Gruber patch // VMED

    AZ::ComponentApplication app;
    AZ::Entity* systemEntity = app.Create(appDesc);
    ASSERT_TRUE(systemEntity != nullptr);
    app.RegisterComponentDescriptor(VideoPlaybackFramework::VideoPlaybackFrameworkSystemComponent::CreateDescriptor());

    systemEntity->CreateComponent<VideoPlaybackFramework::VideoPlaybackFrameworkSystemComponent>();

    systemEntity->Init();
    systemEntity->Activate();

    app.Destroy();
    ASSERT_TRUE(true);
}

class VideoPlaybackFrameworkTestApp
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;
        // appDesc.m_stackRecordLevels = 20; // Gruber patch // VMED

        AZ::ComponentApplication::StartupParameters appStartup;
        appStartup.m_createStaticModulesCallback =
            [](AZStd::vector<AZ::Module*>& modules)
        {
            modules.emplace_back(new VideoPlaybackFramework::VideoPlaybackFrameworkModule);
        };

        m_systemEntity = m_application.Create(appDesc, appStartup);
        m_systemEntity->Init();
        m_systemEntity->Activate();

        m_application.RegisterComponentDescriptor(VideoPlaybackFramework::VideoPlaybackFrameworkSystemComponent::CreateDescriptor());
    }

    void TearDown() override
    {
        delete m_systemEntity;
        m_application.Destroy();
    }

    AZ::ComponentApplication m_application;
    AZ::Entity* m_systemEntity{};
};

TEST_F(VideoPlaybackFrameworkTestApp, VideoPlaybackFramework_BasicApp)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV); // Gruber patch // VMED
