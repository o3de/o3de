/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/JointConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(JointConfiguration, AZ::SystemAllocator);

    void JointConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointConfiguration>()
                ->Version(1)
                ->Field("Name", &JointConfiguration::m_debugName)
                ->Field("ParentLocalRotation", &JointConfiguration::m_parentLocalRotation)
                ->Field("ParentLocalPosition", &JointConfiguration::m_parentLocalPosition)
                ->Field("ChildLocalRotation", &JointConfiguration::m_childLocalRotation)
                ->Field("ChildLocalPosition", &JointConfiguration::m_childLocalPosition)
                ->Field("StartSimulationEnabled", &JointConfiguration::m_startSimulationEnabled)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<JointConfiguration>("Joint Configuration", "Joint configuration.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointConfiguration::m_parentLocalRotation,
                        "Parent local rotation", "Parent joint frame relative to parent body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &JointConfiguration::GetParentLocalRotationVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointConfiguration::m_parentLocalPosition,
                        "Parent local position", "Joint position relative to parent body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &JointConfiguration::GetParentLocalPositionVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointConfiguration::m_childLocalRotation,
                        "Child local rotation", "Child joint frame relative to child body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &JointConfiguration::GetChildLocalRotationVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointConfiguration::m_childLocalPosition,
                        "Child local position", "Joint position relative to child body.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &JointConfiguration::GetChildLocalPositionVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &JointConfiguration::m_startSimulationEnabled,
                        "Start simulation enabled", "When active, the joint will be enabled when the simulation begins.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &JointConfiguration::GetStartSimulationEnabledVisibility)
                    ;
            }
        }
    }

    AZ::Crc32 JointConfiguration::GetPropertyVisibility(JointConfiguration::PropertyVisibility property) const
    {
        return (m_propertyVisibilityFlags & property) != 0 ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void JointConfiguration::SetPropertyVisibility(JointConfiguration::PropertyVisibility property, bool isVisible)
    {
        if (isVisible)
        {
            m_propertyVisibilityFlags |= property;
        }
        else
        {
            m_propertyVisibilityFlags &= ~property;
        }
    }

    AZ::Crc32 JointConfiguration::GetParentLocalRotationVisibility() const
    {
        return GetPropertyVisibility(JointConfiguration::PropertyVisibility::ParentLocalRotation);
    }

    AZ::Crc32 JointConfiguration::GetParentLocalPositionVisibility() const
    {
        return GetPropertyVisibility(JointConfiguration::PropertyVisibility::ParentLocalPosition);
    }

    AZ::Crc32 JointConfiguration::GetChildLocalRotationVisibility() const
    {
        return GetPropertyVisibility(JointConfiguration::PropertyVisibility::ChildLocalRotation);
    }

    AZ::Crc32 JointConfiguration::GetChildLocalPositionVisibility() const
    {
        return GetPropertyVisibility(JointConfiguration::PropertyVisibility::ChildLocalPosition);
    }

    AZ::Crc32 JointConfiguration::GetStartSimulationEnabledVisibility() const
    {
        return GetPropertyVisibility(JointConfiguration::PropertyVisibility::StartSimulationEnabled);
    }
} // namespace AzPhysics
