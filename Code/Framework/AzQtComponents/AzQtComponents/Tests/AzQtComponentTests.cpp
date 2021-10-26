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
