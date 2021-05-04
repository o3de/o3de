/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include <AzCore/Module/Module.h>
#include <AWSCoreEditorSystemComponent.h>

namespace AWSCore
{
    class AWSCoreResourceMappingToolModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSCoreResourceMappingToolModule, "{9D16F400-009C-47FF-A186-E48BEB73D94D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSCoreResourceMappingToolModule, AZ::SystemAllocator, 0);

        AWSCoreResourceMappingToolModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    AWSCoreEditorSystemComponent::CreateDescriptor(),
                });
        }

        ~AWSCoreResourceMappingToolModule() override = default;

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{azrtti_typeid<AWSCoreEditorSystemComponent>()};
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AWSCore_ResourceMappingTool, AWSCore::AWSCoreResourceMappingToolModule)
