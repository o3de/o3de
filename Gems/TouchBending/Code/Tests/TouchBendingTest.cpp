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
#include "TouchBending_precompiled.h"

#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>

#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/ToString.h>
#include <AzCore/Math/Plane.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Components/TransformComponent.h>

#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzFramework/Physics/Utils.h>
#include <PxPhysicsAPI.h>
#include <Physics/PhysicsTests.h>
#include <Physics/PhysicsTests.inl>
#include <PhysX/SystemComponentBus.h>
#include <Simulation/PhysicsComponent.h>

#include <AzFramework/Physics/TouchBendingBus.h>
#include <PhysX/Debug/PhysXDebugInterface.h>

static const char* const TOUCH_BENDING_TEST_WINDOW = "TouchBendingTest";

namespace TouchBending
{
    struct TouchBendingTestState
    {
        TouchBendingTestState() : m_touchBendingTriggerHandle(nullptr),
            m_spineTreeRawId(1),
            m_spineTreeId(nullptr),
            m_spineTreeArchetype(nullptr),
            m_physicalizedSkeleton(nullptr),
            m_skeletonHeight(0.0f)
        {

        }

        ~TouchBendingTestState()
        {
            Reset();
        }

        void Reset()
        {
            m_mainActor = nullptr;
            m_floor = nullptr;
            m_touchBendingTriggerHandle = nullptr;
            m_spineTreeId = nullptr;
            m_spineTreeArchetype = nullptr;
            m_physicalizedSkeleton = nullptr;
            m_skeletonHeight = 0.0f;
            m_initialJointLocations.clear();
        }

        AZStd::shared_ptr<Physics::RigidBodyStatic> m_floor;
        AZStd::shared_ptr<Physics::RigidBody> m_mainActor;

        /// This instance is the equivalent of a CVegetation render node.
        Physics::TouchBendingTriggerHandle* m_touchBendingTriggerHandle;
        int m_spineTreeRawId;
        Physics::SpineTreeIDType m_spineTreeId;
        const Physics::SpineTree* m_spineTreeArchetype;
        Physics::TouchBendingSkeletonHandle* m_physicalizedSkeleton;
        /// We will store here the height of the skeleton that will be created when the proximity trigger
        /// of m_touchBendingTriggerHandle is touched by the main actor.
        float m_skeletonHeight; 
        AZStd::vector<Physics::JointPositions> m_initialJointLocations;
    };
    
    class TouchBendingTestEnvironment
        : public AZ::Test::GemTestEnvironment
        , protected Physics::DefaultWorldBus::Handler
    {
    private:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;
        void AddGemsAndComponents() override;
        void PostCreateApplication() override;

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override
        {
            return m_defaultScene->GetLegacyWorld();
        }

        // Flag to enable pvd in tests
        static const bool EnablePvd = false;

        AZ::IO::LocalFileIO m_fileIo;

        AzPhysics::SceneHandle m_sceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::Scene* m_defaultScene;
    };

    void TouchBendingTestEnvironment::SetupEnvironment()
    {
        AZ::Test::GemTestEnvironment::SetupEnvironment();
#if !AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS

        AZ::IO::FileIOBase::SetInstance(&m_fileIo);

        if (EnablePvd)
        {
            auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
            if (debug)
            {
                debug->ConnectToPvd();
            }
        }

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfig = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfig.m_legacyId = Physics::DefaultPhysicsWorldId;
            m_sceneHandle = physicsSystem->AddScene(sceneConfig);
            m_defaultScene = physicsSystem->GetScene(m_sceneHandle);
        }

        Physics::DefaultWorldBus::Handler::BusConnect();
#endif // !AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS
    }

    void TouchBendingTestEnvironment::AddGemsAndComponents()
    {
        AddDynamicModulePaths({ "Gem.PhysX.4e08125824434932a0fe3717259caa47.v0.1.0" });
        AddComponentDescriptors({ Simulation::PhysicsComponent::CreateDescriptor() });
        AddRequiredComponents({ Simulation::PhysicsComponent::TYPEINFO_Uuid() });
    }

    void TouchBendingTestEnvironment::PostCreateApplication()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext)
        {
            Physics::ReflectionUtils::ReflectPhysicsApi(serializeContext);
        }
    }

    void TouchBendingTestEnvironment::TeardownEnvironment()
    {
#if !AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS

        Physics::DefaultWorldBus::Handler::BusDisconnect();
        

        if (EnablePvd)
        {
            auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
            if (debug)
            {
                debug->DisconnectFromPvd();
            }
        }

        //Ensure any Scenes used have been removed
        m_defaultScene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_sceneHandle);

            //going to stop and restart PhysXSystem to ensure a clean slate.
            //copy current config, if available, otherwise use default.
            const AzPhysics::SystemConfiguration config = physicsSystem->GetConfiguration() ? *(physicsSystem->GetConfiguration()) : AzPhysics::SystemConfiguration();
            physicsSystem->Shutdown(); //shutdown the system (this will clean everything up
            physicsSystem->Initialize(&config); //init to a fresh state.
        }
        m_sceneHandle = AzPhysics::InvalidSceneHandle;
