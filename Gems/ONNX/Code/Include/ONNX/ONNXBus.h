/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Ort
{
    struct Env;
    struct AllocatorWithDefaultOptions;
} // namespace Ort

namespace ONNX
{
    struct PrecomputedTimingData
    {
        float m_totalPrecomputedRuntime;
        float m_averagePrecomputedRuntime;
        int m_totalNumberOfInferences;
        int64_t m_numberOfCorrectInferences;
    };

    class ONNXRequests
    {
    public:
        AZ_RTTI(ONNXRequests, "{F8599C7E-CDC7-4A72-A296-2C043D1E525A}");
        virtual ~ONNXRequests() = default;

        virtual Ort::Env* GetEnv() = 0;
        virtual Ort::AllocatorWithDefaultOptions* GetAllocator() = 0;

        virtual void AddTimingSample(const char* modelName, float inferenceTimeInMilliseconds) = 0;
        virtual void AddTimingSampleCuda(const char* modelName, float inferenceTimeInMilliseconds) = 0;

        virtual PrecomputedTimingData* GetPrecomputedTimingData() = 0;
        virtual void SetPrecomputedTimingData(int totalCount, int64_t correctCount, float totalTime, float avgTime) = 0;

        virtual PrecomputedTimingData* GetPrecomputedTimingDataCuda() = 0;
        virtual void SetPrecomputedTimingDataCuda(int totalCount, int64_t correctCount, float totalTime, float avgTime) = 0;
    };

    class ONNXBusTraits : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ONNXRequestBus = AZ::EBus<ONNXRequests, ONNXBusTraits>;
    using ONNXInterface = AZ::Interface<ONNXRequests>;

} // namespace ONNX
