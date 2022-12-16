/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/PipelineState.h>

#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI/DevicePipelineState.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        bool PipelineState::ValidateNotInitialized() const
        {
            if (Validation::IsEnabled())
            {
                if (IsInitialized())
                {
                    AZ_Error("PipelineState", false, "PipelineState already initialized!");
                    return false;
                }
            }

            return true;
        }

        ResultCode PipelineState::Init(
            DeviceMask deviceMask, const PipelineStateDescriptorForDraw& descriptor, PipelineLibrary* pipelineLibrary)
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
                m_type = PipelineStateType::Draw;
            }

            return resultCode;
        }

        ResultCode PipelineState::Init(
            DeviceMask deviceMask, const PipelineStateDescriptorForDispatch& descriptor, PipelineLibrary* pipelineLibrary)
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

        ResultCode PipelineState::Init(
            DeviceMask deviceMask, const PipelineStateDescriptorForRayTracing& descriptor, PipelineLibrary* pipelineLibrary)
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

        void PipelineState::Shutdown()
        {
            if (IsInitialized())
            {
                m_devicePipelineStates.clear();
                MultiDeviceObject::Shutdown();
            }
        }

        PipelineStateType PipelineState::GetType() const
        {
            return m_type;
        }
    }
}
