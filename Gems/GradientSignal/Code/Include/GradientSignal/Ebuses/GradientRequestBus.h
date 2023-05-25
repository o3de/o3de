/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/EBusSharedDispatchTraits.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/span.h>

namespace GradientSignal
{
    struct GradientSampleParams final
    {
        AZ_CLASS_ALLOCATOR(GradientSampleParams, AZ::SystemAllocator);
        AZ_TYPE_INFO(GradientSampleParams, "{DC4B9269-CB3C-4071-989D-C885FB9946A5}");

        GradientSampleParams() {}
        GradientSampleParams(const AZ::Vector3& position) : m_position(position) {}
        AZ::Vector3 m_position = AZ::Vector3(0.0f, 0.0f, 0.0f);
    };

    /**
     * Handles gradient sampling requests based on up to 3 data points such as X,Y,Z.
     * This bus uses shared dispatches, which means that all requests on the bus can run in parallel, but will NOT run in parallel
     * with bus connections / disconnections.
     */
    class GradientRequests : public AZ::EBusSharedDispatchTraits<GradientRequests>
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        ////////////////////////////////////////////////////////////////////////

        virtual ~GradientRequests() = default;

        /**
        * Given a certain position, generate a value.  Implementations of this need to be thread-safe without using locks, 
        * as it can get called from both the Main thread and the Vegetation thread simultaneously, and has the potential to cause
        * lock inversion deadlocks.
        * @return a value generated using the position
        */
        virtual float GetValue(const GradientSampleParams& sampleParams) const = 0;

        /**
         * Given a list of positions, generate values. Implementations of this need to be thread-safe without using locks,
         * as it can get called from multiple threads simultaneously and has the potential to cause lock inversion deadlocks.
         * \param positions The input list of positions to query.
         * \param outValues The output list of values. This list is expected to be the same size as the positions list.
         */
        virtual void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
        {
            // Reference implementation of GetValues for any gradients that don't have their own optimized implementations.
            // This is 10%-60% faster than calling GetValue via EBus many times due to the per-call EBus overhead.

            if (positions.size() != outValues.size())
            {
                AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
                return;
            }

            GradientSampleParams sampleParams;
            for (size_t index = 0; index < positions.size(); index++)
            {
                sampleParams.m_position = positions[index];
                outValues[index] = GetValue(sampleParams);
            }
        }

        /**
        * Call to check the hierarchy to see if a given entityId exists in the gradient signal chain
        */
        virtual bool IsEntityInHierarchy([[maybe_unused]] const AZ::EntityId& entityId) const { return false; }
    };

    using GradientRequestBus = AZ::EBus<GradientRequests>;

} // namespace GradientSignal

