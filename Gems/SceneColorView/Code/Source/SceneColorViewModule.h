/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <AzCore/Module/Module.h>
#include "SceneColorViewSystemComponent.h"
#include "SceneColorViewComponent.h"

namespace SceneColorView
{
    class SceneColorViewModule : public AZ::Module
    {
    public:
        AZ_RTTI(SceneColorViewModule, "{6E694086-8100-46D5-86A0-D5C7D183CE49}", AZ::Module);
        AZ_CLASS_ALLOCATOR(SceneColorViewModule, AZ::SystemAllocator);
        SceneColorViewModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                SceneColorViewSystemComponent::CreateDescriptor(), SceneColorViewComponent::CreateDescriptor()});
        }
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        { return {azrtti_typeid<SceneColorViewSystemComponent>()}; }
    };
}
