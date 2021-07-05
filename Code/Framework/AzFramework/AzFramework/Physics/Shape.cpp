/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Shape.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/ClassConverters.h>

namespace Physics
{
    const float ColliderConfiguration::ContactOffsetDelta = 1e-2f;

    void ColliderConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ColliderConfiguration>()
                ->Version(4, &Physics::ClassConverters::ColliderConfigurationConverter)
                ->Field("CollisionLayer", &ColliderConfiguration::m_collisionLayer)
                ->Field("CollisionGroupId", &ColliderConfiguration::m_collisionGroupId)
                ->Field("Visible", &ColliderConfiguration::m_visible)
                ->Field("Trigger", &ColliderConfiguration::m_isTrigger)
                ->Field("Simulated", &ColliderConfiguration::m_isSimulated)
                ->Field("InSceneQueries", &ColliderConfiguration::m_isInSceneQueries)
                ->Field("Exclusive", &ColliderConfiguration::m_isExclusive)
                ->Field("Position", &ColliderConfiguration::m_position)
                ->Field("Rotation", &ColliderConfiguration::m_rotation)
                ->Field("MaterialSelection", &ColliderConfiguration::m_materialSelection)
                ->Field("propertyVisibilityFlags", &ColliderConfiguration::m_propertyVisibilityFlags)
                ->Field("ColliderTag", &ColliderConfiguration::m_tag)
                ->Field("RestOffset", &ColliderConfiguration::m_restOffset)
                ->Field("ContactOffset", &ColliderConfiguration::m_contactOffset)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ColliderConfiguration>("ColliderConfiguration", "Configuration for a collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_isTrigger, "Trigger", "If set, this collider will act as a trigger")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetIsTriggerVisibility)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_isSimulated, "Simulated", "If set, this collider will partake in collision in the physical simulation")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetIsTriggerVisibility)
                        ->Attribute(AZ::Edit::Attributes::ReadOnly, &ColliderConfiguration::m_isTrigger) // Trigger shapes ignore simulated flag, making it read-only in the UI.
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_isInSceneQueries, "In Scene Queries", "If set, this collider will be visible for scene queries")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetIsTriggerVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_collisionLayer, "Collision Layer", "The collision layer assigned to the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetCollisionLayerVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_collisionGroupId, "Collides With", "The collision group containing the layers this collider collides with")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetCollisionLayerVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_position, "Offset", "Local offset from the rigid body")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetOffsetVisibility)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_rotation, "Rotation", "Local rotation relative to the rigid body")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetOffsetVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_materialSelection, "Physics Materials", "Select which physics materials to use for each element of this shape")
                        ->Attribute(AZ::Edit::Attributes::Visibility, &ColliderConfiguration::GetMaterialSelectionVisibility)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_tag, "Tag", "Tag used to identify colliders from one another")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_restOffset, "Rest offset",
                        "Bodies will come to rest separated by the sum of their rest offset values (must be less than contact offset)")
                        ->Attribute(AZ::Edit::Attributes::Step, 1e-2f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ColliderConfiguration::OnRestOffsetChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ColliderConfiguration::m_contactOffset, "Contact offset",
                        "Bodies will begin to generate contacts when within the sum of their contact offsets (must exceed rest offset)")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1e-2f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ColliderConfiguration::OnContactOffsetChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ;
            }
        }
    }

    void ColliderConfiguration::OnRestOffsetChanged()
    {
        if (m_restOffset > m_contactOffset - ContactOffsetDelta)
        {
            m_restOffset = AZ::GetMax(0.0f, m_contactOffset - ContactOffsetDelta);
        }
    }

    void ColliderConfiguration::OnContactOffsetChanged()
    {
        if (m_contactOffset < m_restOffset + ContactOffsetDelta)
        {
            m_contactOffset = AZ::GetMin(1.0f, m_restOffset + ContactOffsetDelta);
        }
    }

    AZ::Crc32 ColliderConfiguration::GetPropertyVisibility(PropertyVisibility property) const
    {
        return (m_propertyVisibilityFlags & property) != 0 ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void ColliderConfiguration::SetPropertyVisibility(PropertyVisibility property, bool isVisible)
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

    AZ::Crc32 ColliderConfiguration::GetIsTriggerVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::IsTrigger);
    }

    AZ::Crc32 ColliderConfiguration::GetCollisionLayerVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::CollisionLayer);
    }

    AZ::Crc32 ColliderConfiguration::GetMaterialSelectionVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::MaterialSelection);
    }

    AZ::Crc32 ColliderConfiguration::GetOffsetVisibility() const
    {
        return GetPropertyVisibility(PropertyVisibility::Offset);
    }
}
