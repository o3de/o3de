/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineTest.h"
#include <UiSerialize.h>

namespace UnitTest
{
    class LyShineSerializationTest
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

    TEST_F(LyShineSerializationTest, Serialization_LayoutErrorsOnNullptr_FT)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;

        UiSerialize::SetAnchorLeft(nullptr, .0f);
        UiSerialize::SetAnchorTop(nullptr, .0f);
        UiSerialize::SetAnchorRight(nullptr, .0f);
        UiSerialize::SetAnchorBottom(nullptr, .0f);
        UiSerialize::SetAnchors(nullptr, .0f, .0f, .0f, .0f);

        UiSerialize::SetOffsetLeft(nullptr, .0f);
        UiSerialize::SetOffsetTop(nullptr, .0f);
        UiSerialize::SetOffsetRight(nullptr, .0f);
        UiSerialize::SetOffsetBottom(nullptr, .0f);
        UiSerialize::SetOffsets(nullptr, .0f, .0f, .0f, .0f);

        UiSerialize::SetPaddingLeft(nullptr, 0);
        UiSerialize::SetPaddingTop(nullptr, 0);
        UiSerialize::SetPaddingRight(nullptr, 0);
        UiSerialize::SetPaddingBottom(nullptr, 0);
        UiSerialize::SetPadding(nullptr, 0, 0, 0, 0);

        AZ_TEST_STOP_TRACE_SUPPRESSION(15);
    }
} //namespace UnitTest
