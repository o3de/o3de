/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/EBus.h>

namespace EMotionFX::MotionMatching
{
    class ImGuiMonitorRequests
        : public AZ::EBusTraits
    {
    public:
        // Enable multi-threaded access by locking primitive using a mutex when connecting handlers to the EBus or executing events.
        using MutexType = AZStd::recursive_mutex;

        virtual void PushPerformanceHistogramValue(const char* performanceMetricName, float value) = 0;
        virtual void PushCostHistogramValue(const char* costName, float value, const AZ::Color& color) = 0;

        virtual void SetFeatureMatrixMemoryUsage(size_t sizeInBytes) = 0;
        virtual void SetFeatureMatrixNumFrames(size_t numFrames) = 0;
        virtual void SetFeatureMatrixNumComponents(size_t numFeatureComponents) = 0;

        virtual void SetKdTreeMemoryUsage(size_t sizeInBytes) = 0;
        virtual void SetKdTreeNumNodes(size_t numNodes) = 0;
        virtual void SetKdTreeNumDimensions(size_t numDimensions) = 0;
    };
    using ImGuiMonitorRequestBus = AZ::EBus<ImGuiMonitorRequests>;
} // namespace EMotionFX::MotionMatching
