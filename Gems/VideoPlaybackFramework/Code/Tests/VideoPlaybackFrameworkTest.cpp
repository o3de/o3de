/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <VideoPlaybackFrameworkModule.h>
#include <VideoPlaybackFrameworkSystemComponent.h>

using VideoPlaybackFrameworkTest = UnitTest::LeakDetectionFixture;
TEST_F(VideoPlaybackFrameworkTest, ComponentsWithComponentApplication)
{
    AZ::ComponentApplication::Descriptor appDesc;
    appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
    appDesc.m_recordingMode = AZ::Debug::AllocationRecords::Mode::RECORD_FULL;

    AZ::ComponentApplication app;
    AZ::ComponentApplication::StartupParameters startupParameters;
    startupParameters.m_loadSettingsRegistry = false;
    AZ::Entity* systemEntity = app.Create(appDesc, startupParameters);
    ASSERT_TRUE(systemEntity != nullptr);
    app.RegisterComponentDescriptor(VideoPlaybackFramework::VideoPlaybackFrameworkSystemComponent::CreateDescriptor());

    systemEntity->CreateComponent<VideoPlaybackFramework::VideoPlaybackFrameworkSystemComponent>();

    systemEntity->Init();
    systemEntity->Activate();

    app.Destroy();
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
