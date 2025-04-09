/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <ShapeThreadsafeTest.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace UnitTest
{
    void ShapeThreadsafeTest::TestShapeGetSetCallsAreThreadsafe(
        AZ::Entity& shapeEntity, int numIterations,
        AZStd::function<void(AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, float height)> shapeSetFn)
    {
        // This test will run parallel threads that all query "DistanceFromPoint" on the same shape and test point while
        // simultaneously running a thread that keeps changing any unimportant dimensions on the shape.
        // If the calls are threadsafe between Get/Set and between multiple Get calls themselves, all queries should return
        // the same distance because the shape height and point queried are staying invariant.
        // If the calls aren't threadsafe, the internal shape data will become inconsistent and we can get arbitrary results.

        // The test point that we'll use for getting the distance to the shape.
        const AZ::Vector3 TestPoint(0.0f, 0.0f, 20.0f);

        // The expected distance from the test point to the shape.
        // Since we're setting it above the shape and keeping the height constant, the expected distance will
        // always be 10.  (The shape extends 10 above and 10 below the origin, so 20 above origin is 10 above the shape)
        constexpr float ExpectedDistance = 10.0f;

        // Comparing floats needs a tolerance based on whether we are using NEON or not
        #if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        constexpr float compareTolerance = 1.0e-4;
        #else
        constexpr float compareTolerance = 1.0e-6;
        #endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON


        AZ::EntityId shapeEntityId = shapeEntity.GetId();

        // Pick an arbitrary number of threads and iterations that are large enough to demonstrate thread safety problems.
        constexpr int NumQueryThreads = 4;
        AZStd::thread queryThreads[NumQueryThreads];

        AZStd::semaphore syncThreads;

        // Create all of the threads that will query DistanceFromPoint.
        for (auto& queryThread : queryThreads)
        {
            queryThread = AZStd::thread(
                [shapeEntityId, TestPoint, ExpectedDistance = ExpectedDistance, numIterations, &syncThreads]()
                {
                    // Block until all the threads are created, so that we can run them 100% in parallel.
                    syncThreads.acquire();

                    // Keep querying the same shape and point and verify that we get back the same distance.
                    // This can fail if the calls aren't threadsafe because the internal shape data will become inconsistent
                    // and return odd results.
                    for (int i = 0; i < numIterations; i++)
                    {
                        // Pick an impossible value to initialize with so that we can see in the results if we ever fail
                        // due to a shape not being connected to the EBus.
                        float distance = -10.0f;

                        LmbrCentral::ShapeComponentRequestsBus::EventResult(
                            distance, shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::DistanceFromPoint, TestPoint);

                        // This is wrapped in an 'if' statement just to make it easier to debug if anything goes wrong.
                        // You can put a breakpoint on the ASSERT_EQ line to see the current state of things in the failure case.
                        if (distance != ExpectedDistance)
                        {
                            ASSERT_NEAR(distance, ExpectedDistance, compareTolerance);
                        }
                    }
                });
        }

        // Create a single thread that will continuously set shape dimension except height to random values in a tight loop
        // until all our query threads have finished their iterations.
        AZStd::atomic_bool stopSetThread = false;
        AZStd::thread setThread = AZStd::thread(
            [shapeEntityId, NumQueryThreads = NumQueryThreads, &shapeSetFn, &syncThreads, &stopSetThread]()
            {
                // Now that all threads are created, signal everything to start running in parallel.
                syncThreads.release(NumQueryThreads);

                // Change the dimensions in a tight loop until the query threads are all finished.
                while (!stopSetThread)
                {
                    shapeSetFn(shapeEntityId, MinDimension, DimensionVariance, ShapeHeight);
                }
            });

        // Wait for all the query threads to finish.
        for (auto& queryThread : queryThreads)
        {
            queryThread.join();
        }

        // Signal that the "set" thread should finish and wait for it to end.
        stopSetThread = true;
        setThread.join();
    }

}
