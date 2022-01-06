/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <Common/PhysXSceneQueryHelpers.h>
#include <PhysX/Utils.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>
#include <PhysX/Debug/PhysXDebugConfiguration.h>
#include <PhysX/MathConversion.h>
#include <PhysXCharacters/API/CharacterController.h>
#include <Source/Collision.h>
#include <Source/Shape.h>

namespace PhysX
{
    void CharacterControllerConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterControllerConfiguration, Physics::CharacterConfiguration>()
                ->Version(1)
                ->Field("SlopeBehaviour", &CharacterControllerConfiguration::m_slopeBehaviour)
                ->Field("ContactOffset", &CharacterControllerConfiguration::m_contactOffset)
                ->Field("ScaleCoeff", &CharacterControllerConfiguration::m_scaleCoefficient)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CharacterControllerConfiguration>(
                    "PhysX Character Controller Configuration", "PhysX Character Controller Configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &CharacterControllerConfiguration::m_slopeBehaviour,
                        "Slope Behavior", "Behavior of the controller on surfaces that exceed the Maximum Slope Angle.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->EnumAttribute(SlopeBehaviour::PreventClimbing, "Prevent Climbing")
                    ->EnumAttribute(SlopeBehaviour::ForceSliding, "Force Sliding")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterControllerConfiguration::m_contactOffset,
                        "Contact Offset", "Distance from the controller boundary where contact with surfaces can be resolved.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CharacterControllerConfiguration::m_scaleCoefficient,
                        "Scale", "Scales the controller. Usually less than 1.0 to ensure visual contact between the character and surface.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    void CharacterControllerCallbackManager::SetControllerFilter(ControllerFilter controllerFilter)
    {
        m_controllerFilter = controllerFilter;
    }

    void CharacterControllerCallbackManager::SetObjectPreFilter(ObjectPreFilter objectPreFilter)
    {
        m_objectPreFilter = objectPreFilter;
    }

    void CharacterControllerCallbackManager::SetObjectPostFilter(ObjectPostFilter objectPostFilter)
    {
        m_objectPostFilter = objectPostFilter;
    }

    void CharacterControllerCallbackManager::SetOnShapeHit(OnShapeHit onShapeHit)
    {
        m_onShapeHit = onShapeHit;
    }

    void CharacterControllerCallbackManager::SetOnControllerHit(OnControllerHit onControllerHit)
    {
        m_onControllerHit = onControllerHit;
    }

    void CharacterControllerCallbackManager::SetOnObstacleHit(OnObstacleHit onObstacleHit)
    {
        m_onObstacleHit = onObstacleHit;
    }

    void CharacterControllerCallbackManager::SetObjectRidingBehavior(ObjectRidingBehavior objectRidingBehavior)
    {
        m_objectRidingBehavior = objectRidingBehavior;
    }

    void CharacterControllerCallbackManager::SetControllerRidingBehavior(ControllerRidingBehavior controllerRidingBehavior)
    {
        m_controllerRidingBehavior = controllerRidingBehavior;
    }

    void CharacterControllerCallbackManager::SetObstacleRidingBehavior(ObstacleRidingBehavior obstacleRidingBehavior)
    {
        m_obstacleRidingBehavior = obstacleRidingBehavior;
    }

    bool CharacterControllerCallbackManager::filter(
        const physx::PxController& controllerA, const physx::PxController& controllerB)
    {
        return m_controllerFilter
            ? m_controllerFilter(controllerA, controllerB)
            : true;
    }

    physx::PxQueryHitType::Enum CharacterControllerCallbackManager::preFilter(
        const physx::PxFilterData& filterData, const physx::PxShape* shape,
        const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags)
    {
        return m_objectPreFilter
            ? m_objectPreFilter(filterData, shape, actor, queryFlags)
            : physx::PxQueryHitType::Enum::eBLOCK;
    }

    physx::PxQueryHitType::Enum CharacterControllerCallbackManager::postFilter(
        const physx::PxFilterData& filterData, const physx::PxQueryHit& hit)
    {
        return m_objectPostFilter
            ? m_objectPostFilter(filterData, hit)
            : physx::PxQueryHitType::Enum::eBLOCK;
    }

    void CharacterControllerCallbackManager::onShapeHit(const physx::PxControllerShapeHit& hit)
    {
        if (m_onShapeHit)
        {
            m_onShapeHit(hit);
        }
    }

    void CharacterControllerCallbackManager::onControllerHit(const physx::PxControllersHit& hit)
    {
        if (m_onControllerHit)
        {
            m_onControllerHit(hit);
        }
    }

    void CharacterControllerCallbackManager::onObstacleHit(const physx::PxControllerObstacleHit& hit)
    {
        if (m_onObstacleHit)
        {
            m_onObstacleHit(hit);
        }
    }

    physx::PxControllerBehaviorFlags CharacterControllerCallbackManager::getBehaviorFlags(
        const physx::PxShape& shape, const physx::PxActor& actor)
    {
        if (m_objectRidingBehavior)
        {
            return m_objectRidingBehavior(shape, actor);
        }

        // default flag for riding on objects in PhysX if a callback is not defined
        return physx::PxControllerBehaviorFlags(0);
    }

    physx::PxControllerBehaviorFlags CharacterControllerCallbackManager::getBehaviorFlags(
        const physx::PxController& controller)
    {
        if (m_controllerRidingBehavior)
        {
            return m_controllerRidingBehavior(controller);
        }

        // default flag for riding on controllers in PhysX if a callback is not defined
        return physx::PxControllerBehaviorFlags(0);
    }

    physx::PxControllerBehaviorFlags CharacterControllerCallbackManager::getBehaviorFlags(
        const physx::PxObstacle& obstacle)
    {
        if (m_obstacleRidingBehavior)
        {
            return m_obstacleRidingBehavior(obstacle);
        }

        // default flag for riding on obstacles in PhysX if a callback is not defined
        return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;
    }

    void CharacterController::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CharacterController>()
                ->Version(1)
                ;
        }
    }

    CharacterController::CharacterController(physx::PxController* pxController,
        AZStd::unique_ptr<CharacterControllerCallbackManager> callbackManager,
        AzPhysics::SceneHandle sceneHandle)
        : m_pxController(pxController)
        , m_callbackManager(AZStd::move(callbackManager))
    {
        m_sceneOwner = sceneHandle;
        m_simulating = false; //character controller starts disabled, so set m_simulating to false

        AZ_Assert(m_pxController, "pxController should not be null.");
        m_pxControllerFilters.mFilterCallback = m_callbackManager.get();
        m_pxControllerFilters.mCCTFilterCallback = m_callbackManager.get();
    }

    void CharacterController::SetFilterDataAndShape(const Physics::CharacterConfiguration& characterConfig)
    {
        AzPhysics::CollisionGroup collisionGroup;
        Physics::CollisionRequestBus::BroadcastResult(collisionGroup, &Physics::CollisionRequests::GetCollisionGroupById, characterConfig.m_collisionGroupId);

        UpdateFilterLayerAndGroup(characterConfig.m_collisionLayer, collisionGroup);

        physx::PxRigidDynamic* actor = nullptr;
        physx::PxU32 numShapes = 0;
        {
            PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
            actor = m_pxController->getActor();
            numShapes = actor->getNbShapes();
        }
        if (numShapes != 1)
        {
            AZ_Error("PhysX Character Controller", false, "Found %i shapes, expected exactly 1.", numShapes)
        }
        else
        {
            physx::PxShape* pxShape = nullptr;
            {
                PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
                actor->getShapes(&pxShape, 1, 0);

                // create a PhysX::Shape from the raw PxShape so that it is appropriately configured for raycasts etc.
                m_shape = AZStd::make_shared<Shape>(pxShape);
            }
            if (m_shape)
            {
                PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
                m_shape->AttachedToActor(actor);
                m_shape->SetCollisionLayer(characterConfig.m_collisionLayer);
                m_shape->SetCollisionGroup(collisionGroup);
            }
        }
    }

    void CharacterController::SetActorName(const AZStd::string& name)
    {
        m_name = name;
        if (m_pxController)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
            m_pxController->getActor()->setName(m_name.c_str());
        }
    }

    void CharacterController::SetUserData(const Physics::CharacterConfiguration& characterConfig)
    {
        m_actorUserData = PhysX::ActorData(m_pxController->getActor());
        m_actorUserData.SetCharacter(this);
        m_actorUserData.SetEntityId(characterConfig.m_entityId);
    }

    void CharacterController::SetMinimumMovementDistance(float distance)
    {
        m_minimumMovementDistance = distance;
    }

    void CharacterController::CreateShadowBody(const Physics::CharacterConfiguration& configuration)
    {
        DestroyShadowBody();

        AzPhysics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_kinematic = true;
        rigidBodyConfig.m_debugName = configuration.m_debugName + " (Shadow)";
        rigidBodyConfig.m_entityId = configuration.m_entityId;
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_shadowBodyHandle = sceneInterface->AddSimulatedBody(m_sceneOwner, &rigidBodyConfig);
            if (m_shadowBodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
            {
                AZ_Error("PhysXCharacter", false, "Failed to create the CharacterController rigid body.");
                return;
            }
            m_shadowBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_sceneOwner, m_shadowBodyHandle));
        }
    }

    void CharacterController::EnablePhysics(const Physics::CharacterConfiguration& configuration)
    {
        if (m_simulating)
        {
            return;
        }

        SetFilterDataAndShape(configuration);
        SetUserData(configuration);
        SetActorName(configuration.m_debugName);
        SetMinimumMovementDistance(configuration.m_minimumMovementDistance);
        SetMaximumSpeed(configuration.m_maximumSpeed);
        CreateShadowBody(configuration);
        SetTag(configuration.m_colliderTag);

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->EnableSimulationOfBody(m_sceneOwner, m_bodyHandle);
        }
    }

    void CharacterController::DisablePhysics()
    {
        if (!m_simulating)
        {
            return;
        }

        DestroyShadowBody();
        RemoveControllerFromScene();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_sceneOwner, m_bodyHandle);
        }
    }

    void CharacterController::DestroyShadowBody()
    {
        if (m_shadowBodyHandle == AzPhysics::InvalidSimulatedBodyHandle)
        {
            return;
        }

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_sceneOwner, m_shadowBodyHandle);
            m_shadowBody = nullptr;
        }
    }

    void CharacterController::RemoveControllerFromScene()
    {
        if (m_pxController)
        {
            if (auto* pxScene = m_pxController->getScene())
            {
                PHYSX_SCENE_WRITE_LOCK(pxScene);
                pxScene->removeActor(*m_pxController->getActor());
                
            }
        }
    }

    void CharacterController::SetTag(const AZStd::string& tag)
    {
        m_colliderTag = AZ::Crc32(tag);
    }

    CharacterControllerCallbackManager* CharacterController::GetCallbackManager()
    {
        return m_callbackManager.get();
    }

    CharacterController::~CharacterController()
    {
        DestroyShadowBody();
        m_shape = nullptr; //shape has to go before m_pxController

        if (m_pxController)
        {
            PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());    
            m_pxController->release(); // This internally removes the controller's actor from the scene
        }

        m_pxController = nullptr;
        m_material = nullptr;
    }

    // Physics::Character
    AZ::Vector3 CharacterController::GetBasePosition() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }
        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        return PxMathConvertExtended(m_pxController->getFootPosition());
    }

    void CharacterController::SetBasePosition(const AZ::Vector3& position)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        m_pxController->setFootPosition(PxMathConvertExtended(position));
        if (m_shadowBody)
        {
            m_shadowBody->SetTransform(AZ::Transform::CreateTranslation(GetBasePosition()));
        }
    }

    AZ::Vector3 CharacterController::GetCenterPosition() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }

        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            const float halfHeight = 0.5f * capsuleController->getHeight() + capsuleController->getRadius();
            return GetBasePosition() + PxMathConvert(halfHeight * m_pxController->getUpDirection());
        }

        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            auto boxController = static_cast<physx::PxBoxController*>(m_pxController);
            return GetBasePosition() + AZ::Vector3::CreateAxisZ(boxController->getHalfHeight());
        }

        AZ_Warning("PhysX Character Controller", false, "Unrecognized shape type.");
        return AZ::Vector3::CreateZero();
    }

    template<class T>
    static T CheckValidAndReturn(physx::PxController* controller, T result)
    {
        if (!controller)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
        }

        return result;
    }

    float CharacterController::GetStepHeight() const
    {
        return CheckValidAndReturn(m_pxController, m_pxController ? m_pxController->getStepOffset() : 0.0f);
    }

    void CharacterController::SetStepHeight(float stepHeight)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        if (stepHeight <= 0.0f)
        {
            AZ_Warning("PhysX Character Controller", false, "PhysX requires the step height to be positive.");
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        m_pxController->setStepOffset(stepHeight);
    }

    AZ::Vector3 CharacterController::GetUpDirection() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }

        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        return PxMathConvert(m_pxController->getUpDirection());
    }

    void CharacterController::SetUpDirection([[maybe_unused]] const AZ::Vector3& upDirection)
    {
        AZ_Warning("PhysX Character Controller", false, "Setting up direction is not currently supported.");
        return;
    }

    float CharacterController::GetSlopeLimitDegrees() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }
        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        return AZ::RadToDeg(acosf(m_pxController->getSlopeLimit()));
    }

    void CharacterController::SetSlopeLimitDegrees(float slopeLimitDegrees)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        float slopeLimitClamped = AZ::GetClamp(slopeLimitDegrees, 0.0f, 90.0f);

        if (slopeLimitDegrees != slopeLimitClamped)
        {
            AZ_Warning("PhysX Character Controller", false, "Slope limit should be in the range 0-90 degrees.  "
                "Value %f was clamped to %f", slopeLimitDegrees, slopeLimitClamped);
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        m_pxController->setSlopeLimit(cosf(AZ::DegToRad(slopeLimitClamped)));
    }

    float CharacterController::GetMaximumSpeed() const
    {
        return m_maximumSpeed;
    }

    void CharacterController::SetMaximumSpeed(float maximumSpeed)
    {
        m_maximumSpeed = AZ::GetMax(0.0f, maximumSpeed);
    }

    AZ::Vector3 CharacterController::GetVelocity() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Vector3::CreateZero();
        }

        return m_observedVelocity;
    }

    void CharacterController::SetCollisionLayer(const AzPhysics::CollisionLayer& layer)
    {
        if (!m_shape)
        {
            AZ_Error("PhysX Character Controller", false, "Attempting to access null shape on character controller.");
            return;
        }
        
        m_shape->SetCollisionLayer(layer);

        UpdateFilterLayerAndGroup(layer, m_shape->GetCollisionGroup());
    }

    void CharacterController::SetCollisionGroup(const AzPhysics::CollisionGroup& group)
    {
        if (!m_shape)
        {
            AZ_Error("PhysX Character Controller", false, "Attempting to access null shape on character controller.");
            return;
        }

        m_shape->SetCollisionGroup(group);

        UpdateFilterLayerAndGroup(m_shape->GetCollisionLayer(), group);
    }

    AzPhysics::CollisionLayer CharacterController::GetCollisionLayer() const
    {
        if (!m_shape)
        {
            AZ_Error("PhysX Character Controller", false, "Attempting to access null shape on character controller.");
            return AzPhysics::CollisionLayer::Default;
        }

        return m_shape->GetCollisionLayer();
    }

    AzPhysics::CollisionGroup CharacterController::GetCollisionGroup() const
    {
        if (!m_shape)
        {
            AZ_Error("PhysX Character Controller", false, "Attempting to access null shape on character controller.");
            return AzPhysics::CollisionGroup::All;
        }

        return m_shape->GetCollisionGroup();
    }

    AZ::Crc32 CharacterController::GetColliderTag() const
    {
        return m_colliderTag;
    }

    void CharacterController::AddVelocity(const AZ::Vector3& velocity)
    {
        m_requestedVelocity += velocity;
    }

    void CharacterController::ApplyRequestedVelocity(float deltaTime)
    {
        const AZ::Vector3 oldPosition = GetBasePosition();
        const AZ::Vector3 clampedVelocity = m_requestedVelocity.GetLength() > m_maximumSpeed
            ? m_maximumSpeed * m_requestedVelocity.GetNormalized()
            : m_requestedVelocity;
        const AZ::Vector3 deltaPosition = clampedVelocity * deltaTime;

        if (m_pxController)
        {
            {
                PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
                m_pxController->move(PxMathConvert(deltaPosition), m_minimumMovementDistance, deltaTime, m_pxControllerFilters);
                if (m_shadowBody)
                {
                    m_shadowBody->SetKinematicTarget(AZ::Transform::CreateTranslation(GetBasePosition()));
                }
            }
            const AZ::Vector3 newPosition = GetBasePosition();
            m_observedVelocity = deltaTime > 0.0f ? (newPosition - oldPosition) / deltaTime : AZ::Vector3::CreateZero();
        }

        m_requestedVelocity = AZ::Vector3::CreateZero();
    }

    void CharacterController::SetRotation(const AZ::Quaternion& rotation)
    {
        if (m_shadowBody)
        {
            AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, GetBasePosition());
            m_shadowBody->SetKinematicTarget(transform);
        }
    }

    void CharacterController::AttachShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (m_shadowBody)
        {
            m_shadowBody->AddShape(shape);
        }
    }

    AZ::EntityId CharacterController::GetEntityId() const
    {
        return m_actorUserData.GetEntityId();
    }

    AzPhysics::Scene* CharacterController::GetScene()
    {
        return m_pxController ? PhysX::Utils::GetUserData(m_pxController->getScene()) : nullptr;
    }

    AZ::Transform CharacterController::GetTransform() const
    {
        return AZ::Transform::CreateTranslation(GetPosition());
    }

    void CharacterController::SetTransform(const AZ::Transform& transform)
    {
        SetBasePosition(transform.GetTranslation());
    }

    AZ::Vector3 CharacterController::GetPosition() const
    {
        return GetBasePosition();
    }

    AZ::Quaternion CharacterController::GetOrientation() const
    {
        return AZ::Quaternion::CreateIdentity();
    }

    AZ::Aabb CharacterController::GetAabb() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return AZ::Aabb::CreateNull();
        }

        // use bounding box inflation factor of 1.0f so users can control inflation themselves
        const float inflationFactor = 1.0f;
        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        return PxMathConvert(m_pxController->getActor()->getWorldBounds(inflationFactor));
    }

    AzPhysics::SceneQueryHit CharacterController::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_pxController)
        {
            if (physx::PxRigidDynamic* actor = m_pxController->getActor())
            {
                return PhysX::SceneQueryHelpers::ClosestRayHitAgainstPxRigidActor(request, actor);
            }
        }

        return AzPhysics::SceneQueryHit();
    }

    AZ::Crc32 CharacterController::GetNativeType() const
    {
        return NativeTypeIdentifiers::CharacterController;
    }

    void* CharacterController::GetNativePointer() const
    {
        return m_pxController;
    }

    // CharacterController specific
    void CharacterController::Resize(float height)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
        }

        if (height <= 0.0f)
        {
            AZ_Error("PhysX Character Controller", false, "PhysX requires controller height to be positive.");
            return;
        }

        // height needs to be adjusted due to differences between LY and PhysX definitions of capsule and box dimensions
        float adjustedHeight = height;
        {
            PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
            if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
            {
                auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
                const float radius = capsuleController->getRadius();
                if (height <= 2.0f * radius)
                {
                    AZ_Error("PhysX Character Controller", false, "Capsule height must exceed twice its radius.");
                    return;
                }
                // LY defines capsule height to include the end caps, but PhysX does not
                adjustedHeight = height - 2.0f * radius;
            }
            else
            {
                // the PhysX box controller resize function actually treats the height argument as half-height
                adjustedHeight = 0.5f * height;
            }
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        m_pxController->resize(adjustedHeight);
    }

    float CharacterController::GetHeight() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            return static_cast<physx::PxBoxController*>(m_pxController)->getHalfHeight() * 2.0f;
        }

        else if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            // PhysX capsule height refers to the length of the cylindrical section.
            // LY capsule height refers to the length including the hemispherical caps.
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            return capsuleController->getHeight() + 2.0f * capsuleController->getRadius();
        }

        AZ_Error("PhysX Character Controller", false, "Unrecognized controller shape type.");
        return 0.0f;
    }

    void CharacterController::SetHeight(float height)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }
        physx::PxControllerShapeType::Enum type;
        {
            PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
            type = m_pxController->getType();
        }
        if (type == physx::PxControllerShapeType::eBOX)
        {
            if (height <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires controller height to be positive.");
                return;
            }

            auto boxController = static_cast<physx::PxBoxController*>(m_pxController);
            PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
            boxController->setHalfHeight(0.5f * height);
        }

        else if (type == physx::PxControllerShapeType::eCAPSULE)
        {
            auto capsuleController = static_cast<physx::PxCapsuleController*>(m_pxController);
            float radius = capsuleController->getRadius();
            if (height <= 2.0f * radius)
            {
                AZ_Error("PhysX Character Controller", false, "Capsule height must exceed twice its radius.");
                return;
            }

            PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
            // PhysX capsule height refers to the length of the cylindrical section.
            // LY capsule height refers to the length including the hemispherical caps.
            capsuleController->setHeight(height - 2.0f * radius);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Unrecognized controller shape type.");
        }
    }

    float CharacterController::GetRadius() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            return static_cast<physx::PxCapsuleController*>(m_pxController)->getRadius();
        }

        AZ_Error("PhysX Character Controller", false, "Radius is only defined for capsule controllers.");
        return 0.0f;
    }

    void CharacterController::SetRadius(float radius)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eCAPSULE)
        {
            if (radius <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires radius to be positive.");
                return;
            }

            static_cast<physx::PxCapsuleController*>(m_pxController)->setRadius(radius);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Radius is only defined for capsule controllers.");
        }
    }

    float CharacterController::GetHalfSideExtent() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            return static_cast<physx::PxBoxController*>(m_pxController)->getHalfSideExtent();
        }

        AZ_Error("PhysX Character Controller", false, "Half side extent is only defined for box controllers.");
        return 0.0f;
    }

    void CharacterController::SetHalfSideExtent(float halfSideExtent)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            if (halfSideExtent <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires half side extent to be positive.");
                return;
            }

            static_cast<physx::PxBoxController*>(m_pxController)->setHalfSideExtent(halfSideExtent);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Half side extent is only defined for box controllers.");
        }
    }

    float CharacterController::GetHalfForwardExtent() const
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return 0.0f;
        }

        PHYSX_SCENE_READ_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            return static_cast<physx::PxBoxController*>(m_pxController)->getHalfForwardExtent();
        }

        AZ_Error("PhysX Character Controller", false, "Half forward extent is only defined for box controllers.");
        return 0.0f;
    }

    void CharacterController::SetHalfForwardExtent(float halfForwardExtent)
    {
        if (!m_pxController)
        {
            AZ_Error("PhysX Character Controller", false, "Invalid character controller.");
            return;
        }

        PHYSX_SCENE_WRITE_LOCK(m_pxController->getScene());
        if (m_pxController->getType() == physx::PxControllerShapeType::eBOX)
        {
            if (halfForwardExtent <= 0.0f)
            {
                AZ_Error("PhysX Character Controller", false, "PhysX requires half forward extent to be positive.");
                return;
            }

            static_cast<physx::PxBoxController*>(m_pxController)->setHalfForwardExtent(halfForwardExtent);
        }

        else
        {
            AZ_Error("PhysX Character Controller", false, "Half forward extent is only defined for box controllers.");
        }
    }

    void CharacterController::UpdateFilterLayerAndGroup(
        AzPhysics::CollisionLayer collisionLayer, AzPhysics::CollisionGroup collisionGroup)
    {
        m_filterData = PhysX::Collision::CreateFilterData(collisionLayer, collisionGroup);
        m_pxControllerFilters.mFilterData = &m_filterData;
    }

    void CharacterController::SetFilterFlags(physx::PxQueryFlags filterFlags)
    {
        m_pxControllerFilters.mFilterFlags = filterFlags;
    }
} // namespace PhysX
