/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/parallel/thread.h>

#include <Tests/PhysXGenericTestFixture.h>
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

#ifdef PHYSX_ENABLE_MULTI_THREADING

//enable this define to enable some logs for debugging
//#define PHYSX_MT_DEBUG_LOGS
#ifdef PHYSX_MT_DEBUG_LOGS
#define Log_Help(window, message, ...) AZ_Printf(window, message, __VA_ARGS__);
#else
#define Log_Help(window, message, ...) ((void)0);
#endif //PHYSX_MT_DEBUG_LOGS

namespace PhysX
{
    namespace Constants
    {
        static const int NumThreads = 50; //number of threads to create and use for the tests

        //Entities to raycast / shapecast / Overlap against
        static const AZ::Vector3 BoxDimensions = AZ::Vector3::CreateOne();
        static const int NumBoxes = 18;
        AZStd::array<AZ::Vector3, NumBoxes> BoxPositions = {
            AZ::Vector3( 1000.0f,  1000.0f,     0.0f),
            AZ::Vector3(-1000.0f, -1000.0f,     0.0f),
            AZ::Vector3( 1000.0f, -1000.0f,     0.0f),
            AZ::Vector3(-1000.0f,  1000.0f,     0.0f),
            AZ::Vector3( 1000.0f,     0.0f,  1000.0f),
            AZ::Vector3(-1000.0f,     0.0f, -1000.0f),
            AZ::Vector3( 1000.0f,     0.0f, -1000.0f),
            AZ::Vector3(-1000.0f,     0.0f,  1000.0f),
            AZ::Vector3(    0.0f,    10.0f,    10.0f),
            AZ::Vector3(    0.0f,   -10.0f,   -10.0f),
            AZ::Vector3(    0.0f,   -10.0f,    10.0f),
            AZ::Vector3(    0.0f,    10.0f,   -10.0f),
            AZ::Vector3( 100.0f,      0.0f,     0.0f),
            AZ::Vector3(-100.0f,      0.0f,     0.0f),
            AZ::Vector3(  0.0f,     100.0f,     0.0f),
            AZ::Vector3(  0.0f,    -100.0f,     0.0f),
            AZ::Vector3(  0.0f,       0.0f,    10.0f),
            AZ::Vector3(  0.0f,       0.0f,   -10.0f)
        };

        static const float ShpereShapeRadius = 2.0f;
    }

    namespace
    {
        void UpdateTestSceneOverTime(AzPhysics::Scene* scene, int updateTimeLimitMilliseconds)
        {
            const AZStd::chrono::milliseconds updateTimeLimit = AZStd::chrono::milliseconds(updateTimeLimitMilliseconds);
            AZStd::chrono::milliseconds totalTime = AZStd::chrono::milliseconds(0);
            do
            {
                AZStd::chrono::steady_clock::time_point startTime = AZStd::chrono::steady_clock::now();
                PhysX::TestUtils::UpdateScene(scene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
                totalTime += AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::steady_clock::now() - startTime);
            } while (totalTime < updateTimeLimit);
        }
    }

    class PhysXMultithreadingTest
        : public PhysX::GenericPhysicsInterfaceTest
        , public ::testing::WithParamInterface<int>
    {
    public:
        virtual void SetUp() override
        {
            PhysX::GenericPhysicsInterfaceTest::SetUpInternal();

            //create some boxes
            for (int i = 0; i < Constants::NumBoxes; i++)
            {
                EntityPtr newEntity = TestUtils::CreateBoxEntity(m_testSceneHandle, Constants::BoxPositions[i], Constants::BoxDimensions);
                m_boxes.emplace_back(newEntity);

                //disable gravity so they don't move
                Physics::RigidBodyRequestBus::Event(newEntity->GetId(), &Physics::RigidBodyRequestBus::Events::SetGravityEnabled, false);
            }
        }

        virtual void TearDown() override
        {
            PhysX::GenericPhysicsInterfaceTest::TearDownInternal();
        }

        AZStd::vector<EntityPtr> m_boxes;
    };

    template<typename RequestType, typename ResultType>
    struct SceneQueryBase
    {
        SceneQueryBase(const AZStd::thread_desc& threadDesc, const RequestType& request, AzPhysics::SceneHandle sceneHandle)
            : m_threadDesc(threadDesc)
            , m_request(request)
            , m_sceneHandle(sceneHandle)
        {
            m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        }

