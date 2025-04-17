
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include "ScriptCanvasTestingSystemComponent.h"

namespace ScriptCanvasTesting
{
    class ScriptCanvasTestingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptCanvasTestingModule, "{AF32BC51-C4E5-48C4-B5E4-D7877C303D43}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasTestingModule, AZ::SystemAllocator);

        ScriptCanvasTestingModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                ScriptCanvasTestingSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ScriptCanvasTestingSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ScriptCanvasTesting::ScriptCanvasTestingModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ScriptCanvasTesting, ScriptCanvasTesting::ScriptCanvasTestingModule)
#endif
