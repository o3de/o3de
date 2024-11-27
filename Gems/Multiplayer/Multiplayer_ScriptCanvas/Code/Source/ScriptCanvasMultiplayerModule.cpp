/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasMultiplayerSystemComponent.h"
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace ScriptCanvasMultiplayer
{
    class ScriptCanvasMultiplayerModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptCanvasMultiplayerModule, "{6cf9ae34-9d26-45cf-b48e-ffc7d8f0c56e}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasMultiplayerModule, AZ::SystemAllocator);

        ScriptCanvasMultiplayerModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ScriptCanvasMultiplayerSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ScriptCanvasMultiplayerSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ScriptCanvasMultiplayer::ScriptCanvasMultiplayerModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Multiplayer_ScriptCanvas, ScriptCanvasMultiplayer::ScriptCanvasMultiplayerModule)
#endif
