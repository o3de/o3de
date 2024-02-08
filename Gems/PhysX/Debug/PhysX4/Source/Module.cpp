/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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
        AZ_CLASS_ALLOCATOR(PhysXDebugModule, AZ::SystemAllocator);

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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), PhysXDebug::PhysXDebugModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_PhysXDebug, PhysXDebug::PhysXDebugModule)
#endif
