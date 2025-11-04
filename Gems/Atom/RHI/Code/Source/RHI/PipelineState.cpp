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
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
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

    void PipelineState::PreInitialize(MultiDevice::DeviceMask deviceMask)
    {
        MultiDeviceObject::IterateDevices(
            deviceMask,
            [this](int deviceIndex)
            {
                m_deviceObjects[deviceIndex] = Factory::Get().CreatePipelineState();
                return true;
            });
    }

    ResultCode PipelineState::Init(
        MultiDevice::DeviceMask deviceMask, const PipelineStateDescriptor& descriptor, PipelineLibrary* pipelineLibrary)
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

                if(!m_deviceObjects.contains(deviceIndex))
                {
                    m_deviceObjects[deviceIndex] = Factory::Get().CreatePipelineState();
                }

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
                        break;
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
                        break;
                    }
                default:
                    {
                        AZ_Error("PipelineState", false, "Unknown PipelineStateType!");
                        resultCode = ResultCode::InvalidArgument;
                        break;
                    }
                }

                return resultCode == ResultCode::Success;
            });

        if (resultCode != ResultCode::Success)
        {
            // Only reset the device mask but the the device-specific PipelineStates, as other
            // threads might be using them already
            MultiDeviceObject::Init(static_cast<MultiDevice::DeviceMask>(0u));
        }

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }

        return resultCode;
    }

    void PipelineState::Shutdown()
    {
        m_deviceObjects.clear();
        if (IsInitialized())
        {
            MultiDeviceObject::Shutdown();
        }
    }

    PipelineStateType PipelineState::GetType() const
    {
        return m_type;
    }
} // namespace AZ::RHI