#endif // !AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS
        AZ::Test::GemTestEnvironment::TeardownEnvironment();
    }

    /// This class mimics the behavior of TouchBendingCVegetationAgent
    class TouchBendingTest
        : public ::testing::Test,
          public UnitTest::TraceBusRedirector,
          public Physics::ITouchBendingCallback,
          public Physics::WorldEventHandler
    {
    protected:
        void SetUp() override
        {
            BusConnect();
        }

        void TearDown() override
        {
            BusDisconnect();
        }

        //*********************************************************************
        // Helper Methods START ***********************************************
        static inline AZ::Vector3 GetUpVectorFromQuaternion(const AZ::Quaternion& q)
        {
            const AZ::Matrix3x3 mat33 = AZ::Matrix3x3::CreateFromQuaternion(q);
            return mat33.GetBasisZ();
        }

        static inline void FillPoint(Physics::SpinePoint& spinePoint, const AZ::Vector3& spineDirection, const Physics::SpinePoint* previousSpinePoint = nullptr)
        {
            const float DEFAULT_BONE_LENGTH = 0.25f;
            const float DEFAULT_BONE_THICKNESS = 0.03f;
            const float DEFAULT_BONE_DAMPING = 0.5f;
            const float DEFAULT_BONE_STIFFNESS = 1.0f;

            spinePoint.m_thickness = DEFAULT_BONE_THICKNESS;
            spinePoint.m_damping = DEFAULT_BONE_DAMPING;
            spinePoint.m_stiffness = DEFAULT_BONE_STIFFNESS;

            if (!previousSpinePoint)
            {
                spinePoint.m_position = AZ::Vector3(0, 0, 0);
                spinePoint.m_mass = 1.0f;
            }
            else
            {
                spinePoint.m_position = previousSpinePoint->m_position + (spineDirection * DEFAULT_BONE_LENGTH);
                spinePoint.m_mass = previousSpinePoint->m_mass * 0.5f;
            }
        }
        /// Returns TRUE if the joint positions and original bone positions match.
        static bool CompareJointPositionsToOriginalArchetypePose(const Physics::JointPositions* jointPositions, AZ::u32 numberOfBones, const Physics::SpineTree* treeArchetype)
        {
            AZ::u32 boneCountInArchetype = treeArchetype->CalculateTotalNumberOfBones();
            if (numberOfBones != boneCountInArchetype)
            {
                AZ_Error(TOUCH_BENDING_TEST_WINDOW, false,
                    "Bone count mismatch. Number of bones in joints array=%u, number of bones in archetype=%u\n",
                    numberOfBones, boneCountInArchetype);
                return false;
            }

            AZ::u32 linearBoneIndex = 0;
            for (AZ::u32 spineIndex = 0; spineIndex < treeArchetype->m_spines.size(); spineIndex++)
            {
                const Physics::Spine& spine = treeArchetype->m_spines[spineIndex];
                const AZ::u32 pointCountInSpine = spine.m_points.size();
                const AZ::u32 boneCountInSpine =  pointCountInSpine - 1;
                for (AZ::u32 bottomPointIndex = 0; bottomPointIndex < boneCountInSpine; bottomPointIndex++)
                {
                    const Physics::JointPositions* jointLocations = &jointPositions[linearBoneIndex];
                    
                    const AZ::Vector3 bottomJointLocation = AZ::Vector3::CreateFromFloat3(jointLocations->m_BottomJointLocation);
                    if (!spine.m_points[bottomPointIndex].m_position.IsClose(bottomJointLocation, TouchBendingTest::s_tolerance))
                    {
                        AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "Wrong bottom joint location for spineIndex[%u], pointIndex[%u], linearBoneIndex[%u], "
                            "bottomJointLocation=%s. Was expecting joint location=%s", spineIndex, bottomPointIndex, linearBoneIndex,
                            AZ::ToString(bottomJointLocation).c_str(), AZ::ToString(spine.m_points[bottomPointIndex].m_position).c_str());
                        return false;
                    }

                    const AZ::Vector3 topJointLocation = AZ::Vector3::CreateFromFloat3(jointLocations->m_TopJointLocation);
                    const AZ::u32 topPointIndex = bottomPointIndex + 1;
                    if (!spine.m_points[topPointIndex].m_position.IsClose(topJointLocation, TouchBendingTest::s_tolerance))
                    {
                        AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "Wrong top joint location for spineIndex[%u], pointIndex[%u], linearBoneIndex[%u], "
                            "bottomJointLocation=%s. Was expecting joint location=%s", spineIndex, topPointIndex, linearBoneIndex,
                            AZ::ToString(topJointLocation).c_str(), AZ::ToString(spine.m_points[topPointIndex].m_position).c_str());
                        return false;
                    }

                    linearBoneIndex++;
                }
            }

            return true;
        }

        static inline AZ::Aabb CalculateJointsAabb(const Physics::JointPositions* jointPositions, AZ::u32 numberOfSegments)
        {
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            for (AZ::u32 boneIndex = 0; boneIndex < numberOfSegments; boneIndex++)
            {
                const AZ::Vector3 topJointLocation = AZ::Vector3::CreateFromFloat3(jointPositions[boneIndex].m_TopJointLocation);
                aabb.AddPoint(topJointLocation);
                const AZ::Vector3 bottomJointLocation = AZ::Vector3::CreateFromFloat3(jointPositions[boneIndex].m_BottomJointLocation);
                aabb.AddPoint(bottomJointLocation);
            }
            return aabb;
        }

        static inline AZ::Vector3 GetCenterOfBone(const Physics::JointPositions* jointPositions)
        {
            const AZ::Vector3 topJointLocation = AZ::Vector3::CreateFromFloat3(jointPositions->m_TopJointLocation);
            const AZ::Vector3 bottomJointLocation = AZ::Vector3::CreateFromFloat3(jointPositions->m_BottomJointLocation);
            const AZ::Vector3 centerJointLocation = bottomJointLocation + (topJointLocation - bottomJointLocation) * 0.5f;
            return centerJointLocation;
        }

        static inline float CalculateDistanceBetweenBonesAprox(const Physics::JointPositions* jointPositionsA, const Physics::JointPositions* jointPositionsB)
        {
            const AZ::Vector3 centerJointLocationA = GetCenterOfBone(jointPositionsA);
            const AZ::Vector3 centerJointLocationB = GetCenterOfBone(jointPositionsB);
            return centerJointLocationA.GetDistanceEstimate(centerJointLocationB);
        }
        // Helper Methods END *************************************************
        //*********************************************************************


        //Physics::ITouchBendingCallback START
        Physics::SpineTreeIDType CheckDistanceToCamera(const void* privateData) override
        {
            TouchBendingTestState* testState = static_cast<TouchBendingTestState*>(const_cast<void*>(privateData));
            testState->m_spineTreeId = &testState->m_spineTreeRawId;
            return testState->m_spineTreeId;
        }

        bool BuildSpineTree(const void* privateData, Physics::SpineTreeIDType spineTreeId, Physics::SpineTree& spineTreeOut) override
        {
            TouchBendingTestState* testState = static_cast<TouchBendingTestState*>(const_cast<void*>(privateData));

            if (spineTreeId != testState->m_spineTreeId)
            {
                AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "%p is not the expected spineTreeId=%p", spineTreeId, testState->m_spineTreeId);
                return false;
            }

            //We are going to create a SpineTree that looks like this one:
            // "+" is the center of the bone.
            // "o" represents a point in the spine (aka joint).
            // 
            // Y+ is towards the screen.
            //
            //                              Z+
            //                              o spine0_point3
            //                              |
            //                              + spine0_bone2
            //                              |    
            //   spine2_point2 o--+--o--+--oo
            //                              |
            //                              + spine0_bone1
            //                              | 
            //   X-                         oo--+--o--+--o spine1_point2      X+
            //                              |
            //                              + spine0_bone0
            //                              |
            //                              o spine0_point0
            //                              Z-
            
            //Let's create spine0.
            const AZ::Vector3 spine0_direction( AZ::Vector3::CreateAxisZ() );
            Physics::SpinePoint spine0_point0;
            FillPoint(spine0_point0, spine0_direction, nullptr);
            Physics::SpinePoint spine0_point1;
            FillPoint(spine0_point1, spine0_direction, &spine0_point0);
            Physics::SpinePoint spine0_point2;
            FillPoint(spine0_point2, spine0_direction, &spine0_point1);
            Physics::SpinePoint spine0_point3;
            FillPoint(spine0_point3, spine0_direction, &spine0_point2);

            Physics::Spine spine0;
            spine0.m_parentSpineIndex = -1;
            spine0.m_parentPointIndex = -1;
            spine0.m_points.set_capacity(4);
            spine0.m_points.push_back(spine0_point0);
            spine0.m_points.push_back(spine0_point1);
            spine0.m_points.push_back(spine0_point2);
            spine0.m_points.push_back(spine0_point3);

            //Let's create spine1
            const AZ::Vector3 noDirection( AZ::Vector3::CreateZero() );
            const AZ::Vector3 spine1_direction( AZ::Vector3::CreateAxisX() );
            Physics::SpinePoint spine1_point0;
            FillPoint(spine1_point0, noDirection, &spine0_point1); //This point should be at the same location of spine0_point1.
            Physics::SpinePoint spine1_point1;
            FillPoint(spine1_point1, spine1_direction, &spine1_point0);
            Physics::SpinePoint spine1_point2;
            FillPoint(spine1_point2, spine1_direction, &spine1_point1);

            Physics::Spine spine1;
            spine1.m_parentSpineIndex = 0;
            spine1.m_parentPointIndex = 1;
            spine1.m_points.set_capacity(3);
            spine1.m_points.push_back(spine1_point0);
            spine1.m_points.push_back(spine1_point1);
            spine1.m_points.push_back(spine1_point2);

            //Let's create spine2
            const AZ::Vector3 spine2_direction( -AZ::Vector3::CreateAxisX() );
            Physics::SpinePoint spine2_point0;
            FillPoint(spine2_point0, noDirection, &spine0_point2);//This point should be at the same location of spine0_point2.
            Physics::SpinePoint spine2_point1;
            FillPoint(spine2_point1, spine2_direction, &spine2_point0);
            Physics::SpinePoint spine2_point2;
            FillPoint(spine2_point2, spine2_direction, &spine2_point1);

            Physics::Spine spine2;
            spine2.m_parentSpineIndex = 0;
            spine2.m_parentPointIndex = 2;
            spine2.m_points.set_capacity(3);
            spine2.m_points.push_back(spine2_point0);
            spine2.m_points.push_back(spine2_point1);
            spine2.m_points.push_back(spine2_point2);

            //Time to put the Tree together.
            spineTreeOut.m_spineTreeId = spineTreeId;
            spineTreeOut.m_spines.set_capacity(3);
            spineTreeOut.m_spines.emplace_back(AZStd::move(spine0));
            spineTreeOut.m_spines.emplace_back(AZStd::move(spine1));
            spineTreeOut.m_spines.emplace_back(AZStd::move(spine2));

            testState->m_spineTreeArchetype = &spineTreeOut;
            return true;
        }

        bool OnPhysicalizedTouchBendingSkeleton(const void* privateData, Physics::TouchBendingSkeletonHandle* skeleton) override
        {
            TouchBendingTestState* testState = static_cast<TouchBendingTestState*>(const_cast<void*>(privateData));
            testState->m_physicalizedSkeleton = skeleton;
            return true;
        }

        //Physics::ITouchBendingCallback END

        // Physics::WorldEventHandler START
        void OnTriggerEnter([[maybe_unused]] const Physics::TriggerEvent& triggerEvent) override {}
        void OnTriggerExit([[maybe_unused]] const Physics::TriggerEvent& triggerEvent) override {}
        void OnCollisionBegin([[maybe_unused]] const Physics::CollisionEvent& collisionEvent) override {}
        void OnCollisionPersist([[maybe_unused]] const Physics::CollisionEvent& collisionEvent) override {}
        void OnCollisionEnd([[maybe_unused]] const Physics::CollisionEvent& collisionEvent) override {}
        // Physics::WorldEventHandler END

        /// The floor is defined by a Static Rigid Cubic actor of half height 0.5f,
        /// centered at 0,0, -0.5f. The world position of the center of its top face will be at 0,0,0.
        /// We will add a PhysX Rigid Dynamic actor with Spheric Shape of radius 0.5f. This actor
        /// will bend the vegetation.
        bool _01_PopulateWorld_MainActorIsSettledOnTopOfFloor(TouchBendingTestState& testState)
        {
            AZStd::shared_ptr<Physics::World> world;
            Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

            world->SetEventHandler(this);

            const float floorBodyHalfHeight = 0.5f;
            const float actorHalfHeight = 0.5f;

            AZ::Transform floorTransform = AZ::Transform::CreateTranslation(AZ::Vector3(0, 0, -floorBodyHalfHeight));
            AZStd::shared_ptr<Physics::RigidBodyStatic> floor = AddStaticFloorToWorld(world.get(), floorTransform);
            if (!floor)
            {
                AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "Failed to create the floor");
                return false;
            }

            const AZ::Vector3 initialFeetLocation = AZ::Vector3::CreateFromFloat3(TouchBendingTest::s_mainActorInitialFeetLocation);
            const float INITIAL_DISTANCE_FROM_THE_FLOOR = 0.1f;
            const AZ::Vector3 initialActorLocation = initialFeetLocation + AZ::Vector3(0, 0, actorHalfHeight + INITIAL_DISTANCE_FROM_THE_FLOOR);
            AZStd::shared_ptr<Physics::RigidBody> actor = AddSphereToWorld(world.get(), initialActorLocation);
            if (!actor)
            {
                AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "Failed to create the main actor");
                return false;
            }

            testState.m_floor = floor;
            testState.m_mainActor = actor;

            //PhysX is a deterministic physics engine. 60 steps is more than enough.
            const int MAX_SIMULATION_STEP_COUNT = 60;
            for (int i = 0; i < MAX_SIMULATION_STEP_COUNT; i++)
            {
                world->Update(TouchBendingTest::s_simulationTimeStep);
            }

            const AZ::Vector3 actorPosition = actor->GetPosition();
            const AZ::Vector3 expectedPosition = initialFeetLocation + AZ::Vector3(0, 0, actorHalfHeight);
            const bool isNearEqual = actorPosition.IsClose(expectedPosition, TouchBendingTest::s_tolerance);
            AZ_Error(TOUCH_BENDING_TEST_WINDOW, isNearEqual, "spherePosition=%s, expectedPosition=%s", AZ::ToString(actorPosition).c_str(), AZ::ToString(expectedPosition).c_str());

            return isNearEqual;
        }

        /// This method mimics the case when CVegetation::Physicalize() is called.
        bool _02_PhysicalizeTouchBendingInstance_NewInstanceIsNotNull(TouchBendingTestState& testState)
        {
            //The touch bending instance will be created at 0,0,0 with default orientation.
            AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
            //The proximity trigger box will be 1x1x1 cube.
            const float proximityBoxHalfHeight = 0.5f;
            const AZ::Vector3 touchBendingTriggerWorldLocation = AZ::Vector3::CreateFromFloat3(TouchBendingTest::s_touchBendingTriggerWorldLocation);
            const AZ::Vector3 worldLocationOfAabbCenter = touchBendingTriggerWorldLocation + AZ::Vector3(0, 0, proximityBoxHalfHeight);
            AZ::Aabb worldAabb = AZ::Aabb::CreateCenterHalfExtents(worldLocationOfAabbCenter, AZ::Vector3(proximityBoxHalfHeight, proximityBoxHalfHeight, proximityBoxHalfHeight));

            Physics::TouchBendingTriggerHandle* touchBendingInstanceHandle;
            Physics::TouchBendingBus::BroadcastResult(touchBendingInstanceHandle, &Physics::TouchBendingRequest::CreateTouchBendingTrigger, worldTransform, worldAabb,
                this, &testState);
            if (!touchBendingInstanceHandle)
            {
                AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "Failed to create Touch Bending instance handle");
                return false;
            }

            //Record the instance in the global state. Will be required by subsequent tests.
            testState.m_touchBendingTriggerHandle = touchBendingInstanceHandle;
            return true;
        }

        /// The main actor is moved with constant velocity towards
        /// the TouchBendingTrigger. The expectation is that the
        /// actor eventually touches the trigger volume and TouchBendingTest::BuildSpineTree()
        /// is called and the Physics::SpineTree is built. After the SpineTree is built,
        /// The PhysicalizedSkeleton is instantiated.
        bool _03_MoveMainActorUntilItTouchesProximityTrigger_PhysicalizedSkeletonIsInstantiated(TouchBendingTestState& testState)
        {
            Physics::RigidBody* actor = testState.m_mainActor.get();
            //Calculate the velocity vector of the main actor so it moves towards
            //the proximity trigger.
            const AZ::Vector3 touchBendingTriggerWorldLocation = AZ::Vector3::CreateFromFloat3(TouchBendingTest::s_touchBendingTriggerWorldLocation);
            const AZ::Vector3 DisplacementToTarget = touchBendingTriggerWorldLocation - actor->GetPosition();
            const AZ::Vector3 DisplacementToTargetOnZPlane(DisplacementToTarget.GetX(), DisplacementToTarget.GetY(), 0.0f);
            const AZ::Vector3 actorDirection = DisplacementToTargetOnZPlane.GetNormalized();
            const AZ::Vector3 actorVelocity = actorDirection * TouchBendingTest::s_mainActorDefaultSpeed;

            actor->SetLinearVelocity(actorVelocity);
            const AZ::Vector3 startingActorPosition = actor->GetPosition();

            //The idea is that if the sphere touches the proximity triggers (which is expected to happen) then
            //TouchBendingTest::CheckDistanceToCamera() is called and TouchBendingTestEnvironment::m_testState.m_spineTreeId
            //is set to something different than null. And eventually the PhysicalizedSkeleton will be instantiated and be ready to be added to the scene.
            AZStd::shared_ptr<Physics::World> world;
            Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);

            //Usually it only takes 45 iterations for this loop to complete.
            //We do it as a while loop because the PhysicalizedSkeleton is built on an AZ::Job.
            //On machines that are being overloaded this loop can take more iterations than 45 to get the AZ::Job to complete.
            //Worst case scenario the AZ::Job has around 20 minutes or so to complete before the UnitTest infrastucture
            //shutsdown this test altogether.
            while ((volatile Physics::TouchBendingSkeletonHandle*)(testState.m_physicalizedSkeleton) == nullptr)
            {
                world->Update(TouchBendingTest::s_simulationTimeStep);
                //PhysX default configuration applies friction. Setting the velocity each loop helps keep it constant.
                actor->SetLinearVelocity(actorVelocity);
                //testState.m_physicalizedSkeleton gets a valid value on an AZ::Job.
                //By sleeping 0 we increase the chances the AZ::Job gets its chance to run.
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(0));
            }

            //The actor has definitely touched the trigger box. Even though NVIDIA PhysX is deterministic, the fact that the PhysicalizedSkeleton is built on an AZ::Job this can cause
            //the actor to move to a non-deterministic location depending on thread scheduling policies, CPU load, etc, etc.
            //Let's set the actor back to its original position:
            const AZ::Vector3 actorPosition = actor->GetPosition();
            const AZ::Vector3 initialFeetLocation = AZ::Vector3::CreateFromFloat3(TouchBendingTest::s_mainActorInitialFeetLocation);
            const AZ::Vector3 newActorPosition(initialFeetLocation.GetX(), initialFeetLocation.GetY(), actorPosition.GetZ());
            AZ::Transform actorTransform = actor->GetTransform();
            actorTransform.SetTranslation(newActorPosition);
            actor->SetTransform(actorTransform);

            return true;
        }

        /// When the PhysicalizedSkeleton becomes visible it will be added to the PhysX World.
        /// We will read the starting joint positions
        /// and compare it against the SpineTreeArchtetype.
        bool _04_SetSkeletonVisible_CheckStartingPose(TouchBendingTestState& testState)
        {
            AZ::u32 boneCount = 0;
            AZ::u32 touchCount = 0;
            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::SetTouchBendingSkeletonVisibility, testState.m_physicalizedSkeleton, true, boneCount, touchCount);

            //No need to check for "touchCount". it can be 0, or 1 depending on the current compute load of the machine
            //where this test is running from. What truly matters (and is guaranteed) is that this function is called after the proximity box was touched
            //at least once.

            const AZ::u32 expectedMinimumBoneCount = 1;
            if (boneCount < expectedMinimumBoneCount)
            {
                AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "boneCount=%u, was expecting a minimum value of %u", boneCount, expectedMinimumBoneCount);
                return false;
            }

            //At this point we will make sure the main actor doesn't move at all but we will run a few simulation cycles.
            //The PhysicalizedSkeleton should not move at all and we will compare the joint positions against the original bone positions
            //in the archetype.
            Physics::RigidBody* actor = testState.m_mainActor.get();
            actor->SetLinearVelocity(AZ::Vector3(0, 0, 0));

            AZStd::shared_ptr<Physics::World> world;
            Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
            const int MAX_SIMULATION_STEPS = 10;
            for (int i = 0; i < MAX_SIMULATION_STEPS; i++)
            {
                world->Update(TouchBendingTest::s_simulationTimeStep);
            }

            testState.m_initialJointLocations.resize_no_construct(boneCount);
            memset(&testState.m_initialJointLocations[0], 0, boneCount * sizeof(Physics::JointPositions));
            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::ReadJointPositionsOfSkeleton,
                testState.m_physicalizedSkeleton, &testState.m_initialJointLocations[0]);

            bool success = CompareJointPositionsToOriginalArchetypePose(&testState.m_initialJointLocations[0],
                boneCount, testState.m_spineTreeArchetype);

            AZ::Aabb jointsAabb = CalculateJointsAabb(&testState.m_initialJointLocations[0], boneCount);
            testState.m_skeletonHeight = jointsAabb.GetExtents().GetZ();

            return success;
        }

        /// In this test the main actor is moved towards the PhysicalizedSkeleton with constant velocity.
        /// Once the main actor is walking on top of the PhysicalizedSkeleton all of the bones of the skeleton are supposed
        /// to be crushed against the floor. We measure the Z distance to the floor for each bone and
        /// expect it to be less than MAX_DISTANCE_FROM_FLOOR.
        bool _05_MoveMainActorUntilItReachesTheLocationOfTheSkeleton_AllJointsOfTheSkeletonAreCloseToTheFloor(TouchBendingTestState& testState)
        {
            Physics::RigidBody* actor = testState.m_mainActor.get();
            //Calculate the velocity vector of the main actor so it moves towards
            //the proximity trigger.
            const AZ::Vector3 touchBendingTriggerWorldLocation = AZ::Vector3::CreateFromFloat3(TouchBendingTest::s_touchBendingTriggerWorldLocation);
            const AZ::Vector3 DisplacementToTarget = touchBendingTriggerWorldLocation - actor->GetPosition();
            AZ::Vector3 DisplacementToTargetOnZPlane(DisplacementToTarget.GetX(), DisplacementToTarget.GetY(), 0.0f);
            const AZ::Vector3 actorDirection = DisplacementToTargetOnZPlane.GetNormalized();
            const AZ::Vector3 actorVelocity = actorDirection * TouchBendingTest::s_mainActorDefaultSpeed;

            //Add the length of the PhysicalizedSkeleton in the moving direction so the main actor ends up stomping on top
            //of the PhysicalizedSkeleton and all its bones are bent touching the floor.
            DisplacementToTargetOnZPlane += actorDirection * testState.m_skeletonHeight;

            const AZ::Vector3 startingActorPosition = actor->GetPosition();

            // Calculate the number of simulation steps required for the actor to reach the target.
            const float distanceToTarget = DisplacementToTargetOnZPlane.GetLength();
            const float timeRequiredToReachTarget = distanceToTarget / TouchBendingTest::s_mainActorDefaultSpeed;
            const int stepCountToReachTarget = (int)(timeRequiredToReachTarget / TouchBendingTest::s_simulationTimeStep);

            AZ::u32 boneCount = 0;
            AZ::u32 touchCount = 0;
            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::SetTouchBendingSkeletonVisibility, testState.m_physicalizedSkeleton, true, boneCount, touchCount);

            actor->SetLinearVelocity(actorVelocity);
            AZStd::shared_ptr<Physics::World> world;
            Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
            for (int i = 0; i <= stepCountToReachTarget; i++)
            {
                world->Update(TouchBendingTest::s_simulationTimeStep);
                //PhysX default configuration applies friction. Setting the velocity each loop helps keep it constant.
                actor->SetLinearVelocity(actorVelocity);
            }

            // The main actor should be on top of the skeleton, crushing it against the floor.
            // Let's make sure all the joints are close to the floor
            AZStd::vector<Physics::JointPositions> jointPositions;
            jointPositions.resize_no_construct(boneCount);
            memset(&jointPositions[0], 0, boneCount * sizeof(Physics::JointPositions));

            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::ReadJointPositionsOfSkeleton,
                testState.m_physicalizedSkeleton, &jointPositions[0]);

            const float MAX_DISTANCE_FROM_FLOOR = 0.1f;
            const AZ::Plane collisionPlane = AZ::Plane::CreateFromNormalAndPoint(-actorDirection, touchBendingTriggerWorldLocation);
            for (AZ::u32 boneIndex = 0; boneIndex < boneCount; boneIndex++)
            {
                const Physics::JointPositions* jointPositionsOfSegment = &jointPositions[boneIndex];
                const AZ::Vector3 boneCenter = GetCenterOfBone(jointPositionsOfSegment);
                if (boneCenter.GetZ() > MAX_DISTANCE_FROM_FLOOR)
                {
                    AZ_Error(TOUCH_BENDING_TEST_WINDOW, false,
                        "boneIndex[%u] failed distance from floor=%f, max distance=%f",
                        boneIndex, boneCenter.GetZ(), MAX_DISTANCE_FROM_FLOOR);
                    return false;
                }
                const float distanceFromCollisionPlane = collisionPlane.GetPointDist(boneCenter);
                if (distanceFromCollisionPlane > 0.0f)
                {
                    AZ_Error(TOUCH_BENDING_TEST_WINDOW, false, "center of bone[%u] did not bend behind the collision plane."
                        "Bone position=%s, distance to plane = %f. Plane[normal=%s, point=%s]\n",
                        boneIndex, AZ::ToString(boneCenter).c_str(), distanceFromCollisionPlane,
                        AZ::ToString(collisionPlane.GetNormal()).c_str(),  AZ::ToString(touchBendingTriggerWorldLocation).c_str());
                    return false;
                }
            }

            return true;
        }

        /// The main actor keeps moving away from the Tree. Because the Tree is configure with 1.0f Stiffness (maximum spring value)
        /// we expect all its bones to return to its starting position.
        bool _06_MoveMainActorAwaysFromTheSkeleton_SkeletonShouldSpringBackToStartingPose(TouchBendingTestState& testState)
        {
            Physics::RigidBody* actor = testState.m_mainActor.get();
            const AZ::Vector3 actorVelocity = actor->GetLinearVelocity();

            const AZ::u32 boneCount = testState.m_initialJointLocations.size();
            AZStd::vector<Physics::JointPositions> jointPositions;
            jointPositions.resize_no_construct(boneCount);
            memset(&jointPositions[0], 0, boneCount * sizeof(Physics::JointPositions));

            //Let's make sure the actor keeps moving while increasing its distance from the skeleton.
            AZStd::bitset<TouchBendingTest::s_maximumBoneCountPerSkeleton> bonesThatReturnedToInitialPosition;
            AZ::u32 countOfBonesPendingToReturnToInitialPosition = boneCount;
            AZStd::shared_ptr<Physics::World> world;
            Physics::DefaultWorldBus::BroadcastResult(world, &Physics::DefaultWorldRequests::GetDefaultWorld);
            const int MAX_SIMULATION_STEP_COUNT = 240;
            for (int i = 0; (i <= MAX_SIMULATION_STEP_COUNT) && (countOfBonesPendingToReturnToInitialPosition > 0); i++)
            {
                world->Update(TouchBendingTest::s_simulationTimeStep);

                Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::ReadJointPositionsOfSkeleton,
                    testState.m_physicalizedSkeleton, &jointPositions[0]);

                for (AZ::u32 boneIndex = 0; boneIndex < boneCount; boneIndex++)
                {
                    if (bonesThatReturnedToInitialPosition[boneIndex])
                    {
                        continue;
                    }
                    const Physics::JointPositions* jointLocations = &jointPositions[boneIndex];
                    const float distanceBetweenBones = CalculateDistanceBetweenBonesAprox(jointLocations, &testState.m_initialJointLocations[boneIndex]);
                    const float MAX_DISTANCE_BETWEEN_A_BONE_AND_ITS_DEFAULT_POSITION = 0.1f;
                    if (distanceBetweenBones < MAX_DISTANCE_BETWEEN_A_BONE_AND_ITS_DEFAULT_POSITION)
                    {
                        bonesThatReturnedToInitialPosition.set(boneIndex);
                        countOfBonesPendingToReturnToInitialPosition--;
                    }

                }
                //PhysX default configuration applies friction. Setting the velocity each loop helps keep it constant.
                actor->SetLinearVelocity(actorVelocity);
            }

            //We don't need the skeleton anymore and it is our responsiblity to remove it.
            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::DephysicalizeTouchBendingSkeleton,
                testState.m_physicalizedSkeleton);
            testState.m_physicalizedSkeleton = nullptr;

            // Make sure we also clean up the touchbending trigger handle.
            // This also tests for regressions of LY-111942 - deletion of touch bending triggers should not
            // trigger an error about multithreaded scene usage in debug builds.
            Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::DeleteTouchBendingTrigger, testState.m_touchBendingTriggerHandle);
            testState.m_touchBendingTriggerHandle = nullptr;

            return (countOfBonesPendingToReturnToInitialPosition == 0);
        }

        ///Used for floating point equality.
        static constexpr float s_tolerance = 1e-3f;
        static constexpr float s_simulationTimeStep = 1.0f / 60.0f;
        static constexpr AZ::u32 s_maximumBoneCountPerSkeleton = 128;

        static constexpr float s_mainActorDefaultSpeed = 3.0f;

        /// World Location of the actor's "feet". This is where the actor touches the floor.
        /// The center of mass is some Z value above this location.
        static constexpr float s_mainActorInitialFeetLocation[3] = { 0.0f, 3.0f, 0.0f };

        /// The main actor will move with constant velocity towards
        /// this location.
        static constexpr float s_touchBendingTriggerWorldLocation[3] = { 0.0f, 0.0f, 0.0f };
    }; //class TouchBendingTest

    /// For development and debug purposes it is possible that the a developer
    /// modifies Physics::IsTouchBendingEnabled() so it returns false and make
    /// the code to default using Cry Physics based touch bending. This first
    /// sanity check makes sure it is set back to true.
    TEST_F(TouchBendingTest, TouchBending_SanityCheck_GemIsEnabled)
    {
        ASSERT_TRUE(Physics::IsTouchBendingEnabled());
    }


