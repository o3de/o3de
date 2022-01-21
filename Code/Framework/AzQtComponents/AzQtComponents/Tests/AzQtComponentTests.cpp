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
#include <AzQtComponents/Utilities/Conversions.h>
#include <QLocale>

// Environments subclass from AZ::Test::ITestEnvironment
class AzQtComponentsTestEnvironment : public AZ::Test::ITestEnvironment
{
public:
    AzQtComponentsTestEnvironment()
    {
        // needed for the controller tests - only ever needs to be run once
        AzQtComponents::registerMetaTypes();
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
