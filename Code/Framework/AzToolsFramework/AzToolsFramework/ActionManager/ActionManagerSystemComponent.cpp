/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/Editor/ActionManagerUtils.h>

namespace AzToolsFramework
{
    void ActionManagerSystemComponent::Init()
    {
        if (IsNewActionManagerEnabled())
        {
            m_actionManager = AZStd::make_unique<ActionManager>();
        }
    }

    void ActionManagerSystemComponent::Activate()
    {
    }

    void ActionManagerSystemComponent::Deactivate()
    {
    }

    void ActionManagerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ActionManagerSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void ActionManagerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ActionManager"));
    }

    void ActionManagerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ActionManagerSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

} // namespace AzToolsFramework
