#pragma once

#include <Atom/RHI/DispatchRaysIndirectBuffer.h>

namespace AZ
{
    namespace Vulkan
    {
        class DispatchRaysIndirectBuffer : public RHI::DispatchRaysIndirectBuffer
        {
        public:
            AZ_RTTI(AZ::Vulkan::DispatchRaysIndirectBuffer, "{D6767937-A314-43A8-82BB-8F8E7D32A937}", RHI::DispatchRaysIndirectBuffer)
            DispatchRaysIndirectBuffer() = default;
            virtual ~DispatchRaysIndirectBuffer() = default;
            DispatchRaysIndirectBuffer(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer(DispatchRaysIndirectBuffer&&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&) = delete;
            DispatchRaysIndirectBuffer& operator=(const DispatchRaysIndirectBuffer&&) = delete;

            static RHI::Ptr<DispatchRaysIndirectBuffer> Create()
            {
                return aznew DispatchRaysIndirectBuffer;
            }

            void Init(RHI::BufferPool*, RHI::BufferPool*) override
            {
            }
            void Build(RHI::RayTracingShaderTable*) override
            {
            }
        };
    } // namespace Vulkan
} // namespace AZ