/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Physics/NameConstants.h>

#include <Material/PhysXMaterialConfiguration.h>

namespace PhysX
{
    void MaterialConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::MaterialConfiguration>()
                ->Version(1)
                ->Field("DynamicFriction", &MaterialConfiguration::m_dynamicFriction)
                ->Field("StaticFriction", &MaterialConfiguration::m_staticFriction)
                ->Field("Restitution", &MaterialConfiguration::m_restitution)
                ->Field("FrictionCombine", &MaterialConfiguration::m_frictionCombine)
                ->Field("RestitutionCombine", &MaterialConfiguration::m_restitutionCombine)
                ->Field("Density", &MaterialConfiguration::m_density)
                ->Field("DebugColor", &MaterialConfiguration::m_debugColor)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {

                editContext->Class<PhysX::MaterialConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "PhysX Material")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_staticFriction, "Static friction", "Friction coefficient when object is still")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_dynamicFriction, "Dynamic friction", "Friction coefficient when object is moving")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_restitution, "Restitution", "Restitution coefficient")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_frictionCombine, "Friction combine", "How the friction is combined between colliding objects")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration::m_restitutionCombine, "Restitution combine", "How the restitution is combined between colliding objects")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration::m_density, "Density", "Material density")
                        ->Attribute(AZ::Edit::Attributes::Min, MaterialConfiguration::MinDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Max, MaterialConfiguration::MaxDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetDensityUnit())
                    ->DataElement(AZ::Edit::UIHandlers::Color, &MaterialConfiguration::m_debugColor, "Debug Color", "Debug color to use for this material")
                    ;
            }
        }
    }
} // namespace PhysX
