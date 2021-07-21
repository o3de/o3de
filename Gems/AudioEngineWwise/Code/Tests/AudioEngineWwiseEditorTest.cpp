/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AudioSystemEditor_wwise.h>

namespace UnitTest
{
    class AudioEngineWwiseEditorTests
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }
        }

        void TearDown() override
        {
            if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }
    };

    TEST_F(AudioEngineWwiseEditorTests, CreateControl_LocalizedBankPath_NameDoesNotContainPath)
    {
        using namespace AudioControls;
        CAudioSystemEditor_wwise editorWwise;
        SControlDef controlDefinition("LocalizedBank", eWCT_WWISE_SOUND_BANK, true, nullptr, "en-us");
        auto audioControl = editorWwise.CreateControl(controlDefinition);
        // Ensure audioControl's name does not contain "en-us".
        AZStd::string name = audioControl->GetName();
        EXPECT_EQ(name.find("en-us"), AZStd::string::npos);
    }

} // namespace UnitTest

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
