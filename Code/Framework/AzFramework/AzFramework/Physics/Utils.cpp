/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "Utils.h"
#include "Material.h"
#include "Shape.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzFramework/Physics/CharacterBus.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/WindBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/CollisionConfiguration.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/SceneConfiguration.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/Common/PhysicsJoint.h>

namespace Physics
{
    namespace ReflectionUtils
    {
        void ReflectSimulatedBodyComponentRequestsBus(AZ::ReflectContext* context)
        {
            if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<AzPhysics::SimulatedBodyComponentRequestsBus>("SimulatedBodyComponentRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                    ->Event("EnablePhysics", &AzPhysics::SimulatedBodyComponentRequests::EnablePhysics)
                    ->Event("DisablePhysics", &AzPhysics::SimulatedBodyComponentRequests::DisablePhysics)
                    ->Event("IsPhysicsEnabled", &AzPhysics::SimulatedBodyComponentRequests::IsPhysicsEnabled)
                    ->Event("GetAabb", &AzPhysics::SimulatedBodyComponentRequests::GetAabb)
                    ->Event("RayCast", &AzPhysics::SimulatedBodyComponentRequests::RayCast)
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

        void ReflectCharacterBus(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<CharacterRequestBus>("CharacterControllerRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "physics")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Event("GetBasePosition", &CharacterRequests::GetBasePosition, "Get Base Position")
                    ->Event("SetBasePosition", &CharacterRequests::SetBasePosition, "Set Base Position")
                    ->Event("GetCenterPosition", &CharacterRequests::GetCenterPosition, "Get Center Position")
                    ->Event("GetStepHeight", &CharacterRequests::GetStepHeight, "Get Step Height")
                    ->Event("SetStepHeight", &CharacterRequests::SetStepHeight, "Set Step Height")
                    ->Event("GetUpDirection", &CharacterRequests::GetUpDirection, "Get Up Direction")
                    ->Event("GetSlopeLimitDegrees", &CharacterRequests::GetSlopeLimitDegrees, "Get Slope Limit (Degrees)")
                    ->Event("SetSlopeLimitDegrees", &CharacterRequests::SetSlopeLimitDegrees, "Set Slope Limit (Degrees)")
                    ->Event("GetMaximumSpeed", &CharacterRequests::GetMaximumSpeed, "Get Maximum Speed")
                    ->Event("SetMaximumSpeed", &CharacterRequests::SetMaximumSpeed, "Set Maximum Speed")
                    ->Event("GetVelocity", &CharacterRequests::GetVelocity, "Get Velocity")
                    ->Event("AddVelocity", &CharacterRequests::AddVelocity, "Add Velocity")
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
            HeightfieldShapeConfiguration::Reflect(context);
            AzPhysics::SystemInterface::Reflect(context);
            AzPhysics::Scene::Reflect(context);
            AzPhysics::CollisionLayer::Reflect(context);
            AzPhysics::CollisionGroup::Reflect(context);
            AzPhysics::CollisionLayers::Reflect(context);
            AzPhysics::CollisionGroups::Reflect(context);
            AzPhysics::CollisionConfiguration::Reflect(context);
            AzPhysics::CollisionEvent::Reflect(context);
            AzPhysics::TriggerEvent::Reflect(context);
            AzPhysics::SceneConfiguration::Reflect(context);
            MaterialConfiguration::Reflect(context);
            DefaultMaterialConfiguration::Reflect(context);
            MaterialLibraryAsset::Reflect(context);
            MaterialInfoReflectionWrapper::Reflect(context);
            AzPhysics::SimulatedBodyConfiguration::Reflect(context);
            AzPhysics::RigidBodyConfiguration::Reflect(context);
            AzPhysics::JointConfiguration::Reflect(context);
            RagdollNodeConfiguration::Reflect(context);
            RagdollConfiguration::Reflect(context);
            CharacterColliderNodeConfiguration::Reflect(context);
            CharacterColliderConfiguration::Reflect(context);
            AnimationConfiguration::Reflect(context);
            CharacterConfiguration::Reflect(context);
            AzPhysics::SimulatedBody::Reflect(context);
            AzPhysics::Joint::Reflect(context);
            ReflectSimulatedBodyComponentRequestsBus(context);
            CollisionFilteringRequests::Reflect(context);
            AzPhysics::SceneQuery::ReflectSceneQueryObjects(context);
            ReflectWindBus(context);
            ReflectCharacterBus(context);
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

        bool FilterTag(AZ::Crc32 tag, AZ::Crc32 filterTag)
        {
            // If the filter tag is empty, then ignore it
            return !filterTag || tag == filterTag;
        }
    }
}
