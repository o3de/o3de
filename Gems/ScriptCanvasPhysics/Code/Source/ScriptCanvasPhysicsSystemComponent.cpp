/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ScriptCanvasPhysics_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include "ScriptCanvasPhysicsSystemComponent.h"
#include "PhysicsNodeLibrary.h"

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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Script::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }

        PhysicsNodeLibrary::Reflect(context);
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
        PhysicsNodeLibrary::InitNodeRegistry(ScriptCanvas::GetNodeRegistry().Get());
    }

    void ScriptCanvasPhysicsSystemComponent::Activate()
    {
    }

    void ScriptCanvasPhysicsSystemComponent::Deactivate()
    {
    }
}
