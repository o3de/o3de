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

namespace AZ::RHI
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
        MultiDevice::DeviceMask deviceMask, const PipelineStateDescriptor& descriptor, MultiDevicePipelineLibrary* pipelineLibrary)
    {
        if (!ValidateNotInitialized())
        {
            return ResultCode::InvalidOperation;
        }

        if (pipelineLibrary)
        {
            deviceMask &= pipelineLibrary->GetDeviceMask();
        }

        MultiDeviceObject::Init(deviceMask);

        ResultCode resultCode = ResultCode::Success;

        IterateDevices(
            [this, &descriptor, &pipelineLibrary, &resultCode](int deviceIndex)
            {
                auto* device = RHISystemInterface::Get()->GetDevice(deviceIndex);

                m_deviceObjects[deviceIndex] = Factory::Get().CreatePipelineState();
                switch (descriptor.GetType())
                {
                case PipelineStateType::Draw:
                    {
                        resultCode = GetDevicePipelineState(deviceIndex)->Init(
                            *device,
                            static_cast<const PipelineStateDescriptorForDraw&>(descriptor),
                            pipelineLibrary ? pipelineLibrary->GetDevicePipelineLibrary(deviceIndex).get() : nullptr);

                        if (resultCode == ResultCode::Success)
                        {
                            m_type = PipelineStateType::Draw;
                        }
                        break;
                    }
                case PipelineStateType::Dispatch:
                    {
                        resultCode = GetDevicePipelineState(deviceIndex)->Init(
                            *device,
                            static_cast<const PipelineStateDescriptorForDispatch&>(descriptor),
                            pipelineLibrary ? pipelineLibrary->GetDevicePipelineLibrary(deviceIndex).get() : nullptr);

                        if (resultCode == ResultCode::Success)
                        {
                            m_type = PipelineStateType::Dispatch;
                        }
                    }
                case PipelineStateType::RayTracing:
                    {
                        resultCode = GetDevicePipelineState(deviceIndex)->Init(
                            *device,
                            static_cast<const PipelineStateDescriptorForRayTracing&>(descriptor),
                            pipelineLibrary ? pipelineLibrary->GetDevicePipelineLibrary(deviceIndex).get() : nullptr);

                        if (resultCode == ResultCode::Success)
                        {
                            m_type = PipelineStateType::RayTracing;
                        }
                    }
                default:
                    {
                        AZ_Error("MultiDevicePipelineState", false, "MultiDevicePipelineState already initialized!");
                        resultCode = ResultCode::InvalidArgument;
                        break;
                    }
                }

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Reset already initialized device-specific PipelineStates and set deviceMask to 0
            m_deviceObjects.clear();
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        return resultCode;
    }

    void MultiDevicePipelineState::Shutdown()
    {
        if (IsInitialized())
        {
            m_deviceObjects.clear();
            MultiDeviceObject::Shutdown();
        }
    }

    PipelineStateType MultiDevicePipelineState::GetType() const
    {
        return m_type;
    }
} // namespace AZ::RHI
