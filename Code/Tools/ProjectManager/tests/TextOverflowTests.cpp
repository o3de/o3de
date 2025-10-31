/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <TextOverflowWidget.h>

namespace O3DE::ProjectManager
{
    class TextOverflowWidgetTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        static bool EndsWithOverflowLink(const QString& str)
        {
            return str.endsWith(" <a href=\"OverflowLink\">Read More...</a>");
        }

        static const int s_testLength = 10;
    };

    TEST_F(TextOverflowWidgetTests, ElideText_UnderMaxLength_NoOverflow)
    {
        QString testStr = "";
        QString resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_FALSE(EndsWithOverflowLink(resultStr));

        testStr = "1234567890";
        resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_FALSE(EndsWithOverflowLink(resultStr));

        testStr = "1234<a href='https://www.o3de.org/'>56</a>7890";
        resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_FALSE(EndsWithOverflowLink(resultStr));
    }

    TEST_F(TextOverflowWidgetTests, ElideText_UnderMaxLength_NoTruncation)
    {
        QString testStr = "";
        QString resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(testStr == resultStr);

        testStr = "1234567890";
        resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(testStr == resultStr);

        testStr = "1234<a href='https://www.o3de.org/'>56</a>7890";
        resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(testStr == resultStr);
    }

    TEST_F(TextOverflowWidgetTests, ElideText_OverMaxLength_ShowOverflow)
    {
        QString testStr = "12345678901";
        QString resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(EndsWithOverflowLink(resultStr));

        testStr = "1234<a href='https://www.o3de.org/'>56</a>78901234<a href='https://www.o3de.org/'>56</a>7890";
        resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(EndsWithOverflowLink(resultStr));
    }

    TEST_F(TextOverflowWidgetTests, ElideText_OverMaxLength_CorrectTruncation)
    {
        QString testStr = "12345678901234567890";
        QString resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(resultStr.startsWith(QString("1234567890 ")));

        testStr = "1234<a href='https://www.o3de.org/'>56</a>78901234567890";
        resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(resultStr.startsWith(QString("1234<a href='https://www.o3de.org/'>56</a>7890 ")));
    }

    TEST_F(TextOverflowWidgetTests, ElideText_OverMaxLengthAtOpeningTag_OpeningTagNotIncluded)
    {
        QString testStr = "1234567890<a href='https://www.o3de.org/'>1</a>";
        QString resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_FALSE(resultStr.startsWith(QString("1234567890<a ")));
    }

    TEST_F(TextOverflowWidgetTests, ElideText_OverMaxLengthAtLinkName_LinkNameTruncatedAndClosingTagIncluded)
    {
        QString testStr = "12345678<a href='https://www.o3de.org/'>901234567890</a>";
        QString resultStr = TextOverflowLabel::ElideLinkedText(testStr, s_testLength);
        EXPECT_TRUE(resultStr.startsWith(QString("12345678<a href='https://www.o3de.org/'>90</a> ")));
    }
} // namespace O3DE::ProjectManager
