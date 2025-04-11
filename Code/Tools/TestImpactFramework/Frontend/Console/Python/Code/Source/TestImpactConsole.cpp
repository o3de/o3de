/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConsoleMain.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>

int main(int argc, char** argv)
{
    TestImpact::Console::ReturnCode returnCode = TestImpact::Console::Main(argc, argv);
    return aznumeric_cast<int>(returnCode);
}
