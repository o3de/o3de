/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/Query.h>
#include <RHI/QueryPool.h>
#include <RHI/QueryPoolResolver.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<QueryPool> QueryPool::Create()
        {
            return aznew QueryPool();
        }

        ID3D12QueryHeap * QueryPool::GetHeap() const
        {
            return m_queryHeap.get();
        }

        void QueryPool::OnQueryEnd(Query& query, D3D12_QUERY_TYPE type)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesToResolveMutex);
            m_queriesToResolve[static_cast<uint32_t>(type)].push_back(&query);
        }

        RHI::ResultCode QueryPool::InitInternal(RHI::Device& baseDevice, const RHI::QueryPoolDescriptor& descriptor)
        {
            auto& device = static_cast<Device&>(baseDevice);
            ID3D12DeviceX* dx12Device = device.GetDevice();
            Microsoft::WRL::ComPtr<ID3D12QueryHeap> heap;

            D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
            queryHeapDesc.Count = descriptor.m_queriesCount;
            queryHeapDesc.Type = ConvertQueryHeapType(descriptor.m_type);
            if (!device.AssertSuccess(dx12Device->CreateQueryHeap(&queryHeapDesc, IID_GRAPHICS_PPV_ARGS(heap.GetAddressOf()))))
            {
                return RHI::ResultCode::Fail;
            }

            m_queryHeap = heap.Get();
            m_queryHeap->SetName(L"QueryHeap");

            const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_READBACK);
            const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetQueryResultSize() * descriptor.m_queriesCount);

            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            if (!device.AssertSuccess(dx12Device->CreateCommittedResource(
                &heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(resource.GetAddressOf()))))
            {
                return RHI::ResultCode::Fail;
            }

            m_readBackBuffer = resource.Get();
            m_readBackBuffer->SetName(L"Readback");

            SetResolver(AZStd::make_unique<QueryPoolResolver>(baseDevice.GetDeviceIndex(), *this));
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode QueryPool::InitQueryInternal([[maybe_unused]] RHI::DeviceQuery& query)
        {
            // Nothing to do
            return RHI::ResultCode::Success;
        }

        RHI::ResultCode QueryPool::GetResultsInternal(uint32_t startIndex, uint32_t queryCount, uint64_t* results, [[maybe_unused]] uint32_t resultsCount, RHI::QueryResultFlagBits flags)
        {
            QueryPoolResolver* resolver = static_cast<QueryPoolResolver*>(GetResolver());
            // First check if the results for all the requested queries are available ...
            for (uint32_t i = startIndex; i < startIndex + queryCount; ++i)
            {
                auto* query = static_cast<const Query*>(GetQuery(RHI::QueryHandle(i)));
                AZ_Assert(query, "Null Query");
                if (RHI::CheckBitsAll(flags, RHI::QueryResultFlagBits::Wait))
                {
                    resolver->WaitForResolve(query->m_resultFenceValue);
                }
                else if(!resolver->IsResolveFinished(query->m_resultFenceValue))
                {
                    return RHI::ResultCode::NotReady;
                }
            }

            size_t resultSize = GetQueryResultSize();
            D3D12_RANGE readRange = { startIndex * resultSize, (startIndex + queryCount) * resultSize };
            uint8_t* bufferData = MapReadBackBuffer(readRange);
            if(!bufferData)
            {
                return RHI::ResultCode::Fail;
            }

            if (GetDescriptor().m_type == RHI::QueryType::PipelineStatistics)
            {
                CopyPipelineStatisticsResult(results, reinterpret_cast<D3D12_QUERY_DATA_PIPELINE_STATISTICS*>(bufferData), queryCount);
            }
            else
            {
                ::memcpy(results, bufferData, resultSize * queryCount);
            }

            UnmapReadBackBuffer();
            return RHI::ResultCode::Success;
        }

        void QueryPool::OnFrameEnd()
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_queriesToResolveMutex);
            QueryPoolResolver* resolver = static_cast<QueryPoolResolver*>(GetResolver());
            for (size_t i = 0; i < m_queriesToResolve.size(); ++i)
            {
                auto& queryList = m_queriesToResolve[i];
                if (queryList.empty())
                {
                    continue;
                }

                AZStd::vector<RHI::Interval> intervals = GetQueryIntervals(queryList);
                for (auto& interval : intervals)
                {
                    QueryPoolResolver::ResolveRequest request;
                    request.m_firstQuery = interval.m_min;
                    request.m_queryCount = interval.m_max - interval.m_min + 1;
                    request.m_resolveBuffer = m_readBackBuffer.get();
                    request.m_queryType = static_cast<D3D12_QUERY_TYPE>(i);
                    request.m_offset = interval.m_min * GetQueryResultSize();
                    uint64_t fenceValue = resolver->QueueResolveRequest(request);
                    for (uint32_t j = interval.m_min; j <= interval.m_max; ++j)
                    {
                        auto* query = static_cast<Query*>(GetQuery(RHI::QueryHandle(j)));
                        AZ_Assert(query, "Null Query");
                        query->m_resultFenceValue = fenceValue;
                    }
                }

                queryList.clear();
            }

            Base::OnFrameEnd();
        }

        size_t QueryPool::GetQueryResultSize() const
        {
            return GetDescriptor().m_type == RHI::QueryType::PipelineStatistics ? sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS) : sizeof(uint64_t);
        }

        void QueryPool::CopyPipelineStatisticsResult(uint64_t* destination, D3D12_QUERY_DATA_PIPELINE_STATISTICS* source, uint32_t queryCount)
        {
            auto mask = GetDescriptor().m_pipelineStatisticsMask;
            // Check if all flags are enabled ...
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::All))
            {
                // Copy all data at the same time ...
                ::memcpy(destination, source, sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS) * queryCount);
                return;
            }

            size_t resultPos = 0;
            for (uint32_t i = 0; i < queryCount; ++i)
            {
                D3D12_QUERY_DATA_PIPELINE_STATISTICS& data = source[i];
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::IAVertices))
                {
                    destination[resultPos++] = data.IAVertices;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::IAPrimitives))
                {
                    destination[resultPos++] = data.IAPrimitives;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::VSInvocations))
                {
                    destination[resultPos++] = data.VSInvocations;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::GSInvocations))
                {
                    destination[resultPos++] = data.GSInvocations;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::GSPrimitives))
                {
                    destination[resultPos++] = data.GSPrimitives;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CInvocations))
                {
                    destination[resultPos++] = data.CInvocations;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CPrimitives))
                {
                    destination[resultPos++] = data.CPrimitives;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::PSInvocations))
                {
                    destination[resultPos++] = data.PSInvocations;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::HSInvocations))
                {
                    destination[resultPos++] = data.HSInvocations;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::DSInvocations))
                {
                    destination[resultPos++] = data.DSInvocations;
                }
                if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CSInvocations))
                {
                    destination[resultPos++] = data.CSInvocations;
                }
            }
        }

        uint8_t* QueryPool::MapReadBackBuffer(D3D12_RANGE readRange)
        {
            uint8_t* buffer = nullptr;
            if (!AssertSuccess(m_readBackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&buffer))))
            {
                AZ_Error("QueryPool", false, "Failed to map buffer for reading results");
                return nullptr;
            }

            // Set the offset
            buffer += readRange.Begin;
            return buffer;
        }

        void QueryPool::UnmapReadBackBuffer()
        {
            // Explicitly specify that nothing has been written to the unmapped buffer by setting
            // the range the same value
            static constexpr D3D12_RANGE InvalidRange = {0,0};
            m_readBackBuffer->Unmap(0, &InvalidRange);
        }

        void QueryPool::ShutdownInternal()
        {
            auto& device = static_cast<Device&>(GetDevice());
            device.QueueForRelease(m_queryHeap);
            m_queryHeap = nullptr;
            device.QueueForRelease(m_readBackBuffer);
            m_readBackBuffer = nullptr;
        }
    }
}
