/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DispatchRaysIndirectBuffer.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceBufferPool.h>
#include <Atom/RHI/MultiDeviceRayTracingShaderTable.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        //! This class needs to be passed to the command list when submitting an indirect raytracing command
        //! The class is only relavant for DX12, other RHIs have dummy implementations
        //! For more information, see the DX12 implementation of this class
        class MultiDeviceDispatchRaysIndirectBuffer : public Object
        {
        public:
            AZ_RTTI(AZ::RHI::MultiDeviceDispatchRaysIndirectBuffer, "{25E39682-5D6C-4ECF-8F15-2C5EFD8B14D2}", Object)
            MultiDeviceDispatchRaysIndirectBuffer(MultiDevice::DeviceMask deviceMask)
                : m_deviceMask{ deviceMask }
            {
                auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

                for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
                {
                    if (CheckBitsAll(AZStd::to_underlying(m_deviceMask), 1u << deviceIndex))
                    {
                        m_deviceDispatchRaysIndirectBuffers.emplace(deviceIndex, Factory::Get().CreateDispatchRaysIndirectBuffer());
                    }
                }
            }

            Ptr<DispatchRaysIndirectBuffer> GetDeviceDispatchRaysIndirectBuffer(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceDispatchRaysIndirectBuffer",
                    m_deviceDispatchRaysIndirectBuffers.find(deviceIndex) != m_deviceDispatchRaysIndirectBuffers.end(),
                    "No DispatchRaysIndirectBuffer found for device index %d\n",
                    deviceIndex);

                return m_deviceDispatchRaysIndirectBuffers.at(deviceIndex);
            }

            AZ_DISABLE_COPY_MOVE(MultiDeviceDispatchRaysIndirectBuffer);

            void Init(RHI::MultiDeviceBufferPool* bufferPool)
            {
                for (auto& [deviceIndex, dispatchRaysIndirectBuffer] : m_deviceDispatchRaysIndirectBuffers)
                {
                    dispatchRaysIndirectBuffer->Init(bufferPool->GetDeviceBufferPool(deviceIndex).get());
                }
            }

            // This needs to be called every time the shader table changes
            void Build(MultiDeviceRayTracingShaderTable* shaderTable)
            {
                for (auto& [deviceIndex, dispatchRaysIndirectBuffer] : m_deviceDispatchRaysIndirectBuffers)
                {
                    dispatchRaysIndirectBuffer->Build(shaderTable->GetDeviceRayTracingShaderTable(deviceIndex).get());
                }
            }

        private:
            //! A DeviceMask denoting on which devices a device-specific SingleDeviceDispatchItem should be generated
            MultiDevice::DeviceMask m_deviceMask{ MultiDevice::DefaultDevice };
            //! A map of all device-specific DispatchRaysIndirectBuffer, indexed by the device index
            AZStd::unordered_map<int, Ptr<DispatchRaysIndirectBuffer>> m_deviceDispatchRaysIndirectBuffers;
        };
    } // namespace RHI
} // namespace AZ