/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "DebugDrawSphereComponent.h"

namespace DebugDraw
{
    void DebugDrawSphereElement::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DebugDrawSphereElement>()
                ->Version(0)
                ->Field("TargetEntityId", &DebugDrawSphereElement::m_targetEntityId)
                ->Field("Color", &DebugDrawSphereElement::m_color)
                ->Field("WorldLocation", &DebugDrawSphereElement::m_worldLocation)
                ->Field("Radius", &DebugDrawSphereElement::m_radius)
                ->Field("IsRayTracingEnabled", &DebugDrawSphereElement::m_isRayTracingEnabled)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DebugDrawSphereElement>("DebugDraw Sphere Element Settings", "Settings for DebugDraw sphere element.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &DebugDrawSphereElement::m_color,  "Color",    "Display color for the line.")
                    ->DataElement(0, &DebugDrawSphereElement::m_radius, "Radius",   "The size of the sphere.")
                    ->DataElement(0, &DebugDrawSphereElement::m_isRayTracingEnabled, "Use ray tracing", "Includes this object in ray tracing calculations.")
                    ;
            }
        }
    }

    void DebugDrawSphereComponent::Reflect(AZ::ReflectContext* context)
    {
        DebugDrawSphereElement::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawSphereComponent, AZ::Component>()
                ->Version(0)
                ->Field("Element", &DebugDrawSphereComponent::m_element)
                ;
        }
    }

    DebugDrawSphereComponent::DebugDrawSphereComponent(const DebugDrawSphereElement& element)
        : m_element(element)
    {
        m_element.m_owningEditorComponent = AZ::InvalidComponentId;
    }

    void DebugDrawSphereComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DebugDrawSphereService"));
    }

    void DebugDrawSphereComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void DebugDrawSphereComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DebugDrawSphereComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawSphereComponent::Init()
    {
    }

    void DebugDrawSphereComponent::Activate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
    }

    void DebugDrawSphereComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

} // namespace DebugDraw
