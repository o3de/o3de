/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// we only want to do the tests for the dll version of our lib. Otherwise, they'd get run twice
#if defined(AZ_QT_COMPONENTS_EXPORT_SYMBOLS)

#include <AzTest/AzTest.h>

#include <AzQtComponents/Components/StyleSheetCache.h>

namespace AzQtComponents
{
    class StyleSheetCacheTests : public ::testing::Test
    {
    public:
        AzQtComponents::StyleSheetCache* cache = nullptr;
        void SetUp() override
        {
            cache = new StyleSheetCache(nullptr);
        }
        void TearDown() override
        {
            delete cache;
        }
    };

    TEST_F(StyleSheetCacheTests, TestLoadWithFallback)
    {
        auto pathOnDisk = QStringLiteral("Code/Framework/AzQtComponents/AzQtComponents/Tests/qrc2");
        auto qrcPath = QStringLiteral(":/StyleSheetTests2");
        cache->setFallbackSearchPaths("fallback", pathOnDisk, qrcPath);

        // The fallback qss color is blue
        QString sheet = cache->loadStyleSheet("sheet1.qss");
        EXPECT_TRUE(sheet == QString("QLabel { background-color: blue; }\n"));

        // Register a primary QRC
        pathOnDisk = QStringLiteral("Code/Framework/AzQtComponents/AzQtComponents/Tests/qrc1");
        qrcPath = QStringLiteral(":/StyleSheetTests1");
        cache->addSearchPaths("test", pathOnDisk, qrcPath);

        // sheet2 has no fallback
        sheet = cache->loadStyleSheet("sheet2.qss");
        EXPECT_TRUE(sheet == QString("QComboBox { color: blue; }\n"));
        // sheet1 is still cached, so should still be blue
        sheet = cache->loadStyleSheet("sheet1.qss");
        EXPECT_TRUE(sheet == QString("QLabel { background-color: blue; }\n"));

        // with a cleared cache, sheet1 should be pulled from "test" before "fallback"
        cache->clearCache();
        sheet = cache->loadStyleSheet("sheet1.qss");
        EXPECT_TRUE(sheet == QString("QLabel { background-color: red; }\n"));
    }
} // namespace AzQtComponents

#endif // defined(AZ_QT_COMPONENTS_EXPORT_SYMBOLS)
