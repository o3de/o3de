/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <RHI/ResourcePoolResolver.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace DX12
    {
        class Device;
        class QueryPool;
        class Buffer;

        /**
        *   Resolve queries from a QueryPool into a buffer.
        */
        class QueryPoolResolver final
            : public ResourcePoolResolver
        {
            using Base = ResourcePoolResolver;
        public:
            AZ_RTTI(QueryPoolResolver, "{90CD6495-0758-4ED2-8B95-D5C5DE80CEBC}", Base);
            AZ_CLASS_ALLOCATOR(QueryPoolResolver, AZ::SystemAllocator, 0);

            /// Request for a resolving part of the QueryPool into a buffer.
            struct ResolveRequest
            {
                uint32_t m_firstQuery = 0;
                uint32_t m_queryCount = 0;
                ID3D12Resource* m_resolveBuffer = nullptr;
                uint64_t m_offset = 0;
                D3D12_QUERY_TYPE m_queryType = D3D12_QUERY_TYPE_OCCLUSION;
            };

            QueryPoolResolver(Device& device, QueryPool& queryPool);
            virtual ~QueryPoolResolver() = default;

            /*  Queues a request for resolving a QueryPool. This will be processed during the resolve phase.
            *   @param request The request to be queue.
            *   @return Returns a fence value used for checking if the resolve has finished.
            */
            uint64_t QueueResolveRequest(const ResolveRequest& request);

            /// Check if a certan request has finished.
            bool IsResolveFinished(uint64_t fenceValue);
            /// Blocks until the request have finished.
            void WaitForResolve(uint64_t fenceValue);

        private:

            void Compile(Scope& scope);
            void Resolve(CommandList& commandList) const override;
            void Deactivate() override;

            QueryPool& m_queryPool; ///< Query pool that is being resolved.
            Device& m_device;
            AZStd::vector<ResolveRequest> m_resolveRequests; /// List of requests to be resolved.

            RHI::Ptr<FenceImpl> m_resolveFence; ///< Fence used for checking if a request has finished.
        };
    }
}