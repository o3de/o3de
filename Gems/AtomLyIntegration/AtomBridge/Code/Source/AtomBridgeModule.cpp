/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AtomBridgeModule.h>

namespace AZ
{
    namespace AtomBridge
    {
        Module::Module()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                    FlyCameraInputComponent::CreateDescriptor(),
                    AtomBridgeSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList Module::GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AtomBridgeSystemComponent>(),
            };
        }
    }
} // namespace AZ

#ifndef EDITOR
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_AtomBridge, AZ::AtomBridge::Module)
#endif
