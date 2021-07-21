/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PhysXDebug_precompiled.h"
#include <AzCore/Memory/SystemAllocator.h>
#include "SystemComponent.h"

#ifdef PHYSXDEBUG_GEM_EDITOR
#include "EditorSystemComponent.h"
#endif
#include <IGem.h>

namespace PhysXDebug
{
    class PhysXDebugModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(PhysXDebugModule, "{7C9CB91D-D7D7-4362-9FE8-E4D61B6A5113}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(PhysXDebugModule, AZ::SystemAllocator, 0);

        PhysXDebugModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                SystemComponent::CreateDescriptor(),
#ifdef PHYSXDEBUG_GEM_EDITOR
                EditorSystemComponent::CreateDescriptor()
#endif
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<SystemComponent>(),
#ifdef PHYSXDEBUG_GEM_EDITOR
                azrtti_typeid<EditorSystemComponent>(),
#endif
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_PhysXDebug, PhysXDebug::PhysXDebugModule)
