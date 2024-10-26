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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AWSMetrics::AWSMetricsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AWSMetrics, AWSMetrics::AWSMetricsModule)
#endif
