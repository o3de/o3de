/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRenderOptionsSystemComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::Render
{
    void AtomRenderOptionsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomRenderOptionsSystemComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomRenderOptionsSystemComponent>(
                      "Atom Render Options", "Allow to toggle and edit render passes inside the Editor viewport")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->Attribute(Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void AtomRenderOptionsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomRenderOptionsService"));
    }

    void AtomRenderOptionsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomRenderOptionsService"));
    }

    void AtomRenderOptionsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void AtomRenderOptionsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomRenderOptionsSystemComponent::Activate()
    {
        if (!m_actionHandler)
        {
            m_actionHandler = AZStd::make_unique<AtomRenderOptionsActionHandler>();
        }
        m_actionHandler->Activate();
    }

    void AtomRenderOptionsSystemComponent::Deactivate()
    {
        m_actionHandler->Deactivate();
    }
} // namespace AZ::Render
