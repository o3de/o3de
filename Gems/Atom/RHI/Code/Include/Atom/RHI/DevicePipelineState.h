/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/DeviceObject.h>

namespace AZ::RHI
{
    class DevicePipelineLibrary;

    //! The pipeline state object is an opaque data structure representing compiled
    //! graphics or compute state. Typically called a 'PSO', it holds the following
    //! platform-specific state:
    //!
    //! - The compiled shader byte code.
    //!
    //! - The compiled pipeline layout containing shader bindings and how they map to
    //!   provided shader byte codes.
    //!
    //! - [Graphics Only] State to control the fixed function output-merger unit. This includes
    //!   Blend, Raster, and Depth-Stencil state.
    //! 
    //! - [Graphics Only] State to identify stream buffers to the fixed function input assembly unit.
    class DevicePipelineState
        : public DeviceObject
    {
    public:
        virtual ~DevicePipelineState() = default;

        //! Initializes a graphics pipeline state, associated with the provided device, using the
        //! provided descriptor. If successful, the PSO is valid to use with DrawItems within scopes
        //! executing on device used for initialization.
        //! @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
        //! platform pipeline state data, reducing compilation and memory cost. It can be left null.
        ResultCode Init(Device& device, const PipelineStateDescriptorForDraw& descriptor, DevicePipelineLibrary* pipelineLibrary = nullptr);

        //! Initializes a compute pipeline state, associated with the provided device, using the
        //! provided descriptor. If successful, the PSO is valid to use with DispatchItems within scopes
        //! executing on device used for initialization.
        //! @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
        //! platform pipeline state data, reducing compilation and memory cost. It can be left null.
        ResultCode Init(Device& device, const PipelineStateDescriptorForDispatch& descriptor, DevicePipelineLibrary* pipelineLibrary = nullptr);

        //! Initializes a ray tracing pipeline state, associated with the provided device, using the
        //! provided descriptor. If successful, the PSO is valid to use with DispatchRaysItems within scopes
        //! executing on device used for initialization.
        //! @param pipelineLibrary An optional pipeline library used to de-duplicate and cache the internal
        //! platform pipeline state data, reducing compilation and memory cost. It can be left null.
        ResultCode Init(Device& device, const PipelineStateDescriptorForRayTracing& descriptor, DevicePipelineLibrary* pipelineLibrary = nullptr);

        PipelineStateType GetType() const;

    private:
        // Pipeline states cannot be re-initialized, as they can be cached.
        void Shutdown() override final;

        bool ValidateNotInitialized() const;

        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when a graphics PSO is being initialized.
        virtual ResultCode InitInternal(Device& device, const PipelineStateDescriptorForDraw& descriptor, DevicePipelineLibrary* pipelineLibrary) = 0;

        /// Called when a compute PSO is being initialized.
        virtual ResultCode InitInternal(Device& device, const PipelineStateDescriptorForDispatch& descriptor, DevicePipelineLibrary* pipelineLibrary) = 0;

        /// Called when a ray tracing PSO is being initialized.
        virtual ResultCode InitInternal(Device& device, const PipelineStateDescriptorForRayTracing& descriptor, DevicePipelineLibrary* pipelineLibrary) = 0;

        /// Called when the PSO is being shutdown.
        virtual void ShutdownInternal() = 0;

        //////////////////////////////////////////////////////////////////////////

        PipelineStateType m_type = PipelineStateType::Count;
    };
}
