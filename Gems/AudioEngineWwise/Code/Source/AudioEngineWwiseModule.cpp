/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <IGem.h>

#include "AudioEngineWwiseGemSystemComponent.h"

#if defined(AUDIO_ENGINE_WWISE_BUILDER)
    #include <AudioControlBuilderComponent.h>
    #include <WwiseBuilderComponent.h>
#endif // AUDIO_ENGINE_WWISE_BUILDER

namespace AudioEngineWwiseGem
{
    class AudioEngineWwiseModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(AudioEngineWwiseModule, "{303B0192-D866-4378-9342-728AA6E66F74}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(AudioEngineWwiseModule, AZ::SystemAllocator);

        AudioEngineWwiseModule()
            : CryHooksModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
            #if defined(AUDIO_ENGINE_WWISE_BUILDER)
                AudioControlBuilder::BuilderPluginComponent::CreateDescriptor(),
                WwiseBuilder::BuilderPluginComponent::CreateDescriptor(),
            #else
                AudioEngineWwiseGemSystemComponent::CreateDescriptor(),
            #endif // AUDIO_ENGINE_WWISE_BUILDER
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
            #if !defined(AUDIO_ENGINE_WWISE_BUILDER)
                azrtti_typeid<AudioEngineWwiseGemSystemComponent>(),
            #endif // !AUDIO_ENGINE_WWISE_BUILDER
            };
        }
    };

} // namespace AudioEngineWwiseGem

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AudioEngineWwiseGem::AudioEngineWwiseModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AudioEngineWwise, AudioEngineWwiseGem::AudioEngineWwiseModule)
#endif