        virtual ~SceneQueryBase() = default;

        void Start(int waitTimeMilliseconds)
        {
            m_waitTimeMilliseconds = waitTimeMilliseconds;
            m_thread = AZStd::thread(m_threadDesc, AZStd::bind(&SceneQueryBase::Tick, this));
        }

        void Join()
        {
            m_thread.join();
        }

        ResultType m_result;

        const RequestType& GetRequest() const
        {
            return m_request;
        }

    private:
        void Tick()
        {
            Log_Help(m_threadDesc.m_name, "Thread %d - sleeping for %dms\n", AZStd::this_thread::get_id(), m_waitTimeMilliseconds);
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(m_waitTimeMilliseconds));
            Log_Help(m_threadDesc.m_name, "Thread %d - running cast\n", AZStd::this_thread::get_id());

            RunRequest();

            Log_Help(m_threadDesc.m_name, "Thread %d - complete - time %dus\n", AZStd::this_thread::get_id(), exeTimeUS.count());
        }

    protected:
        virtual void RunRequest() = 0;

        AZStd::thread m_thread;
        AZStd::thread_desc m_threadDesc;
        int m_waitTimeMilliseconds;
        RequestType m_request;
        AzPhysics::SceneInterface* m_sceneInterface;
        AzPhysics::SceneHandle m_sceneHandle;
    };

    struct RayCaster
        : public SceneQueryBase<AzPhysics::RayCastRequest, AzPhysics::SceneQueryHits>
    {
        RayCaster(const AZStd::thread_desc& threadDesc, const AzPhysics::RayCastRequest& request, AzPhysics::SceneHandle sceneHandle)
            : SceneQueryBase(threadDesc, request, sceneHandle)
        {

        }

    private:
        void RunRequest() override
        {
            m_result = m_sceneInterface->QueryScene(m_sceneHandle, &m_request);
        }
    };

    TEST_P(PhysXMultithreadingTest, RaycastsQueryFromParallelThreads)
    {
        const int seed = GetParam();

        //raycast thread pool
        AZStd::vector<AZStd::unique_ptr<RayCaster>> rayCasters;

        //common request data
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3::CreateZero();
        request.m_distance = 2000.0f;

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "RQFPThreads";  // RaycastsQueryFromParallelThreads

        //create the raycasts
        for (int i = 0; i < Constants::NumThreads; i++)
        {
            //pick a box to raycast against
            const int boxTargetIdx = i % m_boxes.size();
            request.m_direction = Constants::BoxPositions[boxTargetIdx].GetNormalized();

            rayCasters.emplace_back(AZStd::make_unique<RayCaster>(threadDesc, request, m_testSceneHandle));
        }

        //start all threads
        AZ::SimpleLcgRandom random(seed); //constant seed to have consistency.
        for (auto& caster : rayCasters)
        {
            const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
            caster->Start(waitTimeMS);
        }

        //update the world
        Log_Help("RaycastsQueryFromParallelThreads", "Start world Update\n");
        UpdateTestSceneOverTime(m_defaultScene, 500);
        Log_Help("RaycastsQueryFromParallelThreads", "End world Update\n");

        //each request should have completed, join to be sure. and each request should have a result.
        int i = 0;
        for (auto& caster : rayCasters)
        {
            caster->Join();

            const int boxTargetIdx = i % m_boxes.size();
            EXPECT_TRUE(caster->m_result);
            EXPECT_TRUE(caster->m_result.m_hits[0].m_entityId == m_boxes[boxTargetIdx]->GetId());

            caster.release();
            i++;
        }
        rayCasters.clear();
    }

    struct RayCasterMultiple
        : public SceneQueryBase<AzPhysics::RayCastRequest, AzPhysics::SceneQueryHits>
    {
        RayCasterMultiple(const AZStd::thread_desc& threadDesc, const AzPhysics::RayCastRequest& request, AzPhysics::SceneHandle sceneHandle)
            : SceneQueryBase(threadDesc, request, sceneHandle)
        {

        }

    private:
        void RunRequest() override
        {
            m_request.m_reportMultipleHits = true;
            m_result = m_sceneInterface->QueryScene(m_sceneHandle, &m_request);
        }
    };

    TEST_P(PhysXMultithreadingTest, RaycastMultiplesQueryFromParallelThreads)
    {
        const int seed = GetParam();

        //raycast thread pool
        AZStd::vector<AZStd::unique_ptr<RayCasterMultiple>> rayCasters;

        //common request data
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3::CreateZero();
        request.m_distance = 2000.0f;

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "RMQFPThreads";  // RaycastMultiplesQueryFromParallelThreads

        //create the raycasts
        for (int i = 0; i < Constants::NumThreads; i++)
        {
            //pick a box to raycast against
            const int boxTargetIdx = i % m_boxes.size();
            request.m_direction = Constants::BoxPositions[boxTargetIdx].GetNormalized();

            rayCasters.emplace_back(AZStd::make_unique<RayCasterMultiple>(threadDesc, request, m_testSceneHandle));
        }

        //start all threads
        AZ::SimpleLcgRandom random(seed);
        for (auto& caster : rayCasters)
        {
            const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
            caster->Start(waitTimeMS);
        }

        //update the world
        Log_Help("RaycastMultiplesQueryFromParallelThreads", "Start world Update\n");
        UpdateTestSceneOverTime(m_defaultScene, 500);
        Log_Help("RaycastMultiplesQueryFromParallelThreads", "End world Update\n");

        //each request should have completed, join to be sure. and each request should have a result.
        int i = 0;
        for (auto& caster : rayCasters)
        {
            caster->Join();
            //we should have some results
            EXPECT_TRUE(caster->m_result);

            //check the list of result for the target
            bool targetInList = false;
            const int boxTargetIdx = i % m_boxes.size();
            for (auto& hit : caster->m_result.m_hits)
            {
                if (hit && hit.m_entityId == m_boxes[boxTargetIdx]->GetId())
                {
                    targetInList = true;
                    break;
                }
            }
            EXPECT_TRUE(targetInList);

            caster.release();
            i++;
        }
        rayCasters.clear();
    }

     struct ShapeCaster
         : public SceneQueryBase<AzPhysics::ShapeCastRequest, AzPhysics::SceneQueryHits>
     {
         ShapeCaster(const AZStd::thread_desc& threadDesc, const AzPhysics::ShapeCastRequest& request, AzPhysics::SceneHandle sceneHandle)
             : SceneQueryBase(threadDesc, request, sceneHandle)
         {

         }

     private:
         void RunRequest() override
         {
             m_result = m_sceneInterface->QueryScene(m_sceneHandle, &m_request);
         }
     };

     TEST_P(PhysXMultithreadingTest, ShapeCastsQueryFromParallelThreads)
     {
         const int seed = GetParam();

         //shapecast thread pool
         AZStd::vector<AZStd::unique_ptr<ShapeCaster>> shapeCasters;

         //common request data
         AzPhysics::ShapeCastRequest request;
         request.m_start = AZ::Transform::CreateIdentity();
         request.m_distance = 2000.0f;
         request.m_shapeConfiguration = AZStd::make_shared<Physics::SphereShapeConfiguration>(Constants::ShpereShapeRadius);

         AZStd::thread_desc threadDesc;
         threadDesc.m_name = "SCQFPThreads";  // ShapeCastsQueryFromParallelThreads

         //create the raycasts
         for (int i = 0; i < Constants::NumThreads; i++)
         {
             //pick a box to raycast against
             const int boxTargetIdx = i % m_boxes.size();
             request.m_direction = Constants::BoxPositions[boxTargetIdx].GetNormalized();

             shapeCasters.emplace_back(AZStd::make_unique<ShapeCaster>(threadDesc, request, m_testSceneHandle));
         }

         //start all threads
         AZ::SimpleLcgRandom random(seed); //constant seed to have consistency.
         for (auto& caster : shapeCasters)
         {
             const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
             caster->Start(waitTimeMS);
         }

         //update the world
         Log_Help("ShapeCastsQueryFromParallelThreads", "Start world Update\n");
         UpdateTestSceneOverTime(m_defaultScene, 500);
         Log_Help("ShapeCastsQueryFromParallelThreads", "End world Update\n");

         //each request should have completed, join to be sure. and each request should have a result.
         int i = 0;
         for (auto& caster : shapeCasters)
         {
             caster->Join();

             const int boxTargetIdx = i % m_boxes.size();
             EXPECT_TRUE(caster->m_result);
             EXPECT_EQ(caster->m_result.m_hits.size(), 1);
             EXPECT_TRUE(caster->m_result.m_hits[0].m_entityId == m_boxes[boxTargetIdx]->GetId());

             caster.release();
             i++;
         }
         shapeCasters.clear();
     }

     struct ShapeCasterMultiple
         : public SceneQueryBase<AzPhysics::ShapeCastRequest, AzPhysics::SceneQueryHits>
     {
         ShapeCasterMultiple(const AZStd::thread_desc& threadDesc, const AzPhysics::ShapeCastRequest& request, AzPhysics::SceneHandle sceneHandle)
             : SceneQueryBase(threadDesc, request, sceneHandle)
         {

         }

     private:
         void RunRequest() override
         {
             m_request.m_reportMultipleHits = true;
             m_result = m_sceneInterface->QueryScene(m_sceneHandle, &m_request);
         }
     };

     TEST_P(PhysXMultithreadingTest, ShapeCastMultiplesQueryFromParallelThreads)
     {
         const int seed = GetParam();

         //shapecast thread pool
         AZStd::vector<AZStd::unique_ptr<ShapeCasterMultiple>> shapeCasters;

         //common request data
         AzPhysics::ShapeCastRequest request;
         request.m_distance = 2000.0f;
         request.m_shapeConfiguration = AZStd::make_shared<Physics::SphereShapeConfiguration>(Constants::ShpereShapeRadius);

         AZStd::thread_desc threadDesc;
         threadDesc.m_name = "SCMQFPThreads";  // ShapeCastMultiplesQueryFromParallelThreads

         //create the shape casts
         for (int i = 0; i < Constants::NumThreads; i++)
         {
             //pick a box
             const int boxTargetIdx = i % m_boxes.size();
             request.m_direction = Constants::BoxPositions[boxTargetIdx].GetNormalized();

             shapeCasters.emplace_back(AZStd::make_unique<ShapeCasterMultiple>(threadDesc, request, m_testSceneHandle));
         }

         //start all threads
         AZ::SimpleLcgRandom random(seed); //constant seed to have consistency.
         for (auto& caster : shapeCasters)
         {
             const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
             caster->Start(waitTimeMS);
         }

         //update the world
         Log_Help("ShapeCastMultiplesQueryFromParallelThreads", "Start world Update\n");
         UpdateTestSceneOverTime(m_defaultScene, 500);
         Log_Help("ShapeCastMultiplesQueryFromParallelThreads", "End world Update\n");

         //each request should have completed, join to be sure. and each request should have a result.
         int i = 0;
         for (auto& caster : shapeCasters)
         {
             caster->Join();
             //we should have some results
             EXPECT_TRUE(caster->m_result);

             //check the list of result for the target
             bool targetInList = false;
             const int boxTargetIdx = i % m_boxes.size();
             for (auto& hit : caster->m_result.m_hits)
             {
                 if (hit && hit.m_entityId == m_boxes[boxTargetIdx]->GetId())
                 {
                     targetInList = true;
                     break;
                 }
             }
             EXPECT_TRUE(targetInList);

             caster.release();
             i++;
         }
         shapeCasters.clear();
     }

     struct OverlapQuery
         : public SceneQueryBase<AzPhysics::OverlapRequest, AzPhysics::SceneQueryHits>
     {
         OverlapQuery(const AZStd::thread_desc& threadDesc, const AzPhysics::OverlapRequest& request, AzPhysics::SceneHandle sceneHandle)
             : SceneQueryBase(threadDesc, request, sceneHandle)
         {

         }

     private:
         void RunRequest() override
         {
             m_result = m_sceneInterface->QueryScene(m_sceneHandle, &m_request);
         }
     };

     TEST_P(PhysXMultithreadingTest, OverlapQueryFromParallelThreads)
     {
         const int seed = GetParam();

         //Overlap thread pool
         AZStd::vector<AZStd::unique_ptr<OverlapQuery>> overlapQuery;

         //common request data
         AzPhysics::OverlapRequest request;
         request.m_shapeConfiguration = AZStd::make_shared<Physics::SphereShapeConfiguration>(Constants::ShpereShapeRadius);

         AZStd::thread_desc threadDesc;
         threadDesc.m_name = "OQFPThreads";  // OverlapQueryFromParallelThreads

         //create the overlap request
         for (int i = 0; i < Constants::NumThreads; i++)
         {
             //pick a box
             const int boxTargetIdx = i % m_boxes.size();
             request.m_pose = AZ::Transform::CreateTranslation(Constants::BoxPositions[boxTargetIdx]);

             overlapQuery.emplace_back(AZStd::make_unique<OverlapQuery>(threadDesc, request, m_testSceneHandle));
         }

         //start all threads
         AZ::SimpleLcgRandom random(seed); //constant seed to have consistency.
         for (auto& caster : overlapQuery)
         {
             const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
             caster->Start(waitTimeMS);
         }

         //update the world
         Log_Help("OverlapQueryFromParallelThreads", "Start world Update\n");
         UpdateTestSceneOverTime(m_defaultScene, 500);
         Log_Help("OverlapQueryFromParallelThreads", "End world Update\n");

         //each request should have completed, join to be sure. and each request should have a result.
         int i = 0;
         for (auto& caster : overlapQuery)
         {
             caster->Join();
             //we should have some results
             EXPECT_TRUE(caster->m_result);
             EXPECT_TRUE(caster->m_result.m_hits.size() > 0);

             //check the list of result for the target
             bool targetInList = false;
             const int boxTargetIdx = i % m_boxes.size();
             for (auto& hit : caster->m_result.m_hits)
             {
                 if (hit && hit.m_entityId == m_boxes[boxTargetIdx]->GetId())
                 {
                     targetInList = true;
                     break;
                 }
             }
             EXPECT_TRUE(targetInList);

             caster.release();
             i++;
         }
         overlapQuery.clear();
     }

    struct ShapeLocalPoseSetterGetter
        : public SceneQueryBase<AZStd::pair<AZ::Vector3, AZ::Quaternion>, AZStd::pair<AZ::Vector3, AZ::Quaternion>>
    {
        ShapeLocalPoseSetterGetter(const AZStd::thread_desc& threadDesc,
            const AZStd::pair<AZ::Vector3, AZ::Quaternion>& request,
            AZStd::shared_ptr<Physics::Shape> shape)
            : SceneQueryBase(threadDesc, request, AzPhysics::InvalidSceneHandle)
            , m_shape(shape)
        {

        }

    private:

        AZStd::shared_ptr<Physics::Shape> m_shape;

        void RunRequest() override
        {
            m_shape->SetLocalPose(m_request.first, m_request.second);
            m_result = m_shape->GetLocalPose();
        }
    };

