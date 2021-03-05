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

    virtual ~AzQtComponentsTestEnvironment() {}

protected:

    void SetupEnvironment() override
    {
    }

    void TeardownEnvironment() override
    {
    }
};

AZ_UNIT_TEST_HOOK(new AzQtComponentsTestEnvironment);
