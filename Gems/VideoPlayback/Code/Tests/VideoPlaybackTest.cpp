/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "VideoPlayback_precompiled.h"

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Application/Application.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <VideoPlaybackGameComponent.h>

class VideoPlaybackFixture
        : public UnitTest::AllocatorsTestFixture
    {
    protected:
        void SetUp() override
        {
#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER
            m_app.Start(AZ::ComponentApplication::Descriptor{});
            m_app.RegisterComponentDescriptor(AZ::VideoPlayback::VideoPlaybackGameComponent::CreateDescriptor());

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
#endif
        }
        void TearDown() override
        {
#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER
            m_app.Stop();
#endif
        }


        AzFramework::Application m_app;
    };

#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER
TEST_F(VideoPlaybackFixture, VideoPlaybackSetUserTexture_FT)
{
    auto gameEntity = AZStd::make_unique<AZ::Entity>();
    
    auto videoPlaybackComponent = gameEntity->CreateComponent<AZ::VideoPlayback::VideoPlaybackGameComponent>();
    
    gameEntity->Init();
    gameEntity->Activate();
    
    AZStd::string textureName = "$testTexture";
    
    videoPlaybackComponent->SetDestinationTextureName(textureName);
    
    gameEntity->Deactivate();
    
    ASSERT_EQ(videoPlaybackComponent->GetDestinationTextureName(), textureName);
}

TEST_F(VideoPlaybackFixture, VideoPlaybackSetVideoPath_FT)
{
    auto gameEntity = AZStd::make_unique<AZ::Entity>();
    
    auto videoPlaybackComponent = gameEntity->CreateComponent<AZ::VideoPlayback::VideoPlaybackGameComponent>();
    
    gameEntity->Init();
    gameEntity->Activate();
    
    AZStd::string videoAssetPath = "testVideo.mp4";
    
    videoPlaybackComponent->SetVideoPathname(videoAssetPath);
    
    gameEntity->Deactivate();
    
    ASSERT_EQ(videoPlaybackComponent->GetVideoPathname(), videoAssetPath);
}
#endif

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
