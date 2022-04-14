/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>
#include <AzFramework/TargetManagement/TargetManagementComponent.h>

class AzNetworkingTestEnvironment : public AZ::Test::GemTestEnvironment
{
    void AddGemsAndComponents() override;
};

void AzNetworkingTestEnvironment::AddGemsAndComponents()
{
    // Forcibly inject TargetManagement's descriptor since AzNetworking presently requires it
    AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors;
    componentDescriptors.push_back(AzFramework::TargetManagementComponent::CreateDescriptor());
    AddComponentDescriptors(componentDescriptors);
    AddDynamicModulePaths({ "AzNetworking" });
}

AZ_UNIT_TEST_HOOK_DYNAMIC(new AzNetworkingTestEnvironment);

