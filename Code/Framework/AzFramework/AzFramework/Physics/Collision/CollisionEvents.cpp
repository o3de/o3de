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

#include <AzFramework/Physics/Collision/CollisionEvents.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(TriggerEvent, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(Contact, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(CollisionEvent, AZ::SystemAllocator, 0);

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
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
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
                ->Property("Position", BehaviorValueProperty(&Contact::m_position))
                ->Property("Normal", BehaviorValueProperty(&Contact::m_normal))
                ->Property("Impulse", BehaviorValueProperty(&Contact::m_impulse))
                ->Property("Separation", BehaviorValueProperty(&Contact::m_separation))
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
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Property("Contacts", BehaviorValueProperty(&CollisionEvent::m_contacts))
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
