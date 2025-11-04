/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "DebugDrawLineComponent.h"

namespace DebugDraw
{
    void DebugDrawLineElement::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DebugDrawLineElement>()
                ->Version(0)
                ->Field("StartEntityId", &DebugDrawLineElement::m_startEntityId)
                ->Field("EndEntityId", &DebugDrawLineElement::m_endEntityId)
                ->Field("StartWorldLocation", &DebugDrawLineElement::m_startWorldLocation)
                ->Field("EndWorldLocation", &DebugDrawLineElement::m_endWorldLocation)
                ->Field("Color", &DebugDrawLineElement::m_color)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DebugDrawLineElement>("DebugDraw Line element settings", "Settings for DebugDraw line element.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &DebugDrawLineElement::m_endEntityId, "End Entity", "Which entity the line is drawn to (starts on this entity).")
                    ->DataElement(0, &DebugDrawLineElement::m_color, "Color", "Display color for the line.")
                ;
            }
        }
    }

    void DebugDrawLineComponent::Reflect(AZ::ReflectContext* context)
    {
        DebugDrawLineElement::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawLineComponent, AZ::Component>()
                ->Version(0)
                ->Field("Element", &DebugDrawLineComponent::m_element)
            ;
        }
    }

    DebugDrawLineComponent::DebugDrawLineComponent(const DebugDrawLineElement& element)
        : m_element(element)
    {
        m_element.m_owningEditorComponent = AZ::InvalidComponentId;
    }

    void DebugDrawLineComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DebugDrawLineService"));
    }

    void DebugDrawLineComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void DebugDrawLineComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DebugDrawLineComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawLineComponent::Init()
    {
    }

    void DebugDrawLineComponent::Activate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
    }

    void DebugDrawLineComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }
} // namespace DebugDraw
