#pragma once

#include <Atom/RHI/DispatchRaysIndirectBuffer.h>

namespace AZ
{
    namespace Null
    {
        class DispatchRaysIndirectBuffer : public RHI::DispatchRaysIndirectBuffer
        {
        public:
            AZ_RTTI(AZ::Null::DispatchRaysIndirectBuffer, "{BE747193-DF9E-49ED-8EC8-2348044F11B2}", RHI::DispatchRaysIndirectBuffer)
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
    } // namespace Null
} // namespace AZ