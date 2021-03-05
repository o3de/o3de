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


#include "Utils.h"
#include "RigidBody.h"
#include "World.h"
#include "Material.h"
#include "Shape.h"
#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/ScriptCanvasPhysicsUtils.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/WorldBodyBus.h>
#include <AzFramework/Physics/WindBus.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

namespace Physics
{
    namespace ReflectionUtils
    {

        /// Behavior handler which forwards TriggerNotificationBus events to script canvas.
        /// Note this class is not using the usual AZ_EBUS_BEHAVIOR_BINDER macro as the signature
        /// needs to be changed for script canvas
        class TriggerNotificationBusBehaviorHandler
            : public TriggerNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:

            AZ_CLASS_ALLOCATOR(TriggerNotificationBusBehaviorHandler, AZ::SystemAllocator, 0);
            AZ_RTTI(TriggerNotificationBusBehaviorHandler, "{0519A121-16F9-4A97-8D54-092BCD963B95}", AZ::BehaviorEBusHandler);

            TriggerNotificationBusBehaviorHandler() {
                m_events.resize(FN_MAX);
                SetEvent(&TriggerNotificationBusBehaviorHandler::OnTriggerEnterDummy, "OnTriggerEnter");
                SetEvent(&TriggerNotificationBusBehaviorHandler::OnTriggerExitDummy, "OnTriggerExit");
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
                {
                    behaviorContext->EBus<Physics::TriggerNotificationBus>("TriggerNotificationBus")
                        ->Attribute(AZ::Script::Attributes::Module, "physics")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Handler<TriggerNotificationBusBehaviorHandler>()
                        ;
                }
            }

            void OnTriggerEnterDummy(AZ::EntityId /*entityId*/)
            {
                // This is never invoked, and only used for type deduction when calling SetEvent
            }
            void OnTriggerExitDummy(AZ::EntityId /*entityId*/)
            {
                // This is never invoked, and only used for type deduction when calling SetEvent
            }

            using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence 
            < 
                decltype(&TriggerNotificationBusBehaviorHandler::OnTriggerEnterDummy),
                decltype(&TriggerNotificationBusBehaviorHandler::OnTriggerExitDummy)
            >;

        private:

            enum 
            {
                FN_OnTriggerEnter,
                FN_OnTriggerExit,
                FN_MAX
            };

            void Disconnect() override 
            {
                BusDisconnect();
            }

            bool Connect(AZ::BehaviorValueParameter * id = nullptr) override 
            {
                return AZ::Internal::EBusConnector<TriggerNotificationBusBehaviorHandler>::Connect(this, id);
            }

            bool IsConnected() override 
            {
                return AZ::Internal::EBusConnector<TriggerNotificationBusBehaviorHandler>::IsConnected(this);
            }

            bool IsConnectedId(AZ::BehaviorValueParameter * id) override 
            {
                return AZ::Internal::EBusConnector<TriggerNotificationBusBehaviorHandler>::IsConnectedId(this, id);
            }

            int GetFunctionIndex(const char * functionName) const override 
            {
                if (strcmp(functionName, "OnTriggerEnter") == 0) return FN_OnTriggerEnter;
                if (strcmp(functionName, "OnTriggerExit") == 0) return FN_OnTriggerExit;
                return -1;
            }

            void OnTriggerEnter(const TriggerEvent& triggerEvent) override
            {
                Call(FN_OnTriggerEnter, triggerEvent.m_otherBody->GetEntityId());
            }

            void OnTriggerExit(const TriggerEvent& triggerEvent) override
            {
                Call(FN_OnTriggerExit, triggerEvent.m_otherBody->GetEntityId());
            }
        };
        
        void ReflectWorldBus(AZ::ReflectContext* context)
        {
            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<Physics::WorldRequestBus>("WorldRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                    ->Event("GetGravity", &Physics::WorldRequestBus::Events::GetGravity)
                    ->Event("SetGravity", &Physics::WorldRequestBus::Events::SetGravity)
                    ;
            }
        }

        void ReflectWorldBodyBus(AZ::ReflectContext* context)
        {
            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<Physics::WorldBodyRequestBus>("WorldBodyRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                    ->Event("EnablePhysics", &WorldBodyRequests::EnablePhysics)
                    ->Event("DisablePhysics", &WorldBodyRequests::DisablePhysics)
                    ->Event("IsPhysicsEnabled", &WorldBodyRequests::IsPhysicsEnabled)
                    ->Event("GetAabb", &WorldBodyRequests::GetAabb)
                    ->Event("RayCast", &WorldBodyRequests::RayCast)
                    ;
            }
        }

