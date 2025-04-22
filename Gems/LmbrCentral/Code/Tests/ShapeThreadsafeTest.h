/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/thread.h>

namespace UnitTest
{
    class ShapeThreadsafeTest
    {
    public:
        // Constants for defining and modifying the shape dimensions.
        // All of our test shapes will have a constant shape height of 20, with varied settings for the other dimensions
        constexpr static inline float MinDimension = 1.0f;
        constexpr static inline uint32_t DimensionVariance = 5;
        constexpr static inline float ShapeHeight = 20.0f;

        static void TestShapeGetSetCallsAreThreadsafe(
            AZ::Entity& shapeEntity, int numIterations,
            AZStd::function<void(AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, float height)> shapeSetFn);
    };
}
