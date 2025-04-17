/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <StreamerProfilerSystemComponent.h>

namespace Streamer
{
    class StreamerProfilerModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(StreamerProfilerModuleInterface, "{27fc3f53-8e85-43be-b121-3fef086f8f22}", AZ::Module);
        AZ_CLASS_ALLOCATOR(StreamerProfilerModuleInterface, AZ::SystemAllocator);

        StreamerProfilerModuleInterface()
        {
            m_descriptors.insert(m_descriptors.end(), {
                StreamerProfilerSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<StreamerProfilerSystemComponent>(),
            };
        }
    };
}// namespace Streamer
