/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzQtComponents/Utilities/Conversions.h>
#include <QLocale>

TEST(AzQtComponents, FloatToString_Truncate2Decimals)
{
    QLocale testLocal(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    const int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(0.1234, numDecimalPlaces, testLocal, showThousandsSeparator), "0.12");
}

TEST(AzQtComponents, FloatToString_AllZerosButOne)
{
    QLocale testLocal(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(1.0000, numDecimalPlaces, testLocal, showThousandsSeparator), "1.0");
}

TEST(AzQtComponents, FloatToString_TruncateAllZerosButOne)
{
    QLocale testLocal(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(1.0001, numDecimalPlaces, testLocal, showThousandsSeparator), "1.0");
}

TEST(AzQtComponents, FloatToString_TruncateNotRound)
{
    QLocale testLocal(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = false;
    int numDecimalPlaces = 3;
    EXPECT_EQ(AzQtComponents::toString(0.1236, numDecimalPlaces, testLocal, showThousandsSeparator), "0.123");
}

TEST(AzQtComponents, FloatToString_TruncateShowThousandsSeparatorTruncateNoRound)
{
    QLocale testLocal(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = true;
    int numDecimalPlaces = 3;
    EXPECT_EQ(AzQtComponents::toString(1000.1236, numDecimalPlaces, testLocal, showThousandsSeparator), "1,000.123");
}

TEST(AzQtComponents, FloatToString_TruncateShowThousandsSeparatorOnlyOneDecimal)
{
    QLocale testLocal(QLocale::English, QLocale::UnitedStates);

    const bool showThousandsSeparator = true;
    int numDecimalPlaces = 2;
    EXPECT_EQ(AzQtComponents::toString(1000.000, numDecimalPlaces, testLocal, showThousandsSeparator), "1,000.0");
}
