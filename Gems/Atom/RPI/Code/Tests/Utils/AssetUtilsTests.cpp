/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Common/RPITestFixture.h>
#include <Common/ErrorMessageFinder.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    class AssetUtilsTests
        : public RPITestFixture
    {
    protected:
        void SetUp() override
        {
            RPITestFixture::SetUp();
        }

        void TearDown() override
        {
            RPITestFixture::TearDown();
        }

    };

    TEST_F(AssetUtilsTests, TestSanitizeFilename)
    {
        static const char AllSupportedCharacters[] = "abcdefghijklmnopqrstufwxyz-ABCDEFGHIJKLMNOPQRSTUFWXYZ_0123456789.txt";

        EXPECT_STREQ("", AssetUtils::SanitizeFileName("").c_str());
        EXPECT_STREQ(AllSupportedCharacters, AssetUtils::SanitizeFileName(AllSupportedCharacters).c_str());
        EXPECT_STREQ("a_b_c_d_e_f_g_h_i_jklmnopqrstufwxyz", AssetUtils::SanitizeFileName(R"(a\b/c:d*e?f\"g<h>i|jklmnopqrstufwxyz)").c_str());
        EXPECT_STREQ("Hello_World", AssetUtils::SanitizeFileName(R"(Hello<::\*/::>World)").c_str());
        EXPECT_STREQ("CantEndWithDot", AssetUtils::SanitizeFileName("CantEndWithDot.").c_str());
        EXPECT_STREQ("Cant_Have_Multiple_Dots_", AssetUtils::SanitizeFileName("Cant..Have...Multiple....Dots...").c_str());
        EXPECT_STREQ("Prevent_MultipleUnderscores", AssetUtils::SanitizeFileName(R"(Prevent<::_\*/_::>MultipleUnderscores)").c_str());
        // These characters might be allowed by the OS, but they are prevented in FileDialog::GetSaveFileName
        EXPECT_STREQ("Other_Characters", AssetUtils::SanitizeFileName("Other~!@#$%^&*()+=[]{},;'`Characters").c_str());
    }
}
