/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineTest.h"
#include <Mocks/ISystemMock.h>
#include <Sprite.h>

namespace UnitTest
{
    class LyShineSpriteTest
        : public LyShineTest
    {
    protected:

        void SetupApplication() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
            appDesc.m_recordingMode = AZ::Debug::AllocationRecords::RECORD_FULL;

            AZ::ComponentApplication::StartupParameters appStartup;
            appStartup.m_createStaticModulesCallback =
                [](AZStd::vector<AZ::Module*>& modules)
            {
                modules.emplace_back(new LyShine::LyShineModule);
            };

            m_application = aznew AZ::ComponentApplication();
            m_systemEntity = m_application->Create(appDesc, appStartup);
            m_systemEntity->Init();
            m_systemEntity->Activate();
        }

        void SetupEnvironment() override
        {
            LyShineTest::SetupEnvironment();

        }

        void TearDown() override
        {
            LyShineTest::TearDown();
        }

    };

#ifdef LYSHINE_ATOM_TODO // [GHI #6270] Support RTT using Atom
    TEST_F(LyShineSpriteTest, Sprite_CanAcquireRenderTarget)
    {
        // initialize to create the static sprite cache
        CSprite::Initialize();

        const char* renderTargetName = "$test";
        EXPECT_CALL(m_data->m_renderer, EF_GetTextureByName(testing::_, testing::_)).WillRepeatedly(testing::Return(nullptr));

        CSprite* sprite = CSprite::CreateSprite(renderTargetName);
        ASSERT_NE(sprite, nullptr);

        ITexture* texture = sprite->GetTexture();
        EXPECT_EQ(texture, nullptr);

        ITextureMock* mockTexture = new testing::NiceMock<ITextureMock>;

        // expect the sprite acquires the texture and increments the reference count
        EXPECT_CALL(m_data->m_renderer, EF_GetTextureByName(testing::_, testing::_)).WillOnce(testing::Return(mockTexture));
        EXPECT_CALL(*mockTexture, AddRef()).Times(1);

        texture = sprite->GetTexture();
        EXPECT_EQ(texture, mockTexture);

        // the sprite should attempt to release the texture by calling ReleaseResourceAsync
        // Using the A<type> matcher to force the type to be AZStd::unique_ptr<SResourceAsync>, but allow any value to be used for that type
        EXPECT_CALL(m_data->m_renderer, ReleaseResourceAsync(testing::A<AZStd::unique_ptr<SResourceAsync>>())).Times(1);

        delete sprite;
        sprite = nullptr;

        CSprite::Shutdown();
        delete mockTexture;
    }

    TEST_F(LyShineSpriteTest, Sprite_CanCreateWithExistingRenderTarget)
    {
        // initialize to create the static sprite cache
        CSprite::Initialize();

        ITextureMock* mockTexture = new testing::NiceMock<ITextureMock>;

        const char* renderTargetName = "$test";
        EXPECT_CALL(m_data->m_renderer, EF_GetTextureByName(testing::_, testing::_)).WillRepeatedly(testing::Return(mockTexture));

        // the sprite should increment the ref count on the texture
        EXPECT_CALL(*mockTexture, AddRef()).Times(1);

        CSprite* sprite = CSprite::CreateSprite(renderTargetName);
        ASSERT_NE(sprite, nullptr);

        ITexture* texture = sprite->GetTexture();
        EXPECT_EQ(texture, mockTexture);

        // the sprite should attempt to release the texture by calling ReleaseResourceAsync
        EXPECT_CALL(m_data->m_renderer, ReleaseResourceAsync(testing::A<AZStd::unique_ptr<SResourceAsync>>())).Times(1);

        delete sprite;
        sprite = nullptr;

        CSprite::Shutdown();
        delete mockTexture;
    }
#endif
} //namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
