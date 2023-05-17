/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDevicePipelineLibrary.h>
#include <Atom/RHI/MultiDevicePipelineState.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        bool MultiDevicePipelineState::ValidateNotInitialized() const
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("MultiDevicePipelineState", false, "MultiDevicePipelineState already initialized!");
                    return false;
                }
            }

            return true;
        }

        ResultCode MultiDevicePipelineState::Init(
            MultiDevice::DeviceMask deviceMask,
            const PipelineStateDescriptorForDraw& descriptor,
            MultiDevicePipelineLibrary* pipelineLibrary)
        {
            if (!ValidateNotInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            MultiDeviceObject::Init(deviceMask);

            ResultCode resultCode = ResultCode::Success;

            IterateDevices(
                [this, &descriptor, &pipelineLibrary, &resultCode](int deviceIndex)
                {
                    auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                    m_devicePipelineStates[deviceIndex] = Factory::Get().CreatePipelineState();
                    resultCode = m_devicePipelineStates[deviceIndex]->Init(
                        *device, descriptor, pipelineLibrary ? pipelineLibrary->GetDevicePipelineLibrary(deviceIndex).get() : nullptr);

                    return resultCode == ResultCode::Success;
                });

            if (resultCode == ResultCode::Success)
            {
                m_type = PipelineStateType::Draw;
            }

            return resultCode;
        }

        ResultCode MultiDevicePipelineState::Init(
            MultiDevice::DeviceMask deviceMask,
            const PipelineStateDescriptorForDispatch& descriptor,
            MultiDevicePipelineLibrary* pipelineLibrary)
        {
            if (!ValidateNotInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            MultiDeviceObject::Init(deviceMask);

            ResultCode resultCode = ResultCode::Success;

            IterateDevices(
                [this, &descriptor, &pipelineLibrary, &resultCode](int deviceIndex)
                {
                    auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                    m_devicePipelineStates[deviceIndex] = Factory::Get().CreatePipelineState();
                    resultCode = m_devicePipelineStates[deviceIndex]->Init(
                        *device, descriptor, pipelineLibrary->GetDevicePipelineLibrary(deviceIndex).get());

                    return resultCode == ResultCode::Success;
                });

            if (resultCode == ResultCode::Success)
            {
                m_type = PipelineStateType::Dispatch;
            }

            return resultCode;
        }

        ResultCode MultiDevicePipelineState::Init(
            MultiDevice::DeviceMask deviceMask,
            const PipelineStateDescriptorForRayTracing& descriptor,
            MultiDevicePipelineLibrary* pipelineLibrary)
        {
            if (!ValidateNotInitialized())
            {
                return ResultCode::InvalidOperation;
            }

            MultiDeviceObject::Init(deviceMask);

            ResultCode resultCode = ResultCode::Success;

            IterateDevices(
                [this, &descriptor, &pipelineLibrary, &resultCode](int deviceIndex)
                {
                    auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                    m_devicePipelineStates[deviceIndex] = Factory::Get().CreatePipelineState();
                    resultCode = m_devicePipelineStates[deviceIndex]->Init(
                        *device, descriptor, pipelineLibrary->GetDevicePipelineLibrary(deviceIndex).get());

                    return resultCode == ResultCode::Success;
                });

            if (resultCode == ResultCode::Success)
            {
                m_type = PipelineStateType::RayTracing;
            }

            return resultCode;
        }

        void MultiDevicePipelineState::Shutdown()
        {
            if (IsInitialized())
            {
                m_devicePipelineStates.clear();
                MultiDeviceObject::Shutdown();
            }
        }

        PipelineStateType MultiDevicePipelineState::GetType() const
        {
            return m_type;
        }
    } // namespace RHI
} // namespace AZ
