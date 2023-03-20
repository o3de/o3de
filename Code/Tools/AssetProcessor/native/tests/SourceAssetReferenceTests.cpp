/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AssetManager/SourceAssetReference.h>
#include "UnitTestUtilities.h"

namespace UnitTests
{
    using SourceAssetReferenceTests = ::UnitTest::LeakDetectionFixture;

    TEST_F(SourceAssetReferenceTests, Construct_AbsolutePath_Succeeds)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test1{ QString(path) };
        SourceAssetReference test2{ path };
        SourceAssetReference test3{ AZ::IO::Path(path) };

        EXPECT_EQ(test1.AbsolutePath(), path);
        EXPECT_EQ(test2.AbsolutePath(), path);
        EXPECT_EQ(test3.AbsolutePath(), path);

        EXPECT_EQ(test1.RelativePath(), "file.png");
        EXPECT_EQ(test1.ScanFolderPath(), "c:/somepath");
        EXPECT_EQ(test1.ScanFolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, Construct_ScanFolderPath_Succeeds)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";
        constexpr const char* scanfolder = "c:/somepath";
        constexpr const char* relative = "file.png";

        SourceAssetReference test1{ QString(scanfolder), QString(relative) };
        SourceAssetReference test2{ scanfolder, relative };
        SourceAssetReference test3{ AZ::IO::Path(scanfolder), AZ::IO::Path(relative) };

        EXPECT_EQ(test1.AbsolutePath(), path);
        EXPECT_EQ(test2.AbsolutePath(), path);
        EXPECT_EQ(test3.AbsolutePath(), path);

        EXPECT_EQ(test1.RelativePath(), relative);
        EXPECT_EQ(test1.ScanFolderPath(), scanfolder);
        EXPECT_EQ(test1.ScanFolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, Construct_ScanFolderId_Succeeds)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";
        constexpr const char* scanfolder = "c:/somepath";
        constexpr const char* relative = "file.png";

        SourceAssetReference test1{ 1, relative };

        EXPECT_EQ(test1.AbsolutePath(), path);
        EXPECT_EQ(test1.RelativePath(), relative);
        EXPECT_EQ(test1.ScanFolderPath(), scanfolder);
        EXPECT_EQ(test1.ScanFolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, Copy_Succeeds)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test1{ QString(path) };

        auto test2 = test1;
        auto test3(test1);

        EXPECT_EQ(test1.AbsolutePath(), test2.AbsolutePath());
        EXPECT_EQ(test2.AbsolutePath(), test3.AbsolutePath());

        EXPECT_EQ(test1.RelativePath(), test2.RelativePath());
        EXPECT_EQ(test2.RelativePath(), test3.RelativePath());

        EXPECT_EQ(test1.ScanFolderPath(), test2.ScanFolderPath());
        EXPECT_EQ(test2.ScanFolderPath(), test3.ScanFolderPath());

        EXPECT_EQ(test1.ScanFolderId(), test2.ScanFolderId());
        EXPECT_EQ(test2.ScanFolderId(), test3.ScanFolderId());
    }

    TEST_F(SourceAssetReferenceTests, Move_Succeeds)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test1{ QString(path) };

        auto test2 = AZStd::move(test1);

        EXPECT_EQ(test2.AbsolutePath(), path);
        EXPECT_EQ(test2.RelativePath(), "file.png");
        EXPECT_EQ(test2.ScanFolderPath(), "c:/somepath");
        EXPECT_EQ(test2.ScanFolderId(), 1);

        auto test3(AZStd::move(test2));

        EXPECT_EQ(test3.AbsolutePath(), path);
        EXPECT_EQ(test3.RelativePath(), "file.png");
        EXPECT_EQ(test3.ScanFolderPath(), "c:/somepath");
        EXPECT_EQ(test3.ScanFolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, BoolCheck_EmptyReference_ReturnsFalse)
    {
        using namespace AssetProcessor;

        SourceAssetReference test;

        EXPECT_FALSE(test);
    }

    TEST_F(SourceAssetReferenceTests, BoolCheck_ValidReference_ReturnsTrue)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test{ path };

        EXPECT_TRUE(test);
    }

    TEST_F(SourceAssetReferenceTests, SamePaths_AreEqual)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test1{ path };
        SourceAssetReference test2{ path };

        EXPECT_EQ(test1, test2);
    }

    TEST_F(SourceAssetReferenceTests, DifferentPaths_AreNotEqual)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        SourceAssetReference test1{ "c:/somepath/file.png" };
        SourceAssetReference test2{ "c:/somepath/file2.png" };

        EXPECT_NE(test1, test2);
    }

    TEST_F(SourceAssetReferenceTests, Comparison)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        SourceAssetReference test1{ "c:/somepath/file1.png" };
        SourceAssetReference test2{ "c:/somepath/file2.png" };

        EXPECT_LT(test1, test2);
        EXPECT_GT(test2, test1);
    }
}
