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

        struct FrameDatabaseInfo
        {
            size_t m_memoryUsedInBytes = 0;
            size_t m_numFrames;
            size_t m_numMotions;
            float m_motionDataInSeconds;
        };
        virtual void SetFrameDatabaseInfo(const FrameDatabaseInfo& info) = 0;

        struct FeatureMatrixInfo
        {
            size_t m_memoryUsedInBytes = 0;
            size_t m_numFrames = 0;
            size_t m_numDimensions = 0;
        };
        virtual void SetFeatureMatrixInfo(const FeatureMatrixInfo& info) = 0;

        struct KdTreeInfo
        {
            size_t m_memoryUsedInBytes = 0;
            size_t m_numNodes = 0;
            size_t m_numDimensions = 0;
        };
        virtual void SetKdTreeInfo(const KdTreeInfo& info) = 0;
    };
    using ImGuiMonitorRequestBus = AZ::EBus<ImGuiMonitorRequests>;
} // namespace EMotionFX::MotionMatching
