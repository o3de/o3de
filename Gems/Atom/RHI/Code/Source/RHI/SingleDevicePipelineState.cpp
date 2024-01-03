/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI/SingleDevicePipelineState.h>

namespace AZ::RHI
{
    bool SingleDevicePipelineState::ValidateNotInitialized() const
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Error("SingleDevicePipelineState", false, "SingleDevicePipelineState already initialized!");
                return false;
            }
        }

        return true;
    }

    ResultCode SingleDevicePipelineState::Init(Device& device, const PipelineStateDescriptorForDraw& descriptor, SingleDevicePipelineLibrary* pipelineLibrary)
    {
        if (!ValidateNotInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (Validation::IsEnabled())
        {
            bool error = false;

            if (!descriptor.m_inputStreamLayout.IsFinalized())
            {
                AZ_Error("SingleDevicePipelineState", false, "InputStreamLayout is not finalized!");
                error = true;
            }

            const auto& renderTargetConfiguration = descriptor.m_renderAttachmentConfiguration;

            if (renderTargetConfiguration.m_subpassIndex >= renderTargetConfiguration.m_renderAttachmentLayout.m_subpassCount)
            {
                AZ_Error("SingleDevicePipelineState", false, "Invalid subpassIndex %d. SubpassCount is %d.", renderTargetConfiguration.m_subpassIndex, renderTargetConfiguration.m_renderAttachmentLayout.m_subpassCount);
                return ResultCode::InvalidOperation;
            }

            if (descriptor.m_renderStates.m_depthStencilState.m_depth.m_enable || descriptor.m_renderStates.m_depthStencilState.m_stencil.m_enable)
            {
                if (renderTargetConfiguration.GetDepthStencilFormat() == RHI::Format::Unknown)
                {
                    AZ_Error("SingleDevicePipelineState", false, "Depth-stencil format is not set.");
                    error = true;
                }
            }

            for (uint32_t i = 0; i < renderTargetConfiguration.GetRenderTargetCount(); ++i)
            {
                if (renderTargetConfiguration.GetRenderTargetFormat(i) == RHI::Format::Unknown)
                {
                    AZ_Error("SingleDevicePipelineState", false, "Rendertarget attachment %d format is not set.", i);
                    error = true;
                }
            }

            for (uint32_t i = 0; i < renderTargetConfiguration.GetSubpassInputCount(); ++i)
            {
                if (renderTargetConfiguration.GetSubpassInputFormat(i) == RHI::Format::Unknown)
                {
                    AZ_Error("SingleDevicePipelineState", false, "Subpass input attachment %d format is not set.", i);
                    error = true;
                }
            }

            for (uint32_t i = 0; i < renderTargetConfiguration.GetRenderTargetCount(); ++i)
            {
                if (renderTargetConfiguration.DoesRenderTargetResolve(i) &&
                    renderTargetConfiguration.GetRenderTargetResolveFormat(i) != renderTargetConfiguration.GetRenderTargetFormat(i))
                {
                    AZ_Error("SingleDevicePipelineState", false, "Invalid resolve format for attachment %d.", i);
                    error = true;
                }
            }

            if (error)
            {
                return ResultCode::InvalidOperation;
            }
        }

        const ResultCode resultCode = InitInternal(device, descriptor, pipelineLibrary);

        if (resultCode == ResultCode::Success)
        {
            m_type = PipelineStateType::Draw;
            DeviceObject::Init(device);
        }

        return resultCode;
    }

    ResultCode SingleDevicePipelineState::Init(Device& device, const PipelineStateDescriptorForDispatch& descriptor, SingleDevicePipelineLibrary* pipelineLibrary)
    {
        if (!ValidateNotInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        const ResultCode resultCode = InitInternal(device, descriptor, pipelineLibrary);

        if (resultCode == ResultCode::Success)
        {
            m_type = PipelineStateType::Dispatch;
            DeviceObject::Init(device);
        }

        return resultCode;
    }

    ResultCode SingleDevicePipelineState::Init(Device& device, const PipelineStateDescriptorForRayTracing& descriptor, SingleDevicePipelineLibrary* pipelineLibrary)
    {
        if (!ValidateNotInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        const ResultCode resultCode = InitInternal(device, descriptor, pipelineLibrary);

        if (resultCode == ResultCode::Success)
        {
            m_type = PipelineStateType::RayTracing;
            DeviceObject::Init(device);
        }

        return resultCode;
    }

    void SingleDevicePipelineState::Shutdown()
    {
        if (IsInitialized())
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }

    PipelineStateType SingleDevicePipelineState::GetType() const
    {
        return m_type;
    }
}
