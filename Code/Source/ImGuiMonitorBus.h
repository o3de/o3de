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
