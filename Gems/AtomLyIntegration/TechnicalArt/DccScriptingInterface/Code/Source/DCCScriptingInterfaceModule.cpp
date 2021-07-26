/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <DCCScriptingInterfaceSystemComponent.h>

namespace DCCScriptingInterface
{
    class DCCScriptingInterfaceModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(DCCScriptingInterface::DCCScriptingInterfaceModule, "{9A30C8CC-042A-4C5B-8D1F-1ABA5C58337E}", AZ::Module);
        AZ_CLASS_ALLOCATOR(DCCScriptingInterfaceModule, AZ::SystemAllocator, 0);

        DCCScriptingInterfaceModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                DCCScriptingInterfaceSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<DCCScriptingInterfaceSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(DCCScriptingInterface_7bf5a77dacd8438bb4966a66b5a678d8, DCCScriptingInterface::DCCScriptingInterfaceModule)
