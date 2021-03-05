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

#pragma once

#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Physics/World.h>

namespace Physics
{
    namespace ReflectionUtils
    {
        /// Behavior handler which forwards CollisionNotificationBus events to script canvas.
        /// Note this class is not using the usual AZ_EBUS_BEHAVIOR_BINDER macro as the signature
        /// needs to be changed for script canvas
        class CollisionNotificationBusBehaviorHandler
            : public CollisionNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:

            AZ_CLASS_ALLOCATOR(CollisionNotificationBusBehaviorHandler, AZ::SystemAllocator, 0);
            AZ_RTTI(CollisionNotificationBusBehaviorHandler, "{A28ACB8F-3429-4F92-88DB-481ACF90EF21}", AZ::BehaviorEBusHandler);
            static void Reflect(AZ::ReflectContext* context);

            CollisionNotificationBusBehaviorHandler();

            // Script Canvas Signature
            void OnCollisionBeginDummy(AZ::EntityId entityId, const AZStd::vector<Contact>& contacts);
            void OnCollisionPersistDummy(AZ::EntityId entityId, const AZStd::vector<Contact>& contacts);
            void OnCollisionEndDummy(AZ::EntityId entityId);

            using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence
                <
                decltype(&CollisionNotificationBusBehaviorHandler::OnCollisionBeginDummy),
                decltype(&CollisionNotificationBusBehaviorHandler::OnCollisionPersistDummy),
                decltype(&CollisionNotificationBusBehaviorHandler::OnCollisionEndDummy)
                >;

        private:
            enum
            {
                FN_OnCollisionBegin,
                FN_OnCollisionPersist,
                FN_OnCollisionEnd,
                FN_MAX
            };

            // AZ::BehaviorEBusHandler
            void Disconnect() override;
            bool Connect(AZ::BehaviorValueParameter* id = nullptr) override;
            bool IsConnected() override;
            bool IsConnectedId(AZ::BehaviorValueParameter* id) override;
            int GetFunctionIndex(const char* functionName) const override;


            // CollisionNotificationBus
            void OnCollisionBegin(const CollisionEvent& triggerEvent) override;
            void OnCollisionPersist(const CollisionEvent& triggerEvent) override;
            void OnCollisionEnd(const CollisionEvent& triggerEvent) override;
        };

        class WorldNotificationBusBehaviorHandler
            : public WorldNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            static void Reflect(AZ::ReflectContext* context);
            AZ_EBUS_BEHAVIOR_BINDER(WorldNotificationBusBehaviorHandler, "{D8B108B8-9126-4C66-B857-377BA5DB3062}", AZ::SystemAllocator
                , OnPrePhysicsTick
                , OnPrePhysicsSubtick
                , OnPostPhysicsSubtick
                , OnPostPhysicsTick
                , GetPhysicsTickOrder
            );

            // WorldNotificationBus ...
            void OnPrePhysicsTick(float deltaTime) override;
            void OnPrePhysicsSubtick(float fixedDeltaTime) override;
            void OnPostPhysicsSubtick(float fixedDeltaTime) override;
            void OnPostPhysicsTick(float deltaTime) override;
            int GetPhysicsTickOrder() override;
        };
    } // namespace ReflectionUtils
} // namespace Physics