#if (PX_PHYSICS_VERSION_MAJOR == 5)
    // Double-buffering is removed in PhysX 5.
    // It is not allowed to add, remove or modify scene objects while the simulation is running.
    // If this functionality is desired, the application layer needs to implement the buffering of the changes and apply them after the fetchResults() call.
    // Disabling this test until it is implemented in the PhysX gem.
    TEST_P(PhysXMultithreadingTest, DISABLED_SetGetLocalShapeFromParallelThreads)
#else
    TEST_P(PhysXMultithreadingTest, SetGetLocalShapeFromParallelThreads)
#endif
    {
        const int seed = GetParam();

        //Local pose setter getter thread pool
        AZStd::vector<AZStd::unique_ptr<ShapeLocalPoseSetterGetter>> setterGetterQueries;

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "SGLSFPThreads";  // SetGetLocalShapeFromParallelThreads

        //create the local pose set request
        for (int i = 0; i < Constants::NumThreads; i++)
        {
            //pick a box
            const int boxTargetIdx = i % m_boxes.size();
            const AZ::EntityId entityId = m_boxes[boxTargetIdx]->GetId();
            const AZStd::pair<AZ::Vector3, AZ::Quaternion> pose = { Constants::BoxPositions[boxTargetIdx], AZ::Quaternion::CreateIdentity() };

            AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes;
            PhysX::ColliderComponentRequestBus::EventResult(shapes, entityId, &PhysX::ColliderComponentRequests::GetShapes);

            setterGetterQueries.emplace_back(AZStd::make_unique<ShapeLocalPoseSetterGetter>(threadDesc, pose, shapes[0]));
        }

        //start all threads
        AZ::SimpleLcgRandom random(seed); //constant seed to have consistency.
        for (auto& query : setterGetterQueries)
        {
            const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
            query->Start(waitTimeMS);
        }

        //update the world
        Log_Help("SetGetLocalShapeFromParallelThreads", "Start world Update\n");
        UpdateTestSceneOverTime(m_defaultScene, 500);
        Log_Help("SetGetLocalShapeFromParallelThreads", "End world Update\n");

        //each request should have completed, join to be sure. Each request data should be == to result
        for (auto& query : setterGetterQueries)
        {
            query->Join();
            EXPECT_EQ(query->GetRequest(), query->m_result);
        }
        setterGetterQueries.clear();

    }

    struct RigidBodyRayCaster
        : public SceneQueryBase<AzPhysics::RayCastRequest, AzPhysics::SceneQueryHit>
    {
        RigidBodyRayCaster(const AZStd::thread_desc& threadDesc,
            const AzPhysics::RayCastRequest& request,
            AzPhysics::RigidBody* rigidBody)
            : SceneQueryBase(threadDesc, request, AzPhysics::InvalidSceneHandle)
            , m_rigidBody(rigidBody)
        {

        }

    private:

        AzPhysics::RigidBody* m_rigidBody;

        void RunRequest() override
        {
            m_result = m_rigidBody->RayCast(m_request);
        }
    };

