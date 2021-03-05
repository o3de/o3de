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

#include "ScriptCanvasPhysicsUtils.h"
#include <AzFramework/Physics/WorldBody.h>

namespace Physics
{
    namespace ReflectionUtils
    {
        CollisionNotificationBusBehaviorHandler::CollisionNotificationBusBehaviorHandler()
        {
            m_events.resize(FN_MAX);
            SetEvent(&CollisionNotificationBusBehaviorHandler::OnCollisionBeginDummy, "OnCollisionBegin");
            SetEvent(&CollisionNotificationBusBehaviorHandler::OnCollisionPersistDummy, "OnCollisionPersist");
            SetEvent(&CollisionNotificationBusBehaviorHandler::OnCollisionEndDummy, "OnCollisionEnd");
        }

        void CollisionNotificationBusBehaviorHandler::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Physics::Contact>()
                    ->Version(1)
                    ->Field("Position", &Physics::Contact::m_position)
                    ->Field("Normal", &Physics::Contact::m_normal)
                    ->Field("Impulse", &Physics::Contact::m_impulse)
                    ->Field("Separation", &Physics::Contact::m_separation)
                    ;

                serializeContext->Class<Physics::CollisionEvent>()
                    ->Field("Contacts", &Physics::CollisionEvent::m_contacts)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<Physics::Contact>("Contact")
                    ->Property("Position", BehaviorValueProperty(&Physics::Contact::m_position))
                    ->Property("Normal", BehaviorValueProperty(&Physics::Contact::m_normal))
                    ->Property("Impulse", BehaviorValueProperty(&Physics::Contact::m_impulse))
                    ->Property("Separation", BehaviorValueProperty(&Physics::Contact::m_separation))
                    ;

                behaviorContext->Class<Physics::CollisionEvent>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                    ->Property("Contacts", BehaviorValueProperty(&Physics::CollisionEvent::m_contacts))
                    ;

                behaviorContext->EBus<Physics::CollisionNotificationBus>("CollisionNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Handler<CollisionNotificationBusBehaviorHandler>()
                    ;
            }
        }

        void CollisionNotificationBusBehaviorHandler::Disconnect()
        {
            BusDisconnect();
        }

        bool CollisionNotificationBusBehaviorHandler::Connect(AZ::BehaviorValueParameter* id)
        {
            return AZ::Internal::EBusConnector<CollisionNotificationBusBehaviorHandler>::Connect(this, id);
        }

        bool CollisionNotificationBusBehaviorHandler::IsConnected()
        {
            return AZ::Internal::EBusConnector<CollisionNotificationBusBehaviorHandler>::IsConnected(this);
        }

        bool CollisionNotificationBusBehaviorHandler::IsConnectedId(AZ::BehaviorValueParameter* id)
        {
            return AZ::Internal::EBusConnector<CollisionNotificationBusBehaviorHandler>::IsConnectedId(this, id);
        }

        int CollisionNotificationBusBehaviorHandler::GetFunctionIndex(const char* functionName) const
        {
            if (strcmp(functionName, "OnCollisionBegin") == 0) return FN_OnCollisionBegin;
            if (strcmp(functionName, "OnCollisionPersist") == 0) return FN_OnCollisionPersist;
            if (strcmp(functionName, "OnCollisionEnd") == 0) return FN_OnCollisionEnd;
            return -1;
        }

        void CollisionNotificationBusBehaviorHandler::OnCollisionBeginDummy(AZ::EntityId /*entityId*/, const AZStd::vector<Contact>& /*contacts*/)
        {
            // This is never invoked, and only used for type deduction when calling SetEvent
        }

        void CollisionNotificationBusBehaviorHandler::OnCollisionPersistDummy(AZ::EntityId /*entityId*/, const AZStd::vector<Contact>& /*contacts*/)
        {
            // This is never invoked, and only used for type deduction when calling SetEvent
        }
        
        void CollisionNotificationBusBehaviorHandler::OnCollisionEndDummy(AZ::EntityId /*entityId*/)
        {
            // This is never invoked, and only used for type deduction when calling SetEvent
        }

        void CollisionNotificationBusBehaviorHandler::OnCollisionBegin(const CollisionEvent& collisionEvent)
        {
            Call(FN_OnCollisionBegin, collisionEvent.m_body2->GetEntityId(), collisionEvent.m_contacts);
        }

        void CollisionNotificationBusBehaviorHandler::OnCollisionPersist(const CollisionEvent& collisionEvent)
        {
            Call(FN_OnCollisionPersist, collisionEvent.m_body2->GetEntityId(), collisionEvent.m_contacts);
        }

        void CollisionNotificationBusBehaviorHandler::OnCollisionEnd(const CollisionEvent& collisionEvent)
        {
            Call(FN_OnCollisionEnd, collisionEvent.m_body2->GetEntityId());
        }

        void WorldNotificationBusBehaviorHandler::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<WorldNotificationBus>("WorldNotificationBus")
                    ->Handler<WorldNotificationBusBehaviorHandler>()
                    ;
            }
        }

        void WorldNotificationBusBehaviorHandler::OnPrePhysicsTick(float deltaTime)
        {
            Call(FN_OnPrePhysicsTick, deltaTime);
        }

        void WorldNotificationBusBehaviorHandler::OnPrePhysicsSubtick(float fixedDeltaTime)
        {
            Call(FN_OnPrePhysicsSubtick, fixedDeltaTime);
        }

        void WorldNotificationBusBehaviorHandler::OnPostPhysicsSubtick(float fixedDeltaTime)
        {
            Call(FN_OnPostPhysicsSubtick, fixedDeltaTime);
        }

        void WorldNotificationBusBehaviorHandler::OnPostPhysicsTick(float deltaTime)
        {
            Call(FN_OnPostPhysicsTick, deltaTime);
        }

        int WorldNotificationBusBehaviorHandler::GetPhysicsTickOrder()
        {
            int order = WorldNotifications::Scripting;
            CallResult(order, FN_GetPhysicsTickOrder);
            return order;
        }
    } // namespace ReflectionUtils
} // namespace Physics
