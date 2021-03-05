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


#include <platform.h>
#include "PathHelpers.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <CryCommon/LegacyAllocator.h>

namespace PathHelpersTest
{
    class CryCommonToolsPathHelpersTest
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            AZ::AllocatorInstance<CryStringAllocator>::Create();
        }

        void TearDown()
        {
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
            AZ::AllocatorInstance<CryStringAllocator>::Destroy();
            UnitTest::AllocatorsTestFixture::TearDown();
        }
    };

    TEST_F(CryCommonToolsPathHelpersTest, FindExtension_StringPathNoExtension_ReturnsEmptyString)
    {
        const char* filePath = "ext";
        string result = PathHelpers::FindExtension(filePath);
        EXPECT_STREQ("", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, FindExtension_StringPath_ReturnsStringExtension)
    {
        const char* extension = "ext";
        const char* filePath = "foo.ext";
        string result = PathHelpers::FindExtension(filePath);
        EXPECT_STREQ(extension, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, FindExtension_WStringPathNoExtension_ReturnsEmptyString)
    {
        const wchar_t filePath[] = L"ext";
        const wchar_t expectedResult[] = L"";
        const wstring result = PathHelpers::FindExtension(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, FindExtension_WStringPath_ReturnsStringExtension)
    {
        const wchar_t extension[] = L"ext";
        const wchar_t filePath[] = L"foo.ext";
        const wstring result = PathHelpers::FindExtension(filePath);
        EXPECT_TRUE(result == extension);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_EmptyStringPath_ReturnsEmptyString)
    {
        const char* filePath = "";
        const char* newExtension = "new";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_StringNoExtension_ReturnsStringNoExtension)
    {
        const char* filePath = "foo.ext";
        const char* newExtension = "";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ("foo", result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_StringPathWithDoubleBackSlash_ReturnsUnalteredString)
    {
        const char* filePath = "foo.ext\\";
        const char* newExtension = "new";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ(filePath, result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_StringPathWithForwardSlash_ReturnsUnalteredString)
    {
        const char* filePath = "foo.ext/";
        const char* newExtension = "new";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_StringPathWithColon_ReturnsUnalteredString)
    {
        const char* filePath = "foo.ext:";
        const char* newExtension = "new";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_StringPathEndsWithPeriod_ReturnsUnalteredString)
    {
        const char* filePath = "foo.ext.";
        const char* newExtension = "new";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_StringNewExtension_ReturnsStringWithNewExtension)
    {
        const char* filePath = "foo.ext";
        const char* newExtension = "new";
        string result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_STREQ("foo.new", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_EmptyWStringPath_ReturnsEmptyWString)
    {
        const wchar_t filePath[] = L"";
        const wchar_t newExtension[] = L"new";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_WStringNoExtension_ReturnsWStringNoExtension)
    {
        const wchar_t filePath[] = L"foo.ext";
        const wchar_t newExtension[] = L"";
        const wchar_t expectedResult[] = L"foo";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == expectedResult);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_WStringPathWithDoubleBackSlash_ReturnsUnalteredWString)
    {
        const wchar_t filePath[] = L"foo.ext\\";
        const wchar_t newExtension[] = L"new";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == filePath);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_WStringPathWithForwardSlash_ReturnsUnalteredWString)
    {
        const wchar_t filePath[] = L"foo.ext/";
        const wchar_t newExtension[] = L"new";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_WStringPathWithColon_ReturnsUnalteredWString)
    {
        const wchar_t filePath[] = L"foo.ext:";
        const wchar_t newExtension[] = L"new";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_WStringPathEndsWithPeriod_ReturnsUnalteredWString)
    {
        const wchar_t filePath[] = L"foo.ext.";
        const wchar_t newExtension[] = L"new";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ReplaceExtension_WStringNewExtension_ReturnsWStringWithNewExtension)
    {
        const wchar_t filePath[] = L"foo.ext";
        const wchar_t newExtension[] = L"new";
        const wchar_t expectedResult[] = L"foo.new";
        wstring result = PathHelpers::ReplaceExtension(filePath, newExtension);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveExtension_StringPathNoExtension_ReturnsUnalteredString)
    {
        const char* filePath = "foo";
        string result = PathHelpers::RemoveExtension(filePath);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveExtension_StringPath_ReturnsStringWithoutExtension)
    {
        const char* filePath = "foo.bar";
        string result = PathHelpers::RemoveExtension(filePath);
        EXPECT_STREQ("foo", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveExtension_WStringPathNoExtension_ReturnsUnalteredWString)
    {
        const wchar_t filePath[] = L"foo";
        wstring result = PathHelpers::RemoveExtension(filePath);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveExtension_WStringPath_ReturnsWStringWithoutExtension)
    {
        const wchar_t filePath[] = L"foo.bar";
        const wchar_t expectedResult[] = L"foo";
        wstring result = PathHelpers::RemoveExtension(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_StringPathWithColon_RemovesCharactersAfterColon)
    {
        const char* filePath = "foo:bar";
        string result = PathHelpers::GetDirectory(filePath);
        EXPECT_STREQ("foo:", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_StringPathWithColonAsCharacterBeforeLastSeparator_RemovesCharactersAfterLastSeparator)
    {
        const char* filePath = "foo:/bar";
        string result = PathHelpers::GetDirectory(filePath);
        EXPECT_STREQ("foo:/", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_StringPathWithLastSeparatorAsFirstCharacter_ReturnsStringColon)
    {
        const char* filePath = ":foo";
        string result = PathHelpers::GetDirectory(filePath);
        EXPECT_STREQ(":", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_StringPathStartsWithForwardSlash_ReturnsFullString)
    {
        const char* filePath = "//foo";
        string result = PathHelpers::GetDirectory(filePath);
        EXPECT_STREQ(filePath, result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_StringPathStartsWithDoubleBackSlash_ReturnsFullString)
    {
        const char* filePath = "\\\\foo";
        string result = PathHelpers::GetDirectory(filePath);
        EXPECT_STREQ(filePath, result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_StringPath_ReturnsOnlyStringPath)
    {
        const char* filePath = "foobar/";
        string result = PathHelpers::GetDirectory(filePath);
        EXPECT_STREQ("foobar", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_WStringPathWithColon_RemovesCharactersAfterColon)
    {
        const wchar_t filePath[] = L"foo:bar";
        const wchar_t expectedResult[] = L"foo:";
        wstring result = PathHelpers::GetDirectory(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_WStringPathWithColonAsCharacterBeforeLastSeparator_RemovesCharactersAfterLastSeparator)
    {
        const wchar_t filePath[] = L"foo:/bar";
        const wchar_t expectedResult[] = L"foo:/";
        wstring result = PathHelpers::GetDirectory(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_WStringPathWithLastSeparatorAsFirstCharacter_ReturnsWStringColon)
    {
        const wchar_t filePath[] = L":foo";
        const wchar_t expectedResult[] = L":";
        wstring result = PathHelpers::GetDirectory(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_WStringPathStartsWithForwardSlash_ReturnsFullWString)
    {
        const wchar_t filePath[] = L"//foo";
        wstring result = PathHelpers::GetDirectory(filePath);
        EXPECT_TRUE(result == filePath);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_WStringPathStartsWithDoubleBackSlash_ReturnsFullWString)
    {
        const wchar_t filePath[] = L"\\\\foo";
        wstring result = PathHelpers::GetDirectory(filePath);
        EXPECT_TRUE(result == filePath);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, GetDirectory_WStringPath_ReturnsOnlyWStringPath)
    {
        const wchar_t filePath[] = L"foobar/";
        const wchar_t expectedResult[] = L"foobar";
        wstring result = PathHelpers::GetDirectory(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetFilename_StringPathStartsWithForwardSlash_ReturnsEmptyString)
    {
        const char* filePath = "/:foobar";
        string result = PathHelpers::GetFilename(filePath);
        EXPECT_STREQ("", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetFilename_StringPathStartsWithDoubleBackSlash_ReturnsEmptyString)
    {
        const char* filePath = "\\:foobar";
        string result = PathHelpers::GetFilename(filePath);
        EXPECT_STREQ("", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetFilename_StringPath_ReturnsStringFilename)
    {
        const char* filePath = "/foo/foo/foobar";
        string result = PathHelpers::GetFilename(filePath);
        EXPECT_STREQ("foobar", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetFilename_WStringPathStartsWithForwardSlash_ReturnsEmptyWString)
    {
        const wchar_t filePath[] = L"/:foobar";
        const wchar_t expectedResult[] = L"";
        wstring result = PathHelpers::GetFilename(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, GetFilename_WStringPathStartsWithDoubleBackSlash_ReturnsEmptyWString)
    {
        const wchar_t filePath[] = L"\\:foobar";
        const wchar_t expectedResult[] = L"";
        wstring result = PathHelpers::GetFilename(filePath);
        EXPECT_TRUE(result == expectedResult);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, GetFilename_WStringPath_ReturnsWStringFilename)
    {
        const wchar_t filePath[] = L"/foo/foo/foobar";
        const wchar_t expectedResult[] = L"foobar";
        wstring result = PathHelpers::GetFilename(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_EmptyPath_ReturnsEmptyString)
    {
        const char* filePath = "";
        string result = PathHelpers::AddSeparator(filePath);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_StringPathEndsWithForwardSlash_ReturnsStringPath)
    {
        const char* filePath = "foo/";
        string result = PathHelpers::AddSeparator(filePath);
        EXPECT_STREQ(filePath, result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_StringPathEndsWithDoubleBackSlash_ReturnsStringPath)
    {
        const char* filePath = "foo\\";
        string result = PathHelpers::AddSeparator(filePath);
        EXPECT_STREQ(filePath, result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_StringPathEndsWithColon_ReturnsStringPath)
    {
        const char* filePath = "foo:";
        string result = PathHelpers::AddSeparator(filePath);
        EXPECT_STREQ(filePath, result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_StringPath_ReturnsStringWithDoubleBackSlashAdded)
    {
        const char* filePath = "foo";
        string result = PathHelpers::AddSeparator(filePath);
        EXPECT_STREQ("foo\\", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_EmptyPath_ReturnsEmptyWString)
    {
        const wchar_t filePath[] = L"";
        wstring result = PathHelpers::AddSeparator(filePath);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_WStringPathEndsWithForwardSlash_ReturnsWStringPath)
    {
        const wchar_t filePath[] = L"foo/";
        wstring result = PathHelpers::AddSeparator(filePath);
        EXPECT_TRUE(result == filePath);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_WStringPathEndsWithDoubleBackSlash_ReturnsWStringPath)
    {
        const wchar_t filePath[] = L"foo\\";
        wstring result = PathHelpers::AddSeparator(filePath);
        EXPECT_TRUE(result == filePath);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_WStringPathEndsWithColon_ReturnsWStringPath)
    {
        const wchar_t filePath[] = L"foo:";
        wstring result = PathHelpers::AddSeparator(filePath);
        EXPECT_TRUE(result == filePath);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, AddSeparator_WStringPath_ReturnsWStringWithDoubleBackSlashAdded)
    {
        const wchar_t filePath[] = L"foo";
        const wchar_t expectedResult[] = L"foo\\";
        wstring result = PathHelpers::AddSeparator(filePath);
        EXPECT_TRUE(result == expectedResult);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_EmptyStringPath_ReturnsEmptyString)
    {
        const char* filePath = "";
        string result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_StringPathEndsWithForwardSlash_ReturnsStringWithoutForwardSlash)
    {
        const char* filePath = "foo/";
        string result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_STREQ("foo", result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_StringPathEndsWithDoubleBackSlash_ReturnsStringWithoutDoubleBackSlash)
    {
        const char* filePath = "foo\\";
        string result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_STREQ("foo", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_StringPath_ReturnsStringPath)
    {
        const char* filePath = "foo";
        string result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_STREQ(filePath, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_EmptyWStringPath_ReturnsEmptyWString)
    {
        const wchar_t filePath[] = L"";
        wstring result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_WStringPathEndsWithForwardSlash_ReturnsWStringWithoutForwardSlash)
    {
        const wchar_t filePath[] = L"foo/";
        const wchar_t expectedResult[] = L"foo";
        wstring result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_WStringPathEndsWithDoubleBackSlash_ReturnsWStringWithoutDoubleBackSlash)
    {
        const wchar_t filePath[] = L"foo\\";
        const wchar_t expectedResult[] = L"foo";
        wstring result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_TRUE(result == expectedResult);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, RemoveSeparator_WStringPath_ReturnsWStringPath)
    {
        const wchar_t filePath[] = L"foo";
        wstring result = PathHelpers::RemoveSeparator(filePath);
        EXPECT_TRUE(result == filePath);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveDuplicateSeparators_StringPathLengthEqualOne_ReturnsStringPath)
    {
        const char* filePath = "f";
        string result = PathHelpers::RemoveDuplicateSeparators(filePath);
        EXPECT_STREQ(filePath, result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, RemoveDuplicateSeparators_StringPathWithDuplicateBackSlashes_ReturnsStringWithoutDoubleBackSlashes)
    {
        const char* filePath = "foo\\\\bar";
        string result = PathHelpers::RemoveDuplicateSeparators(filePath);
        EXPECT_STREQ("foo\\bar", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, RemoveDuplicateSeparators_StringPathWithDuplicateForwardSlashes_ReturnsStringWithoutForwardSlashes)
    {
        const char* filePath = "foo//bar";
        string result = PathHelpers::RemoveDuplicateSeparators(filePath);
        EXPECT_STREQ("foo/bar", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, RemoveDuplicateSeparators_WStringPathLengthEqualOne_ReturnsWStringPath)
    {
        const wchar_t filePath[] = L"f";
        wstring result = PathHelpers::RemoveDuplicateSeparators(filePath);
        EXPECT_TRUE(result == filePath);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, RemoveDuplicateSeparators_WStringPathWithDuplicateBackSlashes_ReturnsWStringWithoutDoubleBackSlashes)
    {
        const wchar_t filePath[] = L"foo\\\\bar";
        const wchar_t expectedResult[] = L"foo\\bar";
        wstring result = PathHelpers::RemoveDuplicateSeparators(filePath);
        EXPECT_TRUE(result == expectedResult);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, RemoveDuplicateSeparators_WStringPathWithDuplicateForwardSlashes_ReturnsWStringWithoutForwardSlashes)
    {
        const wchar_t filePath[] = L"foo//bar";
        const wchar_t expectedResult[] = L"foo/bar";
        wstring result = PathHelpers::RemoveDuplicateSeparators(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, Join_EmptySecondStringPath_ReturnsFirstString)
    {
        const char* filePath1 = "foo";
        const char* filePath2 = "";
        string result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_STREQ(filePath1, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, Join_EmptyFirstStringPath_ReturnsSecondString)
    {
        const char* filePath1 = "";
        const char* filePath2 = "bar";
        string result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_STREQ(filePath2, result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, Join_StringPath_ReturnsStringAppendedWithDoubleBackSlashDivider)
    {
        const char* filePath1 = "foo";
        const char* filePath2 = "bar";
        string result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_STREQ("foo\\bar", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, Join_FirstStringPathEndsWithForwardSlash_ReturnsAppendedString)
    {
        const char* filePath1 = "foo/";
        const char* filePath2 = "bar";
        string result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_STREQ("foo/bar", result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, Join_FirstStringPathEndsWithDoubleBackSlash_ReturnsAppendedString)
    {
        const char* filePath1 = "foo\\";
        const char* filePath2 = "bar";
        string result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_STREQ("foo\\bar", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, Join_FirstStringPathEndsWithColon_ReturnsAppendedString)
    {
        const char* filePath1 = "foo:";
        const char* filePath2 = "bar";
        string result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_STREQ("foo:bar", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, Join_EmptySecondWStringPath_ReturnsFirstWString)
    {
        const wchar_t filePath1[] = L"foo";
        const wchar_t filePath2[] = L"";
        wstring result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_TRUE(result == filePath1);
    }

    TEST_F(CryCommonToolsPathHelpersTest, Join_EmptyFirstWStringPath_ReturnsSecondWString)
    {
        const wchar_t filePath1[] = L"";
        const wchar_t filePath2[] = L"bar";
        wstring result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_TRUE(result == filePath2);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, Join_WStringPath_ReturnsWStringAppendedWithDoubleBackSlashDivider)
    {
        const wchar_t filePath1[] = L"foo";
        const wchar_t filePath2[] = L"bar";
        const wchar_t expectedResult[] = L"foo\\bar";
        wstring result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_TRUE(result == expectedResult);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, Join_FirstWStringPathEndsWithForwardSlash_ReturnsAppendedWString)
    {
        const wchar_t filePath1[] = L"foo/";
        const wchar_t filePath2[] = L"bar";
        const wchar_t expectedResult[] = L"foo/bar";
        wstring result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_TRUE(result == expectedResult);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, Join_FirstWStringPathEndsWithDoubleBackSlash_ReturnsAppendedWString)
    {
        const wchar_t filePath1[] = L"foo\\";
        const wchar_t filePath2[] = L"bar";
        const wchar_t expectedResult[] = L"foo\\bar";
        wstring result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_TRUE(result == expectedResult);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, Join_FirstWStringPathEndsWithColon_ReturnsAppendedWString)
    {
        const wchar_t filePath1[] = L"foo:";
        const wchar_t filePath2[] = L"bar";
        const wchar_t expectedResult[] = L"foo:bar";
        wstring result = PathHelpers::Join(filePath1, filePath2);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_EmptyStringPath_ReturnsTrue)
    {
        const char* filePath = "";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_TRUE(result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_StringPath_ReturnsTrue)
    {
        const char* filePath = "foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_TRUE(result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_StringPathBeginsWithForwardSlash_ReturnsFalse)
    {
        const char* filePath = "/foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_FALSE(result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_StringPathBeginsWithDoubleBackSlash_ReturnsFalse)
    {
        const char* filePath = "\\foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_FALSE (result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_StringPathBeginsWithColon_ReturnsFalse)
    {
        const char* filePath = ":foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_FALSE(result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_EmptyWStringPath_ReturnsTrue)
    {
        const wchar_t filePath[] = L"";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_TRUE(result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_WStringPath_ReturnsTrue)
    {
        const wchar_t filePath[] = L"foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_TRUE(result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_WStringPathBeginsWithForwardSlash_ReturnsFalse)
    {
        const wchar_t filePath[] = L"/foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_FALSE(result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_WStringPathBeginsWithDoubleBackSlash_ReturnsFalse)
    {
        const wchar_t filePath[] = L"\\foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_FALSE(result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, IsRelative_WStringPathBeginsWithColon_ReturnsFalse)
    {
        const wchar_t filePath[] = L":foo";
        bool result = PathHelpers::IsRelative(filePath);
        EXPECT_FALSE(result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ToUnixPath_StringPath_ReturnsStringWithForwardSlashes)
    {
        const char* filePath = "foo\\foo\\foo";
        string result = PathHelpers::ToUnixPath(filePath);
        EXPECT_STREQ("foo/foo/foo", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ToUnixPath_WStringPath_ReturnsWStringWithForwardSlashes)
    {
        const wchar_t filePath[] = L"foo\\foo\\foo";
        const wchar_t expectedResult[] = L"foo/foo/foo";
        wstring result = PathHelpers::ToUnixPath(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ToDosPath_StringPath_ReturnsStringWithDoubleBackSlashes)
    {
        const char* filePath = "foo/foo/foo";
        string result = PathHelpers::ToDosPath(filePath);
        EXPECT_STREQ("foo\\foo\\foo", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, ToDosPath_WStringPath_ReturnsStringWithDoubleBackSlashes)
    {
        const wchar_t filePath[] = L"foo/foo/foo";
        const wchar_t expectedResult[] = L"foo\\foo\\foo";
        wstring result = PathHelpers::ToDosPath(filePath);
        EXPECT_TRUE(result == expectedResult);
    }

    TEST_F(CryCommonToolsPathHelpersTest, GetAsciiPath_EmptyStringPath_ReturnsEmpty)
    {
        const char* filePath = "";
        string result = PathHelpers::GetAsciiPath(filePath);
        EXPECT_STREQ(filePath, result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, GetAsciiPath_StringPath_ReturnsStringWithoutForwardSlash)
    {
        const char* filePath = "foo/bar/";
        string result = PathHelpers::GetAsciiPath(filePath);
        EXPECT_STREQ("foo\\bar", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS

    TEST_F(CryCommonToolsPathHelpersTest, GetAsciiPath_EmptyWStringPath_ReturnsEmpty)
    {
        const wchar_t filePath[] = L"";
        string expectedResult = "";
        string result = PathHelpers::GetAsciiPath(filePath);
        EXPECT_STREQ(expectedResult, result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, CanonicalizePath_StringPathLengthLessThanThree_ReturnsStringWithoutForwardSlash)
    {
        const char* filePath = "./";
        string result = PathHelpers::CanonicalizePath(filePath);
        EXPECT_STREQ(".", result);
    }

    TEST_F(CryCommonToolsPathHelpersTest, CanonicalizePath_StringPathStartsWithPeriodForwardSlash_ReturnsStringWithoutPeriodAndForwardSlash)
    {
        const char* filePath = "./foo";
        string result = PathHelpers::CanonicalizePath(filePath);
        EXPECT_STREQ("foo", result);
    }

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
    TEST_F(CryCommonToolsPathHelpersTest, CanonicalizePath_StringPathStartsWithPeriodDoubleBackSlash_ReturnsStringWithoutPeriodAndDoubleBackSlash)
    {
        const char* filePath = ".\\foo";
        string result = PathHelpers::CanonicalizePath(filePath);
        EXPECT_STREQ("foo", result);
    }
#endif // AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
