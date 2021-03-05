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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include "StatisticalProfilerProxySystemComponent.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    namespace Statistics
    {
        StatisticalProfilerProxy* StatisticalProfilerProxy::TimedScope::m_profilerProxy = nullptr;

        ////////////////////////////////////////////////////////////////////////////////////////////////
        void StatisticalProfilerProxySystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<StatisticalProfilerProxySystemComponent, AZ::Component>()
                    ->Version(1);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        void StatisticalProfilerProxySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("StatisticalProfilerService", 0x20066f73));
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        void StatisticalProfilerProxySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("StatisticalProfilerService", 0x20066f73));
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        StatisticalProfilerProxySystemComponent::StatisticalProfilerProxySystemComponent()
            : m_StatisticalProfilerProxy(nullptr)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        StatisticalProfilerProxySystemComponent::~StatisticalProfilerProxySystemComponent()
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        void StatisticalProfilerProxySystemComponent::Activate()
        {
            m_StatisticalProfilerProxy = new StatisticalProfilerProxy;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        void StatisticalProfilerProxySystemComponent::Deactivate()
        {
            delete m_StatisticalProfilerProxy;
        }
    } //namespace Statistics
} // namespace AZ
