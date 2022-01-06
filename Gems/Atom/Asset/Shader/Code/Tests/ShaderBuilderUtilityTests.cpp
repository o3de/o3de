/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include "Common/ShaderBuilderTestFixture.h"

#include <ShaderBuilderUtility.h>

namespace UnitTest
{
    using namespace AZ;

    // The main purpose of this class is to test ShaderBuilderUtility functions
    class ShaderBuilderUtilityTests : public ShaderBuilderTestFixture
    {
    }; // class ShaderBuilderUtilityTests


    TEST_F(ShaderBuilderUtilityTests, IncludedFilesParser_ParseStringAndGetIncludedFiles)
    {
        AZStd::string haystack(
            "Some content to parse\n"
            "#include <valid_file1.azsli>\n"
            "// #include <valid_file2.azsli>\n"
            "blah # include \"valid_file3.azsli\"\n"
            "bar include <a\\dire-ctory\\invalid-file4.azsli>\n"
            "foo #   include \"a/directory/valid-file5.azsli\"\n"
            "# include <a\\dire-ctory\\valid-file6.azsli>\n"
            "#includ \"a\\dire-ctory\\invalid-file7.azsli\"\n"
        );

        AZ::ShaderBuilder::ShaderBuilderUtility::IncludedFilesParser includedFilesParser;
        auto fileList = includedFilesParser.ParseStringAndGetIncludedFiles(haystack);
        EXPECT_EQ(fileList.size(), 5);

        auto it = AZStd::find(fileList.begin(), fileList.end(), "valid_file1.azsli");
        EXPECT_TRUE(it != fileList.end());

        it = AZStd::find(fileList.begin(), fileList.end(), "valid_file2.azsli");
        EXPECT_TRUE(it != fileList.end());

        it = AZStd::find(fileList.begin(), fileList.end(), "valid_file3.azsli");
        EXPECT_TRUE(it != fileList.end());

        // Remark: From now on We must normalize because internally AZ::ShaderBuilder::ShaderBuilderUtility::IncludedFilesParser
        // always returns normalized paths.
        {
            AZStd::string fileName("a\\dire-ctory\\invalid-file4.azsli");
            AzFramework::StringFunc::Path::Normalize(fileName);
            it = AZStd::find(fileList.begin(), fileList.end(), fileName);
            EXPECT_TRUE(it == fileList.end());
        }

        {
            AZStd::string fileName("a\\directory\\valid-file5.azsli");
            AzFramework::StringFunc::Path::Normalize(fileName);
            it = AZStd::find(fileList.begin(), fileList.end(), fileName);
            EXPECT_TRUE(it != fileList.end());
        }

        {
            AZStd::string fileName("a\\dire-ctory\\valid-file6.azsli");
            AzFramework::StringFunc::Path::Normalize(fileName);
            it = AZStd::find(fileList.begin(), fileList.end(), fileName);
            EXPECT_TRUE(it != fileList.end());
        }

        {
            AZStd::string fileName("a\\dire-ctory\\invalid-file7.azsli");
            AzFramework::StringFunc::Path::Normalize(fileName);
            it = AZStd::find(fileList.begin(), fileList.end(), fileName);
            EXPECT_TRUE(it == fileList.end());
        }
    }

} //namespace UnitTest

//AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

