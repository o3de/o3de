/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasPhysics_precompiled.h"

#include <AzCore/Memory/SystemAllocator.h>

#include "ScriptCanvasPhysicsSystemComponent.h"
#include "PhysicsNodeLibrary.h"

#include <AzCore/Module/Module.h>

namespace ScriptCanvasPhysics
{
    class ScriptCanvasPhysicsModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptCanvasPhysicsModule, "{6B4D5464-DAA5-439D-A0D9-22311608C610}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasPhysicsModule, AZ::SystemAllocator, 0);

        ScriptCanvasPhysicsModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                    ScriptCanvasPhysicsSystemComponent::CreateDescriptor(),
                });

            AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(PhysicsNodeLibrary::GetComponentDescriptors());
            m_descriptors.insert(m_descriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                       azrtti_typeid<ScriptCanvasPhysicsSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_ScriptCanvasPhysics, ScriptCanvasPhysics::ScriptCanvasPhysicsModule)
