/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "ScriptCanvasPhysicsSystemComponent.h"
#include <AutoGenFunctionRegistry.generated.h>

REGISTER_SCRIPTCANVAS_AUTOGEN_FUNCTION(ScriptCanvasPhysicsStatic);

namespace ScriptCanvasPhysics
{
    void ScriptCanvasPhysicsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptCanvasPhysicsSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ScriptCanvasPhysicsSystemComponent>("ScriptCanvasPhysics", "Exposes legacy physics features to scripting through the Behavior Context to Lua and Script Canvas.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Script::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void ScriptCanvasPhysicsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasPhysicsService", 0x4686eefd));
    }

    void ScriptCanvasPhysicsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ScriptCanvasPhysicsService", 0x4686eefd));
    }

    void ScriptCanvasPhysicsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("LmbrCentralService", 0xc3a02410));
    }

    void ScriptCanvasPhysicsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ScriptCanvasPhysicsSystemComponent::Init()
    {
    }

    void ScriptCanvasPhysicsSystemComponent::Activate()
    {
    }

    void ScriptCanvasPhysicsSystemComponent::Deactivate()
    {
    }
}
