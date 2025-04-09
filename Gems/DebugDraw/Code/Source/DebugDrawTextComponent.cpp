/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include "DebugDrawTextComponent.h"

namespace DebugDraw
{
    void DebugDrawTextElement::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DebugDrawTextElement>()
                ->Version(1)
                ->Field("Text", &DebugDrawTextElement::m_text)
                ->Field("Color", &DebugDrawTextElement::m_color)
                ->Field("DrawMode", &DebugDrawTextElement::m_drawMode)
                ->Field("WorldLocation", &DebugDrawTextElement::m_worldLocation)
                ->Field("TargetEntity", &DebugDrawTextElement::m_targetEntityId)
                ->Field("Centered", &DebugDrawTextElement::m_centered)
                ->Field("Size", &DebugDrawTextElement::m_size)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DebugDrawTextElement>("DebugDraw Text element settings", "Settings for DebugDraw text element.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &DebugDrawTextElement::m_text, "Text", "The Debug Text.")
                    ->DataElement(0, &DebugDrawTextElement::m_color, "Color", "Text Color.")
                    ->DataElement(0, &DebugDrawTextElement::m_size, "Size", "Text size.")
                    ->DataElement(0, &DebugDrawTextElement::m_drawMode, "Draw Mode", "Draw Mode Preference.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &DebugDrawTextElement::m_drawMode, "Draw Mode", "Draw Mode Preference.")
                    ->EnumAttribute(DrawMode::OnScreen, "Screen Space")
                    ->EnumAttribute(DrawMode::InWorld, "World Space")
                    ->DataElement(0, &DebugDrawTextElement::m_centered, "Centered", "Center align the text if enabled, otherwise left align.")
                    // Currently supports World Space placement on component owners location Or exact placement via behavior context / scripting (TBC)
                    //->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    //->DataElement(AZ::Edit::UIHandlers::Default, &DebugDrawTextElement::m_targetEntityId, "TargetEntity", "The target entity to position debug text at.")
                    //->Attribute(AZ::Edit::Attributes::Visibility, &DebugDrawTextElement::isWorldSpace)
                    ;
            }
        }
    }


    void DebugDrawTextComponent::Reflect(AZ::ReflectContext* context)
    {
        DebugDrawTextElement::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<DebugDrawTextComponent, AZ::Component>()
                ->Version(0)
                ->Field("TextElement", &DebugDrawTextComponent::m_element)
                ;
        }
    }

    DebugDrawTextComponent::DebugDrawTextComponent(const DebugDrawTextElement& textElement)
        : m_element(textElement)
    {
        m_element.m_owningEditorComponent = AZ::InvalidComponentId;
    }

    void DebugDrawTextComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DebugDrawTextService"));
    }

    void DebugDrawTextComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void DebugDrawTextComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void DebugDrawTextComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void DebugDrawTextComponent::Init()
    {
    }

    void DebugDrawTextComponent::Activate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::RegisterDebugDrawComponent, this);
    }

    void DebugDrawTextComponent::Deactivate()
    {
        DebugDrawInternalRequestBus::Broadcast(&DebugDrawInternalRequestBus::Events::UnregisterDebugDrawComponent, this);
    }

} // namespace DebugDraw