// [SPEC-5222] Disabled the test even when the trait isn't enabled, because it was sporadically failing even on platforms
// where it generally works.
#if AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS
    TEST_F(TouchBendingTest, DISABLED_TouchBending_CreateAndPopulateWorld_TheSkeletonBendsWhenTouchedByMainActor)
#else
    TEST_F(TouchBendingTest, DISABLED_TouchBending_CreateAndPopulateWorld_TheSkeletonBendsWhenTouchedByMainActor)
#endif // AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS
    {
        TouchBendingTestState testState;

        //The reason the following methods are not TEST_F by themselves is because
        //each test depends on the previous test to be successful. It is important
        //NOT to give the option to the user to scan/find these methods as tests because
        //they won't PASS when ran in isolation.
        ASSERT_TRUE(_01_PopulateWorld_MainActorIsSettledOnTopOfFloor(testState));
        ASSERT_TRUE(_02_PhysicalizeTouchBendingInstance_NewInstanceIsNotNull(testState));
        ASSERT_TRUE(_03_MoveMainActorUntilItTouchesProximityTrigger_PhysicalizedSkeletonIsInstantiated(testState));
        ASSERT_TRUE(_04_SetSkeletonVisible_CheckStartingPose(testState));
        ASSERT_TRUE(_05_MoveMainActorUntilItReachesTheLocationOfTheSkeleton_AllJointsOfTheSkeletonAreCloseToTheFloor(testState));
        ASSERT_TRUE(_06_MoveMainActorAwaysFromTheSkeleton_SkeletonShouldSpringBackToStartingPose(testState));
    }

