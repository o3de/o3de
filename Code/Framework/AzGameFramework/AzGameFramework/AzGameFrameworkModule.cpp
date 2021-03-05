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
#include <AzGameFramework/AzGameFrameworkModule.h>

// Component includes
#include <AzFramework/Driller/RemoteDrillerInterface.h>
#include <AzFramework/Driller/DrillToFileComponent.h>

namespace AzGameFramework
{
    AzGameFrameworkModule::AzGameFrameworkModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzFramework::DrillToFileComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList AzGameFrameworkModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<AzFramework::DrillToFileComponent>(),
        };
    }
}
