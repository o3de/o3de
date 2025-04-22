/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/PipelineStateDescriptor.h>

namespace AZ::RHI
{
    class PipelineLibrary;

    //! PipelineState is a multi-device class (representing a DevicePipelineState on multiple devices).
    //! It holds a map of device-specific DevicePipelineState objects, which can be addressed with a device index.
    //! The class is initialized with a device mask (1 bit per device), which initializes one DevicePipelineState
    //! for each bit set and stores them in a map.
    //! The API then forwards all calls to the all device-specific DevicePipelineState objects by iterating over them
    //! and forwarding the call.
    //! A device-specific DevicePipelineState can be accessed by calling GetDevicePipelineState
    //! with the corresponding device index
    class PipelineState : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(PipelineState, AZ::SystemAllocator, 0);
        AZ_RTTI(PipelineState, "77B85640-C2E2-4312-AD67-68FED421F84E", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(PipelineState);
        PipelineState() = default;
        virtual ~PipelineState() = default;

        //! Initializes a pipeline state (either graphics, compute or ray tracing),
        //! associated with the all devices selected in the deviceMask, using the provided descriptor.
        //! The initialization call is forwarded to the respective device- and descriptor-specific method.
        //! @param descriptor Can be either PipeLineStateDescriptorForDraw, PipeLineStateDescriptorForDispatch
        //! or PipeLineStateDescriptorForRayTracing
        //! @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
        //! platform pipeline state data, reducing compilation and memory cost. It can be left null.
        ResultCode Init(
            MultiDevice::DeviceMask deviceMask,
            const PipelineStateDescriptor& descriptor,
            PipelineLibrary* pipelineLibrary = nullptr);

        //! Preinitializes a pipeline state to allow for safe usage with multi-threaded DrawPacket creation
        void PreInitialize(MultiDevice::DeviceMask deviceMask);

        PipelineStateType GetType() const;

    private:
        //! Pipeline states cannot be re-initialized, as they can be cached.
        void Shutdown() override final;

        bool ValidateNotInitialized() const;

        PipelineStateType m_type = PipelineStateType::Count;
    };
} // namespace AZ::RHI
