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

#include <AzCore/PlatformDef.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <IGem.h>

#include <AudioSystemGemSystemComponent.h>

namespace AudioSystemGem
{
    class AudioSystemModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(AudioSystemModule, "{BE8CD7ED-AEB9-4617-B069-D848EA986ED3}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(AudioSystemModule, AZ::SystemAllocator, 0);

        AudioSystemModule()
            : CryHooksModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                AudioSystemGemSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AudioSystemGemSystemComponent>(),
            };
        }
    };

} // namespace AudioSystemGem

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AudioSystem, AudioSystemGem::AudioSystemModule)
