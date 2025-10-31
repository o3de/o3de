/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "DebugDrawRayComponent.h"

namespace DebugDraw
{
    void DebugDrawRayElement::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DebugDrawRayElement>()
                ->Version(0)
                ->Field("StartEntityId", &DebugDrawRayElement::m_startEntityId)
                ->Field("EndEntityId", &DebugDrawRayElement::m_endEntityId)
                ->Field("WorldLocation", &DebugDrawRayElement::m_worldLocation)
                ->Field("WorldDirection", &DebugDrawRayElement::m_worldDirection)
                ->Field("Color", &DebugDrawRayElement::m_color)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DebugDrawRayElement>("DebugDraw Ray element settings", "Settings for DebugDraw ray element.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &DebugDrawRayElement::m_endEntityId, "End Entity", "Which entity the ray is drawn to (starts on this entity).")
                    ->DataElement(0, &DebugDrawRayElement::m_color, "Color", "Display color for the line.")
                ;
            }
        }
    }

    void DebugDrawRayComponent::Reflect(AZ::ReflectContext* context)
    {
        DebugDrawRayElement::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawRayComponent, AZ::Component>()
                ->Version(0)
                ->Field("Element", &DebugDrawRayComponent::m_element)
            ;
        }
    }

    DebugDrawRayComponent::DebugDrawRayComponent(const DebugDrawRayElement& element)
        : m_element(element)
    {
        m_element.m_owningEditorComponent = AZ::InvalidComponentId;
    }

    void DebugDrawRayComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DebugDrawRayService"));
    }

    void DebugDrawRayComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void DebugDrawRayComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DebugDrawRayComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawRayComponent::Init()
    {
    }

    void DebugDrawRayComponent::Activate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
    }

    void DebugDrawRayComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }
} // namespace DebugDraw