#if (PX_PHYSICS_VERSION_MAJOR == 5)
    // Double-buffering is removed in PhysX 5.
    // It is not allowed to add, remove or modify scene objects while the simulation is running.
    // If this functionality is desired, the application layer needs to implement the buffering of the changes and apply them after the
    // fetchResults() call. Disabling this test until it is implemented in the PhysX gem.
    TEST_P(PhysXMultithreadingTest, DISABLED_RigidBodyRayCaster)
#else
    TEST_P(PhysXMultithreadingTest, RigidBodyRayCaster)
#endif
    {
        const int seed = GetParam();

        //raycast thread pool
        AZStd::vector<AZStd::unique_ptr<RigidBodyRayCaster>> rayCasters;

        //common request data
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3::CreateZero();
        request.m_distance = 2000.0f;

        AZStd::thread_desc threadDesc;
        threadDesc.m_name = "RBRQFPThreads";  // RigidBodyRaycastsQueryFromParallelThreads

        //create the raycasts
        for (int i = 0; i < Constants::NumThreads; i++)
        {
            //pick a box to raycast against
            const int boxTargetIdx = i % m_boxes.size();
            request.m_direction = Constants::BoxPositions[boxTargetIdx].GetNormalized();
            AzPhysics::RigidBody* rigidBody = nullptr;
            Physics::RigidBodyRequestBus::EventResult(rigidBody, m_boxes[boxTargetIdx]->GetId(), &Physics::RigidBodyRequests::GetRigidBody);

            rayCasters.emplace_back(AZStd::make_unique<RigidBodyRayCaster>(threadDesc, request, rigidBody));
        }

        //start all threads
        AZ::SimpleLcgRandom random(seed); //constant seed to have consistency.
        for (auto& caster : rayCasters)
        {
            const int waitTimeMS = aznumeric_cast<int>((random.GetRandomFloat() + 0.25f) * 250.0f); //generate a time between 62.5 - 312.5 ms
            caster->Start(waitTimeMS);
        }

        //update the world
        Log_Help("RaycastsQueryFromParallelThreads", "Start world Update\n");
        UpdateTestSceneOverTime(m_defaultScene, 500);
        Log_Help("RaycastsQueryFromParallelThreads", "End world Update\n");

        //each request should have completed, join to be sure. and each request should have a result.
        int i = 0;
        for (auto& caster : rayCasters)
        {
            caster->Join();

            const int boxTargetIdx = i % m_boxes.size();
            EXPECT_TRUE(caster->m_result);
            EXPECT_TRUE(caster->m_result.m_entityId == m_boxes[boxTargetIdx]->GetId());

            caster.release();
            i++;
        }
        rayCasters.clear();
    }


    INSTANTIATE_TEST_CASE_P(PhysXMultithreading, PhysXMultithreadingTest, ::testing::Values(1, 42, 123, 1337, 1403, 5317, 133987258));
}

#ifdef PHYSX_MT_DEBUG_LOGS
#undef PHYSX_MT_DEBUG_LOGS
#undef Log_Help
#endif //PHYSX_MT_DEBUG_LOGS


#endif //PHYSX_ENABLE_MULTI_THREADING
