/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/PipelineState.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDevicePipelineLibrary;

        /**
         * The pipeline state object is an opaque data structure representing compiled
         * graphics or compute state. Typically called a 'PSO', it holds the following
         * platform-specific state:
         *
         * - The compiled shader byte code.
         *
         * - The compiled pipeline layout containing shader bindings and how they map to
         *   provided shader byte codes.
         *
         * - [Graphics Only] State to control the fixed function output-merger unit. This includes
         *   Blend, Raster, and Depth-Stencil state.
         *
         * - [Graphics Only] State to identify stream buffers to the fixed function input assembly unit.
         */
        class MultiDevicePipelineState : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDevicePipelineState, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDevicePipelineState, "77B85640-C2E2-4312-AD67-68FED421F84E", MultiDeviceObject);
            MultiDevicePipelineState() = default;
            virtual ~MultiDevicePipelineState() = default;

            /**
             * Initializes a graphics pipeline state, associated with the provided device, using the
             * provided descriptor. If successful, the PSO is valid to use with DrawItems within scopes
             * executing on device used for initialization.
             * @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
             * platform pipeline state data, reducing compilation and memory cost. It can be left null.
             */
            ResultCode Init(
                MultiDevice::DeviceMask deviceMask,
                const PipelineStateDescriptorForDraw& descriptor,
                MultiDevicePipelineLibrary* pipelineLibrary = nullptr);

            /**
             * Initializes a compute pipeline state, associated with the provided device, using the
             * provided descriptor. If successful, the PSO is valid to use with DispatchItems within scopes
             * executing on device used for initialization.
             * @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
             * platform pipeline state data, reducing compilation and memory cost. It can be left null.
             */
            ResultCode Init(
                MultiDevice::DeviceMask deviceMask,
                const PipelineStateDescriptorForDispatch& descriptor,
                MultiDevicePipelineLibrary* pipelineLibrary = nullptr);

            /**
             * Initializes a ray tracing pipeline state, associated with the provided device, using the
             * provided descriptor. If successful, the PSO is valid to use with DispatchRaysItems within scopes
             * executing on device used for initialization.
             * @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
             * platform pipeline state data, reducing compilation and memory cost. It can be left null.
             */
            ResultCode Init(
                MultiDevice::DeviceMask deviceMask,
                const PipelineStateDescriptorForRayTracing& descriptor,
                MultiDevicePipelineLibrary* pipelineLibrary = nullptr);

            inline Ptr<PipelineState> GetDevicePipelineState(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDevicePipelineState",
                    m_devicePipelineStates.find(deviceIndex) != m_devicePipelineStates.end(),
                    "No DevicePipelineState found for device index %d\n",
                    deviceIndex);

                return m_devicePipelineStates.at(deviceIndex);
            }

            PipelineStateType GetType() const;

        private:
            // Pipeline states cannot be re-initialized, as they can be cached.
            void Shutdown() override final;

            bool ValidateNotInitialized() const;

            PipelineStateType m_type = PipelineStateType::Count;

            AZStd::unordered_map<int, Ptr<PipelineState>> m_devicePipelineStates;
        };
    } // namespace RHI
} // namespace AZ
