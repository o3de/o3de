/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>

namespace AZ::RHI
{
    RHI::Ptr<RHI::DeviceRayTracingPipelineState> DeviceRayTracingPipelineState::CreateRHIRayTracingPipelineState()
    {
        RHI::Ptr<RHI::DeviceRayTracingPipelineState> rayTracingPipelineState = RHI::Factory::Get().CreateRayTracingPipelineState();
        AZ_Error("DeviceRayTracingPipelineState", rayTracingPipelineState, "Failed to create RHI::DeviceRayTracingPipelineState");
        return rayTracingPipelineState;
    }

    ResultCode DeviceRayTracingPipelineState::Init(Device& device, const DeviceRayTracingPipelineStateDescriptor* descriptor)
    {
        m_descriptor = *descriptor;

        ResultCode resultCode = InitInternal(device, descriptor);
        if (resultCode == ResultCode::Success)
        {
            DeviceObject::Init(device);
        }
        return resultCode;
    }

    void DeviceRayTracingPipelineState::Shutdown()
    {
        ShutdownInternal();
        DeviceObject::Shutdown();
    }
}
