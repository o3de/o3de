/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MicrophoneSystemComponent.h"

#include <IGem.h>

namespace Audio
{
    class MicrophoneModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(MicrophoneModule, "{99939704-566A-42B1-8A7A-567E01C63D9C}", CryHooksModule);

        MicrophoneModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), { MicrophoneSystemComponent::CreateDescriptor() });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<MicrophoneSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Audio::MicrophoneModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Microphone, Audio::MicrophoneModule)
#endif
