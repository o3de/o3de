/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace MiniAudio
{
    // forwarded here to avoid leaking miniaudio.h into shared files.
    extern AZ::ComponentDescriptor* MiniAudioSystemComponent_CreateDescriptor();
    extern AZ::TypeId MiniAudioSystemComponent_GetTypeId();
    extern AZ::ComponentDescriptor* MiniAudioPlaybackComponent_CreateDescriptor();
    extern AZ::ComponentDescriptor* MiniAudioListenerComponent_CreateDescriptor();

    class MiniAudioModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(MiniAudioModuleInterface, "{290D3CED-B418-46E5-88A4-69EBF7DFC32C}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MiniAudioModuleInterface, AZ::SystemAllocator, 0);

        MiniAudioModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                MiniAudioSystemComponent_CreateDescriptor(),
                MiniAudioPlaybackComponent_CreateDescriptor(),
                MiniAudioListenerComponent_CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                MiniAudioSystemComponent_GetTypeId(),
            };
        }
    };
}// namespace MiniAudio
