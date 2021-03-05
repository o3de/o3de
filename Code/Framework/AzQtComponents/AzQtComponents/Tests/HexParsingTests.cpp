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

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>

static const qreal g_colorChannelToleranceHexParsing = 0.0000005;
static const qreal g_unchangedAlpha = 0.2;

TEST(AzQtComponents, ParsingWhiteHexValueNoAlpha)
{
    using namespace AzQtComponents;

    const bool parseAlpha = false;
    ColorHexEdit::ParsedColor test = ColorHexEdit::convertTextToColorValues("FFFFFF", parseAlpha, g_unchangedAlpha);
    EXPECT_NEAR(test.red, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.green, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.blue, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.alpha, g_unchangedAlpha, g_colorChannelToleranceHexParsing);
}


TEST(AzQtComponents, ParsingHexValueNoAlpha)
{
    using namespace AzQtComponents;

    const bool parseAlpha = false;
    ColorHexEdit::ParsedColor test = ColorHexEdit::convertTextToColorValues("FF00FF", parseAlpha, g_unchangedAlpha);
    EXPECT_NEAR(test.red, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.green, 0.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.blue, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.alpha, g_unchangedAlpha, g_colorChannelToleranceHexParsing);
}

TEST(AzQtComponents, ParsingASlightlyDifferentHexValueWithNoAlphaButNonFullOnChannels)
{
    using namespace AzQtComponents;

    const bool parseAlpha = false;
    ColorHexEdit::ParsedColor test = ColorHexEdit::convertTextToColorValues("0F0F0F", parseAlpha, g_unchangedAlpha);
    EXPECT_NEAR(test.red, (static_cast<float>(0xf) / 255.0), g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.green, (static_cast<float>(0xf) / 255.0), g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.blue, (static_cast<float>(0xf) / 255.0), g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.alpha, g_unchangedAlpha, g_colorChannelToleranceHexParsing);
}

TEST(AzQtComponents, ParsingHexValueIncludingAlphaValueWithoutSpecifyingAnAlphaValueAndConfirmingItsNotChangedWhenParsed)
{
    using namespace AzQtComponents;

    const bool parseAlpha = true;
    ColorHexEdit::ParsedColor test = ColorHexEdit::convertTextToColorValues("FF00FF", parseAlpha, g_unchangedAlpha);
    EXPECT_NEAR(test.alpha, g_unchangedAlpha, g_colorChannelToleranceHexParsing);
}

TEST(AzQtComponents, ParsingHexValueWithOneHexDigitOfAlpha)
{
    using namespace AzQtComponents;

    const bool parseAlpha = true;
    ColorHexEdit::ParsedColor test = ColorHexEdit::convertTextToColorValues("FF00FFF", parseAlpha, g_unchangedAlpha);
    EXPECT_NEAR(test.red, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.green, 0.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.blue, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.alpha, (static_cast<float>(0xf) / 255.0), g_colorChannelToleranceHexParsing);
}

TEST(AzQtComponents, ParsingHexValueWithTwoHexDigitsOfAlpha)
{
    using namespace AzQtComponents;

    const bool parseAlpha = true;
    ColorHexEdit::ParsedColor test = ColorHexEdit::convertTextToColorValues("FF00FFFF", parseAlpha, g_unchangedAlpha);
    EXPECT_NEAR(test.red, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.green, 0.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.blue, 1.0, g_colorChannelToleranceHexParsing);
    EXPECT_NEAR(test.alpha, 1.0, g_colorChannelToleranceHexParsing);
}
