/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Color.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/QtMetaTypes.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <QLocale>

// Environments subclass from AZ::Test::ITestEnvironment
class AzQtComponentsTestEnvironment : public AZ::Test::ITestEnvironment
{
public:
    AzQtComponentsTestEnvironment()
    {
        // needed for the controller tests - only ever needs to be run once
        qRegisterMetaType<AZ::Color>();
        qRegisterMetaType<AzQtComponents::Palette>();

        // The flattened GUI (stylesheets disabled) is what the style tests cover. StyleManager::stylesheetsDisabled()
        // keeps its first answer for the whole process (StyleManager.cpp:67), so the switch is set here, before any
        // test reads it; legacy rules are tested through Qt's own sheet style, which does not read it.
        qputenv("O3DE_DISABLE_STYLESHEETS", "1");
    }

    ~AzQtComponentsTestEnvironment() override {}

protected:

    void SetupEnvironment() override
    {
    }

    void TeardownEnvironment() override
    {
    }
};

AZ_UNIT_TEST_HOOK(new AzQtComponentsTestEnvironment);

TEST(AzQtComponents, ToStringReturnsTruncatedString)
{
    double testVal = 1.2399999;
    QString result = AzQtComponents::toString(testVal, 3, QLocale(), false, false);
    EXPECT_TRUE(result == "1.239");
}

TEST(AzQtComponents, ToStringReturnsRoundedString)
{
    double testVal = 1.2399999;
    QString result = AzQtComponents::toString(testVal, 3, QLocale(), false, true);
    EXPECT_TRUE(result == "1.24");
}
