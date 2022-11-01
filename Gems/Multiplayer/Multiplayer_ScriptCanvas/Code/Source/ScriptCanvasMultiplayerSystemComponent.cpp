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

#include "ScriptCanvasMultiplayerSystemComponent.h"
#include <AutoGenNodeableRegistry.generated.h>

REGISTER_SCRIPTCANVAS_AUTOGEN_NODEABLE(Multiplayer_ScriptCanvasStatic);

namespace ScriptCanvasMultiplayer
{
    void ScriptCanvasMultiplayerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptCanvasMultiplayerSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ScriptCanvasMultiplayerSystemComponent>("ScriptCanvasMultiplayer", "Provides various Script Canvas nodes for multiplayer.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Script::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }
    }

    void ScriptCanvasMultiplayerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasMultiplayerService"));
    }

    void ScriptCanvasMultiplayerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ScriptCanvasMultiplayerService"));
    }

    void ScriptCanvasMultiplayerSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("LmbrCentralService"));
    }

    void ScriptCanvasMultiplayerSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void ScriptCanvasMultiplayerSystemComponent::Init()
    {
    }

    void ScriptCanvasMultiplayerSystemComponent::Activate()
    {
    }

    void ScriptCanvasMultiplayerSystemComponent::Deactivate()
    {
    }
}
