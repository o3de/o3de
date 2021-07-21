/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "HttpRequestorSystemComponent.h"
#include <AzCore/Module/Module.h>

namespace HttpRequestor
{
    class HttpRequestorModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(HttpRequestorModule, "{FD411E40-AF83-4F6B-A5A3-F59AB71150BF}", AZ::Module);

        HttpRequestorModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                HttpRequestorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<HttpRequestorSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_HttpRequestor, HttpRequestor::HttpRequestorModule)
