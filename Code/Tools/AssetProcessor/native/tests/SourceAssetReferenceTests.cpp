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
    using SourceAssetReferenceTests = ::UnitTest::ScopedAllocatorSetupFixture;

    TEST_F(SourceAssetReferenceTests, ConstructBasic)
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
        EXPECT_EQ(test1.ScanfolderPath(), "c:/somepath");
        EXPECT_EQ(test1.ScanfolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, ConstructScanFolderPath)
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
        EXPECT_EQ(test1.ScanfolderPath(), scanfolder);
        EXPECT_EQ(test1.ScanfolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, ConstructScanFolderId)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";
        constexpr const char* scanfolder = "c:/somepath";
        constexpr const char* relative = "file.png";

        SourceAssetReference test1{ 1, relative };

        EXPECT_EQ(test1.AbsolutePath(), path);
        EXPECT_EQ(test1.RelativePath(), relative);
        EXPECT_EQ(test1.ScanfolderPath(), scanfolder);
        EXPECT_EQ(test1.ScanfolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, Copy)
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

        EXPECT_EQ(test1.ScanfolderPath(), test2.ScanfolderPath());
        EXPECT_EQ(test2.ScanfolderPath(), test3.ScanfolderPath());

        EXPECT_EQ(test1.ScanfolderId(), test2.ScanfolderId());
        EXPECT_EQ(test2.ScanfolderId(), test3.ScanfolderId());
    }

    TEST_F(SourceAssetReferenceTests, Move)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test1{ QString(path) };

        auto test2 = AZStd::move(test1);

        EXPECT_EQ(test2.AbsolutePath(), path);
        EXPECT_EQ(test2.RelativePath(), "file.png");
        EXPECT_EQ(test2.ScanfolderPath(), "c:/somepath");
        EXPECT_EQ(test2.ScanfolderId(), 1);

        auto test3(AZStd::move(test2));

        EXPECT_EQ(test3.AbsolutePath(), path);
        EXPECT_EQ(test3.RelativePath(), "file.png");
        EXPECT_EQ(test3.ScanfolderPath(), "c:/somepath");
        EXPECT_EQ(test3.ScanfolderId(), 1);
    }

    TEST_F(SourceAssetReferenceTests, BoolCheckNegative)
    {
        using namespace AssetProcessor;

        SourceAssetReference test;

        EXPECT_FALSE(test);
    }

    TEST_F(SourceAssetReferenceTests, BoolCheckPositive)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test{ path };

        EXPECT_TRUE(test);
    }

    TEST_F(SourceAssetReferenceTests, Equality)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file.png";

        SourceAssetReference test1{ path };
        SourceAssetReference test2{ path };
        SourceAssetReference test3{ "c:/somepath/file2.png" };

        EXPECT_EQ(test1, test2);
        EXPECT_NE(test1, test3);
    }

    TEST_F(SourceAssetReferenceTests, Comparison)
    {
        using namespace AssetProcessor;

        MockPathConversion mockPathConversion;

        constexpr const char* path = "c:/somepath/file1.png";

        SourceAssetReference test1{ path };
        SourceAssetReference test2{ "c:/somepath/file2.png" };

        EXPECT_LT(test1, test2);
    }
}
