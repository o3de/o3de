/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "MultiplayerCompressionSystemComponent.h"

namespace MultiplayerCompression
{
    class MultiplayerCompressionModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(MultiplayerCompressionModule, "{939AFA0D-CFBC-4910-88F3-A0CF429307E4}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MultiplayerCompressionModule, AZ::SystemAllocator, 0);

        MultiplayerCompressionModule()
            : AZ::Module()
        {
            // Push results of MultiplayerCompressionSystemComponent::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                MultiplayerCompressionSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MultiplayerCompressionSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(MultiplayerCompression_1d353c8ca3c74ed193fd6c6783ae41cc, MultiplayerCompression::MultiplayerCompressionModule)
