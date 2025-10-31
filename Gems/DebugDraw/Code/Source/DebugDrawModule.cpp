/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "DebugDrawSystemComponent.h"
#include "DebugDrawLineComponent.h"
#include "DebugDrawRayComponent.h"
#include "DebugDrawSphereComponent.h"
#include "DebugDrawObbComponent.h"
#include "DebugDrawTextComponent.h"

#ifdef DEBUGDRAW_GEM_EDITOR
#include "EditorDebugDrawLineComponent.h"
#include "EditorDebugDrawRayComponent.h"
#include "EditorDebugDrawSphereComponent.h"
#include "EditorDebugDrawObbComponent.h"
#include "EditorDebugDrawTextComponent.h"
#endif // DEBUGDRAW_GEM_EDITOR

#include <IGem.h>

namespace DebugDraw
{
    class DebugDrawModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(DebugDrawModule, "{07AC9E51-535C-402D-A2EB-529366ED9985}", CryHooksModule);

        DebugDrawModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {

                //DebugDrawSystemComponent::CreateDescriptor(),
                DebugDrawLineComponent::CreateDescriptor(),
                DebugDrawRayComponent::CreateDescriptor(),
                DebugDrawSphereComponent::CreateDescriptor(),
                DebugDrawObbComponent::CreateDescriptor(),
                DebugDrawTextComponent::CreateDescriptor(),
                DebugDrawSystemComponent::CreateDescriptor(),

                #ifdef DEBUGDRAW_GEM_EDITOR
                EditorDebugDrawLineComponent::CreateDescriptor(),
                EditorDebugDrawRayComponent::CreateDescriptor(),
                EditorDebugDrawSphereComponent::CreateDescriptor(),
                EditorDebugDrawObbComponent::CreateDescriptor(),
                EditorDebugDrawTextComponent::CreateDescriptor(),
                #endif // DEBUGDRAW_GEM_EDITOR
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<DebugDrawSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), DebugDraw::DebugDrawModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_DebugDraw, DebugDraw::DebugDrawModule)
#endif
