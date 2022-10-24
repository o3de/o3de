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
            " #include <..\\Relative\\Path\\To\\File.azsi>\n"
            " #include <C:\\Absolute\\Path\\To\\File.azsi>\n"
        );

        AZ::ShaderBuilder::ShaderBuilderUtility::IncludedFilesParser includedFilesParser;
        auto fileList = includedFilesParser.ParseStringAndGetIncludedFiles(haystack);
        EXPECT_EQ(fileList.size(), 7);


        auto expectHasIncludeFile = [fileList](bool shouldExist, const char* filePath)
        {
            //We must normalize because internally AZ::ShaderBuilder::ShaderBuilderUtility::IncludedFilesParser
            // always returns normalized paths.
            AZStd::string filePathNormalized(filePath);
            AzFramework::StringFunc::Path::Normalize(filePathNormalized);
            auto it = AZStd::find(fileList.begin(), fileList.end(), filePathNormalized);
            if (shouldExist)
            {
                EXPECT_TRUE(it != fileList.end()) << "Could not find path '" << filePath << "' in the include list.";
            }
            else
            {
                EXPECT_TRUE(it == fileList.end()) << "Path '" << filePath << "' should not be in the include list.";
            }
        };

        expectHasIncludeFile(true, "valid_file1.azsli");
        expectHasIncludeFile(true, "valid_file2.azsli");
        expectHasIncludeFile(true, "valid_file3.azsli");
        expectHasIncludeFile(false, "a\\dire-ctory\\invalid-file4.azsli");
        expectHasIncludeFile(true, "a\\directory\\valid-file5.azsli");
        expectHasIncludeFile(true, "a\\dire-ctory\\valid-file6.azsli");
        expectHasIncludeFile(false, "a\\dire-ctory\\invalid-file7.azsli");
        expectHasIncludeFile(true, "C:\\Absolute\\Path\\To\\File.azsi");
        expectHasIncludeFile(true, "..\\Relative\\Path\\To\\File.azsi");
    }

} //namespace UnitTest

//AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

