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
#include <AzFramework/Physics/Material/PhysicsMaterialConfiguration.h>

namespace Physics
{
    void MaterialConfiguration2::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Physics::MaterialConfiguration2>()
                ->Version(1)
                ->Field("DynamicFriction", &MaterialConfiguration2::m_dynamicFriction)
                ->Field("StaticFriction", &MaterialConfiguration2::m_staticFriction)
                ->Field("Restitution", &MaterialConfiguration2::m_restitution)
                ->Field("FrictionCombine", &MaterialConfiguration2::m_frictionCombine)
                ->Field("RestitutionCombine", &MaterialConfiguration2::m_restitutionCombine)
                ->Field("Density", &MaterialConfiguration2::m_density)
                ->Field("DebugColor", &MaterialConfiguration2::m_debugColor)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {

                editContext->Class<Physics::MaterialConfiguration2>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Physics Material")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration2::m_staticFriction, "Static friction", "Friction coefficient when object is still")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration2::m_dynamicFriction, "Dynamic friction", "Friction coefficient when object is moving")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration2::m_restitution, "Restitution", "Restitution coefficient")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration2::m_frictionCombine, "Friction combine", "How the friction is combined between colliding objects")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MaterialConfiguration2::m_restitutionCombine, "Restitution combine", "How the restitution is combined between colliding objects")
                        ->EnumAttribute(CombineMode::Average, "Average")
                        ->EnumAttribute(CombineMode::Minimum, "Minimum")
                        ->EnumAttribute(CombineMode::Maximum, "Maximum")
                        ->EnumAttribute(CombineMode::Multiply, "Multiply")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialConfiguration2::m_density, "Density", "Material density")
                        ->Attribute(AZ::Edit::Attributes::Min, MaterialConfiguration2::MinDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Max, MaterialConfiguration2::MaxDensityLimit)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " " + Physics::NameConstants::GetDensityUnit())
                    ->DataElement(AZ::Edit::UIHandlers::Color, &MaterialConfiguration2::m_debugColor, "Debug Color", "Debug color to use for this material")
                    ;
            }
        }
    }
} // namespace Physics
