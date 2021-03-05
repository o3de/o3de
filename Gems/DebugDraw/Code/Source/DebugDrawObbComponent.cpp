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

#include "DebugDraw_precompiled.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "DebugDrawObbComponent.h"

namespace DebugDraw
{
    void DebugDrawObbElement::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DebugDrawObbElement>()
                ->Version(0)
                ->Field("TargetEntityId", &DebugDrawObbElement::m_targetEntityId)
                ->Field("Color", &DebugDrawObbElement::m_color)
                ->Field("WorldLocation", &DebugDrawObbElement::m_worldLocation)
                ->Field("Scale", &DebugDrawObbElement::m_scale)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DebugDrawObbElement>("DebugDraw Sphere Element Settings", "Settings for DebugDraw sphere element.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &DebugDrawObbElement::m_color, "Color", "Display color for the line.")
                    ->DataElement(0, &DebugDrawObbElement::m_scale, "Scale", "The scale of the box.")
                    ;
            }
        }
    }

    void DebugDrawObbComponent::Reflect(AZ::ReflectContext* context)
    {
        DebugDrawObbElement::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawObbComponent, AZ::Component>()
                ->Version(0)
                ->Field("Element", &DebugDrawObbComponent::m_element)
                ;
        }
    }

    DebugDrawObbComponent::DebugDrawObbComponent(const DebugDrawObbElement& element)
        : m_element(element)
    {
        m_element.m_owningEditorComponent = AZ::InvalidComponentId;
    }

    void DebugDrawObbComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("DebugDrawObbService", 0x1775ae66));
    }

    void DebugDrawObbComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void DebugDrawObbComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DebugDrawObbComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawObbComponent::Init()
    {
    }

    void DebugDrawObbComponent::Activate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
    }

    void DebugDrawObbComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

} // namespace DebugDraw
