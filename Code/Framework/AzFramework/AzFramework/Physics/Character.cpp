/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Character.h>
#include <AzCore/Serialization/EditContext.h>


namespace Physics
{
    AZ_CLASS_ALLOCATOR_IMPL(CharacterColliderNodeConfiguration, AZ::SystemAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(CharacterColliderConfiguration, AZ::SystemAllocator)

    void CharacterColliderNodeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CharacterColliderNodeConfiguration>()
                ->Version(1)
                ->Field("name", &CharacterColliderNodeConfiguration::m_name)
                ->Field("shapes", &CharacterColliderNodeConfiguration::m_shapes)
            ;
        }
    }

    Physics::CharacterColliderNodeConfiguration* CharacterColliderConfiguration::FindNodeConfigByName(const AZStd::string& nodeName) const
    {
        auto nodeIterator = AZStd::find_if(m_nodes.begin(), m_nodes.end(), [&nodeName](const Physics::CharacterColliderNodeConfiguration& node)
            {
                return node.m_name == nodeName;
            });

        if (nodeIterator != m_nodes.end())
        {
            return const_cast<Physics::CharacterColliderNodeConfiguration*>(nodeIterator);
        }

        return nullptr;
    }

    AZ::Outcome<size_t> CharacterColliderConfiguration::FindNodeConfigIndexByName(const AZStd::string& nodeName) const
    {
        auto nodeIterator = AZStd::find_if(m_nodes.begin(), m_nodes.end(), [&nodeName](const Physics::CharacterColliderNodeConfiguration& node)
            {
                return node.m_name == nodeName;
            });

        if (nodeIterator != m_nodes.end())
        {
            return AZ::Success(static_cast<size_t>(nodeIterator - m_nodes.begin()));
        }

        return AZ::Failure();
    }

    void CharacterColliderConfiguration::RemoveNodeConfigByName(const AZStd::string& nodeName)
    {
        const AZ::Outcome<size_t> configIndex = FindNodeConfigIndexByName(nodeName);
        if (configIndex.IsSuccess())
        {
            m_nodes.erase(m_nodes.begin() + configIndex.GetValue());
        }
    }

    void CharacterColliderConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CharacterColliderConfiguration>()
                ->Version(1)
                ->Field("nodes", &CharacterColliderConfiguration::m_nodes)
            ;
        }
    }

    void CharacterConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CharacterConfiguration, AzPhysics::SimulatedBodyConfiguration>()
                ->Version(6)
                ->Field("CollisionLayer", &CharacterConfiguration::m_collisionLayer)
                ->Field("CollisionGroupId", &CharacterConfiguration::m_collisionGroupId)
                ->Field("MaterialSlots", &CharacterConfiguration::m_materialSlots)
                ->Field("UpDirection", &CharacterConfiguration::m_upDirection)
                ->Field("MaximumSlopeAngle", &CharacterConfiguration::m_maximumSlopeAngle)
                ->Field("StepHeight", &CharacterConfiguration::m_stepHeight)
                ->Field("MinDistance", &CharacterConfiguration::m_minimumMovementDistance)
                ->Field("MaxSpeed", &CharacterConfiguration::m_maximumSpeed)
                ->Field("ColliderTag", &CharacterConfiguration::m_colliderTag)
                ->Field("ApplyMoveOnPhysicsTick", &CharacterConfiguration::m_applyMoveOnPhysicsTick)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CharacterConfiguration>(
                    "Character Configuration", "Character Configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_collisionLayer,
                        "Collision Layer", "The collision layer assigned to the controller")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_collisionGroupId,
                        "Collides With", "The collision layers this character controller collides with")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_materialSlots, "", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_maximumSlopeAngle,
                        "Maximum Slope Angle", "Maximum angle of slopes on which the controller can walk")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 89.0f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " degrees")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_stepHeight,
                        "Step Height", "Affects the height of steps the character controller will be able to traverse")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_minimumMovementDistance,
                        "Minimum Movement Distance", "To avoid jittering, the controller will not attempt to move distances below this")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.001f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_maximumSpeed,
                        "Maximum Speed", "If the accumulated requested velocity for a tick exceeds this magnitude, it will be clamped")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_colliderTag,
                        "Collider Tag", "Used to identify the collider associated with the character controller")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterConfiguration::m_applyMoveOnPhysicsTick,
                        "Apply Move On Tick", "Requests to add velocity will be accumulated and applied once on the physics pre-simulation tick "
                        "If unticked, explicit call to apply requested velocity is required")
                ;
            }
        }
    }
} // Physics