        void ReflectWindBus(AZ::ReflectContext* context)
        {
            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                using WindPositionFuncPtr = AZ::Vector3(WindRequests::*)(const AZ::Vector3&) const;
                using WindAabbFuncPtr = AZ::Vector3(WindRequests::*)(const AZ::Aabb&) const;

                behaviorContext->EBus<WindRequestsBus>("WindRequestsBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                    ->Event("GetGlobalWind", &WindRequests::GetGlobalWind)
                    ->Event("GetWindAtPosition", static_cast<WindPositionFuncPtr>(&WindRequests::GetWind))
                    ->Event("GetWindInsideAabb", static_cast<WindAabbFuncPtr>(&WindRequests::GetWind))
                    ;
            }
        }

        void ReflectPhysicsApi(AZ::ReflectContext* context)
        {
            ShapeConfiguration::Reflect(context);
            ColliderConfiguration::Reflect(context);
            BoxShapeConfiguration::Reflect(context);
            CapsuleShapeConfiguration::Reflect(context);
            SphereShapeConfiguration::Reflect(context);
            PhysicsAssetShapeConfiguration::Reflect(context);
            NativeShapeConfiguration::Reflect(context);
            CookedMeshShapeConfiguration::Reflect(context);
            AzPhysics::CollisionLayer::Reflect(context);
            AzPhysics::CollisionGroup::Reflect(context);
            AzPhysics::CollisionLayers::Reflect(context);
            AzPhysics::CollisionGroups::Reflect(context);
            AzPhysics::CollisionConfiguration::Reflect(context);
            AzPhysics::SceneConfiguration::Reflect(context);
            MaterialConfiguration::Reflect(context);
            MaterialLibraryAsset::Reflect(context);
            MaterialLibraryAssetReflectionWrapper::Reflect(context);
            DefaultMaterialLibraryAssetReflectionWrapper::Reflect(context);
            JointLimitConfiguration::Reflect(context);
            WorldBodyConfiguration::Reflect(context);
            RigidBodyConfiguration::Reflect(context);
            RagdollNodeConfiguration::Reflect(context);
            RagdollConfiguration::Reflect(context);
            CharacterColliderNodeConfiguration::Reflect(context);
            CharacterColliderConfiguration::Reflect(context);
            AnimationConfiguration::Reflect(context);
            CharacterConfiguration::Reflect(context);
            ReflectWorldBus(context);
            ReflectWorldBodyBus(context);
            CollisionFilteringRequests::Reflect(context);
            TriggerNotificationBusBehaviorHandler::Reflect(context);
            CollisionNotificationBusBehaviorHandler::Reflect(context);
            RayCastHit::Reflect(context);
            WorldNotificationBusBehaviorHandler::Reflect(context);
            ReflectWindBus(context);
        }
    }

    namespace Utils
    {
        void MakeUniqueString(const AZStd::unordered_set<AZStd::string>& stringSet
            , AZStd::string& stringInOut
            , AZ::u64 maxStringLength)
        {
            AZStd::string originalString = stringInOut;
            for (size_t nameIndex = 1; nameIndex <= stringSet.size(); ++nameIndex)
            {
                AZStd::string postFix;
                to_string(postFix, nameIndex);
                postFix = "_" + postFix;

                AZ::u64 trimLength = (originalString.length() + postFix.length()) - maxStringLength;
                if (trimLength > 0)
                {
                    stringInOut = originalString.substr(0, originalString.length() - trimLength) + postFix;
                }
                else
                {
                    stringInOut = originalString + postFix;
                }

                if (stringSet.find(stringInOut) == stringSet.end())
                {
                    break;
                }
            }
        }

        void DeferDelete(AZStd::unique_ptr<Physics::WorldBody> worldBody)
        {
            if (!worldBody)
            {
                return;
            }

            // If the body is in a world, remove it from the world and defer 
            // the deletion until after the next update to ensure trigger exit events get raised.
            if (Physics::World* world = worldBody->GetWorld())
            {
                world->RemoveBody(*worldBody);
                world->DeferDelete(AZStd::move(worldBody));
            }
        }

        bool FilterTag(AZ::Crc32 tag, AZ::Crc32 filterTag)
        {
            // If the filter tag is empty, then ignore it
            return !filterTag || tag == filterTag;
        }
    }
}
