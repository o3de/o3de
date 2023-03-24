/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Collision/CollisionEvents.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(TriggerEvent, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(Contact, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(CollisionEvent, AZ::SystemAllocator);

    /*static*/ void TriggerEvent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TriggerEvent>()
                ->Version(2)
                ->Field("Type", &TriggerEvent::m_type)
                ->Field("TriggerBodyHandle", &TriggerEvent::m_triggerBodyHandle)
                ->Field("OtherBodyHandle", &TriggerEvent::m_otherBodyHandle)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<TriggerEvent>()
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Method("GetTriggerEntityId", &TriggerEvent::GetTriggerEntityId)
                ->Method("GetOtherEntityId", &TriggerEvent::GetOtherEntityId)
                ;
        }
    }

    AZ::EntityId TriggerEvent::GetTriggerEntityId() const
    {
        if (m_triggerBody)
        {
            return m_triggerBody->GetEntityId();
        }
        return AZ::EntityId();
    }

    AZ::EntityId TriggerEvent::GetOtherEntityId() const
    {
        if (m_otherBody)
        {
            return m_otherBody->GetEntityId();
        }
        return AZ::EntityId();
    }

    /*static*/ void Contact::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Contact>()
                ->Version(1)
                ->Field("Position", &Contact::m_position)
                ->Field("Normal", &Contact::m_normal)
                ->Field("Impulse", &Contact::m_impulse)
                ->Field("Separation", &Contact::m_separation)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<Contact>("Contact")
                ->Property("Position", BehaviorValueGetter(&Contact::m_position), nullptr)                
                ->Property("Normal", BehaviorValueGetter(&Contact::m_normal), nullptr)
                ->Property("Impulse", BehaviorValueGetter(&Contact::m_impulse), nullptr)
                ->Property("Separation", BehaviorValueGetter(&Contact::m_separation), nullptr)
                ;
        }
    }

    /*static*/ void CollisionEvent::Reflect(AZ::ReflectContext* context)
    {
        Contact::Reflect(context);

        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionEvent>()
                ->Version(3)
                ->Field("Type", &CollisionEvent::m_type)
                ->Field("Contacts", &CollisionEvent::m_contacts)
                ->Field("BodyHandle1", &CollisionEvent::m_bodyHandle1)
                ->Field("BodyHandle2", &CollisionEvent::m_bodyHandle2)
                ;
        }

        if (auto* behaviorContext = azdynamic_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<CollisionEvent>()
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Property("Contacts", BehaviorValueGetter(&CollisionEvent::m_contacts), nullptr)
                ->Method("GetBody1EntityId", &CollisionEvent::GetBody1EntityId)
                ->Method("GetBody2EntityId", &CollisionEvent::GetBody2EntityId)
                ;
        }
    }

    AZ::EntityId CollisionEvent::GetBody1EntityId() const
    {
        if (m_body1)
        {
            return m_body1->GetEntityId();
        }
        return AZ::EntityId();
    }

    AZ::EntityId CollisionEvent::GetBody2EntityId() const
    {
        if (m_body2)
        {
            return m_body2->GetEntityId();
        }
        return AZ::EntityId();
    }
}
