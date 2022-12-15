/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSMetricsModule.h>

#include <AWSMetricsSystemComponent.h>

#if defined(AWS_METRICS_EDITOR)
#include <AWSMetricsEditorSystemComponent.h>
#endif

#include <AzCore/Module/Module.h>

namespace AWSMetrics
{
    AWSMetricsModule::AWSMetricsModule()
        : AZ::Module()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        m_descriptors.insert(m_descriptors.end(),
            {
#if defined(AWS_METRICS_EDITOR)
                AWSMetricsEditorSystemComponent::CreateDescriptor()
#else
                AWSMetricsSystemComponent::CreateDescriptor()
#endif
            }
        );
    }

    AZ::ComponentTypeList AWSMetricsModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList
        {
#if defined(AWS_METRICS_EDITOR)
            azrtti_typeid<AWSMetricsEditorSystemComponent>()
#else
            azrtti_typeid<AWSMetricsSystemComponent>()
#endif
        };
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AWSMetrics, AWSMetrics::AWSMetricsModule)
