/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzQtComponents/Utilities/Conversions.h>
#include <QLocale>

TEST(AzQtComponents, FloatToString_Truncate2Decimals)
{
    QLocale testLocale(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    const int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(0.1234, numDecimalPlaces, testLocale, showThousandsSeparator), "0.12");
}

TEST(AzQtComponents, FloatToString_AllZerosButOne)
{
    QLocale testLocale(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(1.0000, numDecimalPlaces, testLocale, showThousandsSeparator), "1.0");
}

TEST(AzQtComponents, FloatToString_TruncateAllZerosButOne)
{
    QLocale testLocale(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(1.0001, numDecimalPlaces, testLocale, showThousandsSeparator), "1.0");
}

TEST(AzQtComponents, FloatToString_TruncateNotRound)
{
    QLocale testLocale(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    int numDecimalPlaces = 3;
    EXPECT_EQ(AzQtComponents::toString(0.1236, numDecimalPlaces, testLocale, showThousandsSeparator), "0.123");
}

TEST(AzQtComponents, FloatToString_TruncateShowThousandsSeparatorTruncateNoRound)
{
    QLocale testLocale(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = true;
    int numDecimalPlaces = 3;
    EXPECT_EQ(AzQtComponents::toString(1000.1236, numDecimalPlaces, testLocale, showThousandsSeparator), "1,000.123");
}

TEST(AzQtComponents, FloatToString_TruncateShowThousandsSeparatorOnlyOneDecimal)
{
    QLocale testLocale(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = true;
    int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(1000.000, numDecimalPlaces, testLocale, showThousandsSeparator), "1,000.0");
}

TEST(AzQtComponents, FloatToString_Truncate2DecimalsWithLocale)
{
    QLocale testLocale{ QLocale() };

    const bool showThousandsSeparator = false;
    const int numDecimalPlaces = 2;
    QString testString = "0" + QString(testLocale.decimalPoint()) + "12";
    EXPECT_EQ(testString, AzQtComponents::toString(0.1234, numDecimalPlaces, testLocale, showThousandsSeparator));
}

TEST(AzQtComponents, FloatToString_AllZerosButOneWithLocale)
{
    QLocale testLocale{ QLocale() };

    const bool showThousandsSeparator = false;
    const int numDecimalPlaces = 2;
    QString testString = "1" + QString(testLocale.decimalPoint()) + "0";
    EXPECT_EQ(testString, AzQtComponents::toString(1.0000, numDecimalPlaces, testLocale, showThousandsSeparator));
}

TEST(AzQtComponents, FloatToString_TruncateShowThousandsSeparatorTruncateNoRoundWithLocale)
{
    QLocale testLocale{ QLocale() };

    const bool showThousandsSeparator = true;
    const int numDecimalPlaces = 3;
    QString testString = "1" + QString(testLocale.groupSeparator()) + "000" + QString(testLocale.decimalPoint()) + "123";
    EXPECT_EQ(testString, AzQtComponents::toString(1000.1236, numDecimalPlaces, testLocale, showThousandsSeparator));
}

TEST(AzQtComponents, FloatToString_TruncateShowThousandsSeparatorOnlyOneDecimalWithLocale)
{
    QLocale testLocale{ QLocale() };

    const bool showThousandsSeparator = true;
    int numDecimalPlaces = 2;
    QString testString = "1" + QString(testLocale.groupSeparator()) + "000" + QString(testLocale.decimalPoint()) + "0";
    EXPECT_EQ(testString, AzQtComponents::toString(1000.000, numDecimalPlaces, testLocale, showThousandsSeparator));
}
