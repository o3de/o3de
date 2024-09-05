/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MiniAudioModuleInterface.h>

namespace MiniAudio
{
    extern AZ::TypeId MiniAudioEditorSystemComponent_GetTypeId();
    extern AZ::ComponentDescriptor* MiniAudioEditorSystemComponent_CreateDescriptor();
    extern AZ::ComponentDescriptor* EditorMiniAudioListenerComponent_CreateDescriptor();
    extern AZ::ComponentDescriptor* EditorMiniAudioPlaybackComponent_CreateDescriptor();

    class MiniAudioEditorModule : public MiniAudioModuleInterface
    {
    public:
        AZ_RTTI(MiniAudioEditorModule, "{501C94A1-993A-4203-9720-D43D6C1DDB7A}", MiniAudioModuleInterface);
        AZ_CLASS_ALLOCATOR(MiniAudioEditorModule, AZ::SystemAllocator, 0);

        MiniAudioEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and
            // EditContext. This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    MiniAudioEditorSystemComponent_CreateDescriptor(),
                    EditorMiniAudioListenerComponent_CreateDescriptor(),
                    EditorMiniAudioPlaybackComponent_CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                MiniAudioEditorSystemComponent_GetTypeId(),
            };
        }
    };
} // namespace MiniAudio

AZ_DECLARE_MODULE_CLASS(Gem_MiniAudio, MiniAudio::MiniAudioEditorModule)
