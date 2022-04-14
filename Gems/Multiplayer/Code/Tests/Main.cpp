/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/TargetManagement/TargetManagementComponent.h>
#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>

class MultiplayerTestEnvironment : public AZ::Test::GemTestEnvironment
{
    void AddGemsAndComponents() override;
};

void MultiplayerTestEnvironment::AddGemsAndComponents()
{
    // Forcibly inject TargetManagement's descriptor since AzNetworking presently requires it
    AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors;
    componentDescriptors.push_back(AzFramework::TargetManagementComponent::CreateDescriptor());
    AddComponentDescriptors(componentDescriptors);
    AddDynamicModulePaths({ "AzNetworking" });
}

AZ_UNIT_TEST_HOOK_DYNAMIC(new MultiplayerTestEnvironment);

