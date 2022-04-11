/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>

class AzNetworkingTestEnvironment : public AZ::Test::GemTestEnvironment
{
    void AddGemsAndComponents() override;
};

void AzNetworkingTestEnvironment::AddGemsAndComponents()
{
    AddDynamicModulePaths({ "AzNetworking" });
}

AZ_UNIT_TEST_HOOK(new AzNetworkingTestEnvironment);

