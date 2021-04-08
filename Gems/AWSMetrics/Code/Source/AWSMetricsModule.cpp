/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include <AWSMetricsModule.h>

#include <AWSMetricsSystemComponent.h>
#include <AzCore/Module/Module.h>

namespace AWSMetrics
{
    AWSMetricsModule::AWSMetricsModule()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(), {AWSMetricsSystemComponent::CreateDescriptor()});
    }

    AZ::ComponentTypeList AWSMetricsModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{azrtti_typeid<AWSMetricsSystemComponent>()};
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(AWSMetrics_cc6fc7a18fc047039a369a26100fcbbe, AWSMetrics::AWSMetricsModule)
