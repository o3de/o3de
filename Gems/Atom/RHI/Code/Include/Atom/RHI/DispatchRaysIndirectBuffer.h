/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceDispatchRaysIndirectBuffer.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/RayTracingShaderTable.h>

namespace AZ
{
    namespace RHI
    {
        //! This class needs to be passed to the command list when submitting an indirect raytracing command
        //! The class is only relavant for DX12, other RHIs have dummy implementations
        //! For more information, see the DX12 implementation of this class
        class DispatchRaysIndirectBuffer : public Object
        {
        public:
            AZ_RTTI(AZ::RHI::DispatchRaysIndirectBuffer, "{25E39682-5D6C-4ECF-8F15-2C5EFD8B14D2}", Object)
            DispatchRaysIndirectBuffer(MultiDevice::DeviceMask deviceMask)
                : m_deviceMask{ deviceMask }
            {
                MultiDeviceObject::IterateDevices(
                    m_deviceMask,
                    [this](int deviceIndex)
                    {
                        m_deviceDispatchRaysIndirectBuffers.emplace(deviceIndex, Factory::Get().CreateDispatchRaysIndirectBuffer());
                        return true;
                    });
            }

            Ptr<DeviceDispatchRaysIndirectBuffer> GetDeviceDispatchRaysIndirectBuffer(int deviceIndex) const
            {
                AZ_Error(
                    "DispatchRaysIndirectBuffer",
                    m_deviceDispatchRaysIndirectBuffers.find(deviceIndex) != m_deviceDispatchRaysIndirectBuffers.end(),
                    "No DeviceDispatchRaysIndirectBuffer found for device index %d\n",
                    deviceIndex);

                return m_deviceDispatchRaysIndirectBuffers.at(deviceIndex);
            }

            AZ_DISABLE_COPY_MOVE(DispatchRaysIndirectBuffer);

            void Init(RHI::BufferPool* bufferPool)
            {
                for (auto& [deviceIndex, dispatchRaysIndirectBuffer] : m_deviceDispatchRaysIndirectBuffers)
                {
                    dispatchRaysIndirectBuffer->Init(bufferPool->GetDeviceBufferPool(deviceIndex).get());
                }
            }

            // This needs to be called every time the shader table changes
            void Build(RayTracingShaderTable* shaderTable)
            {
                for (auto& [deviceIndex, dispatchRaysIndirectBuffer] : m_deviceDispatchRaysIndirectBuffers)
                {
                    dispatchRaysIndirectBuffer->Build(shaderTable->GetDeviceRayTracingShaderTable(deviceIndex).get());
                }
            }

        private:
            //! A DeviceMask denoting on which devices a device-specific SingleDeviceDispatchItem should be generated
            MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
            //! A map of all device-specific DeviceDispatchRaysIndirectBuffer, indexed by the device index
            AZStd::unordered_map<int, Ptr<DeviceDispatchRaysIndirectBuffer>> m_deviceDispatchRaysIndirectBuffers;
        };
    } // namespace RHI
} // namespace AZ