#if AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS
    TEST_F(TouchBendingTest, DISABLED_TouchBending_LY111977_CreateAndPopulateWorld_UnloadLevelToDestroyWorld)
#else
    TEST_F(TouchBendingTest, TouchBending_LY111977_CreateAndPopulateWorld_UnloadLevelToDestroyWorld)
#endif // AZ_TRAIT_DISABLE_FAILED_TOUCH_BENDING_TESTS
    {
        // While this test runs nearly the same steps as the previous test, it triggers an "unload level" event
        // prior to cleanup.  The point of this test is to regress bug LY-111977 - destruction of physicalized
        // skeletons after an "unload level" event should not trigger errors about multithreaded scene usage
        // in debug builds.

        TouchBendingTestState testState;

        Simulation::PhysicsComponent* physicsComponent = nullptr;

        // Slightly non-obvious way to locate our physics component.  We use this method because the test environment
        // we're using creates a separate non-exposed entity to put our Gem's "system" components on, instead of
        // the actual system component.  Without the entity ID, this seemed like the easiest way to find it.

        // NOTE: The reason we need a direct pointer to the PhysicsComponent is to trigger the "unload level" event in isolation
        // on that component.  If we try to mock a full CrySystemEventDispatcher, events will also get set to the PhysX
        // components, which will perform additional work that we don't want in a unit/integration test, like
        // loading default configurations out of files, etc.  Instead, we grab the pointer to the touch bending component
        // to let us direct-call the event when necessary.
        Physics::TouchBendingBus::EnumerateHandlers([&physicsComponent](Physics::TouchBendingRequest* handler) -> bool
        {
            Simulation::PhysicsComponent* component = azrtti_cast<Simulation::PhysicsComponent*>(handler);
            if (component)
            {
                physicsComponent = component;
            }
            return true;
        });

        EXPECT_TRUE(physicsComponent != nullptr);

        // Perform the same setup steps as before - these will create our physics world, touch bending triggers,
        // physicalized skeletons, etc.
        ASSERT_TRUE(_01_PopulateWorld_MainActorIsSettledOnTopOfFloor(testState));
        ASSERT_TRUE(_02_PhysicalizeTouchBendingInstance_NewInstanceIsNotNull(testState));
        ASSERT_TRUE(_03_MoveMainActorUntilItTouchesProximityTrigger_PhysicalizedSkeletonIsInstantiated(testState));
        ASSERT_TRUE(_04_SetSkeletonVisible_CheckStartingPose(testState));
        ASSERT_TRUE(_05_MoveMainActorUntilItReachesTheLocationOfTheSkeleton_AllJointsOfTheSkeletonAreCloseToTheFloor(testState));

        // Direct-call the event handler on the touchbending component to trigger the "level unload" events.
        // This will cause the PhysicsComponent to clear its world state prior to asset cleanup, which is
        // required to test for regressions of LY-111977.
        if (physicsComponent)
        {
            physicsComponent->OnSystemEvent(ESYSTEM_EVENT_LEVEL_UNLOAD, 0, 0);
            physicsComponent->OnSystemEvent(ESYSTEM_EVENT_LEVEL_POST_UNLOAD, 0, 0);
        }

        // Skip step 6 of the previous test, and just perform the cleanup, as that's all that is needed to validate
        // the level unload regression.
        Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::DephysicalizeTouchBendingSkeleton,
            testState.m_physicalizedSkeleton);
        testState.m_physicalizedSkeleton = nullptr;

        Physics::TouchBendingBus::Broadcast(&Physics::TouchBendingRequest::DeleteTouchBendingTrigger, testState.m_touchBendingTriggerHandle);
        testState.m_touchBendingTriggerHandle = nullptr;
    }


}; //namespace TouchBending

AZ_UNIT_TEST_HOOK(new TouchBending::TouchBendingTestEnvironment);
