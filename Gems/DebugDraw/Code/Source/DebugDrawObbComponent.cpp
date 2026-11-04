/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Translation/TranslationDef.h>

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
                ->Field("IsRayTracingEnabled", &DebugDrawObbElement::m_isRayTracingEnabled)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<DebugDrawObbElement>(
                    QT_TRANSLATE_NOOP("DebugDraw", "DebugDraw Sphere Element Settings"),
                    QT_TRANSLATE_NOOP("DebugDraw", "Settings for DebugDraw sphere element."))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Debugging")
                    ->DataElement(0, &DebugDrawObbElement::m_color,
                        QT_TRANSLATE_NOOP("DebugDraw", "Color"),
                        QT_TRANSLATE_NOOP("DebugDraw", "Display color for the line."))
                    ->DataElement(0, &DebugDrawObbElement::m_scale,
                        QT_TRANSLATE_NOOP("DebugDraw", "Scale"),
                        QT_TRANSLATE_NOOP("DebugDraw", "The scale of the box."))
                    ->DataElement(0, &DebugDrawObbElement::m_isRayTracingEnabled,
                        QT_TRANSLATE_NOOP("DebugDraw", "Use ray tracing"),
                        QT_TRANSLATE_NOOP("DebugDraw", "Includes this object in ray tracing calculations."))
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
        provided.push_back(AZ_CRC_CE("DebugDrawObbService"));
    }

    void DebugDrawObbComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
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
