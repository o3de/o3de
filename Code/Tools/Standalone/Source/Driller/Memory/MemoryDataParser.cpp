/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "MemoryDataParser.h"
#include "MemoryDataAggregator.hxx"
#include "MemoryEvents.h"

namespace Driller
{
    AZ::Debug::DrillerHandlerParser* MemoryDrillerHandlerParser::OnEnterTag(AZ::u32 tagName)
    {
        AZ_Assert(m_data, "You must set a valid memory aggregator before we can process the data!");

        if (tagName == AZ_CRC("RegisterAllocator", 0x19f08114))
        {
            m_subTag = ST_REGISTER_ALLOCATOR;
            m_data->AddEvent(aznew MemoryDrillerRegisterAllocatorEvent());
            return this; // m_registerAllocatorHanler
        }
        else if (tagName == AZ_CRC("RegisterAllocation", 0x992a9780))
        {
            m_subTag = ST_REGISTER_ALLOCATION;
            m_data->AddEvent(aznew MemoryDrillerRegisterAllocationEvent());
            return this; // m_registerAllocation
        }
        else if (tagName == AZ_CRC("UnRegisterAllocation", 0xea5dc4cd))
        {
            m_subTag = ST_UNREGISTER_ALLOCATION;
            m_data->AddEvent(aznew MemoryDrillerUnregisterAllocationEvent());
            return this; // m_unregisterAllocation
        }
        else if (tagName == AZ_CRC("ResizeAllocation", 0x8a9c78dc))
        {
            m_subTag = ST_RESIZE_ALLOCATION;
            m_data->AddEvent(aznew MemoryDrillerResizeAllocationEvent());
            return this; // m_resizeAllocation
        }
        else
        {
            m_subTag = ST_NONE;
        }
        return NULL;
    }

    void MemoryDrillerHandlerParser::OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName)
    {
        (void)tagName;
        if (handler != NULL)
        {
            m_subTag = ST_NONE; // we have only one level just go back to the default state
        }
    }

    void MemoryDrillerHandlerParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        AZ_Assert(m_data, "You must set a valid memory aggregator before we can process the data!");

        switch (m_subTag)
        {
        case ST_NONE:
        {
            if (dataNode.m_name == AZ_CRC("UnregisterAllocator", 0xb2b54f93))
            {
                AZ::u64 allocatorId;
                dataNode.Read(allocatorId);
                MemoryDrillerUnregisterAllocatorEvent* event = aznew MemoryDrillerUnregisterAllocatorEvent();
                event->m_allocatorId = allocatorId;
                m_data->AddEvent(event);
            }
        } break;
        case ST_REGISTER_ALLOCATOR:
        {
            MemoryDrillerRegisterAllocatorEvent* event = static_cast<MemoryDrillerRegisterAllocatorEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_allocatorInfo.m_name = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_allocatorInfo.m_id);
            }
            else if (dataNode.m_name == AZ_CRC("Capacity", 0xb5e8b174))
            {
                dataNode.Read(event->m_allocatorInfo.m_capacity);
            }
            else if (dataNode.m_name == AZ_CRC("RecordsId", 0x7caaca88))
            {
                dataNode.Read(event->m_allocatorInfo.m_recordsId);
            }
            else if (dataNode.m_name == AZ_CRC("RecordsMode", 0x764c147a))
            {
                dataNode.Read(event->m_allocatorInfo.m_recordMode);
            }
            else if (dataNode.m_name == AZ_CRC("NumStackLevels", 0xad9cff15))
            {
                dataNode.Read(event->m_allocatorInfo.m_numStackLevels);
            }
        } break;
        case ST_REGISTER_ALLOCATION:
        {
            MemoryDrillerRegisterAllocationEvent* event = static_cast<MemoryDrillerRegisterAllocationEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("RecordsId", 0x7caaca88))
            {
                dataNode.Read(event->m_allocationInfo.m_recordsId);
            }
            else if (dataNode.m_name == AZ_CRC("Address", 0x0d4e6f81))
            {
                dataNode.Read(event->m_address);
            }
            else if (dataNode.m_name == AZ_CRC("Alignment", 0x2cce1e5c))
            {
                dataNode.Read(event->m_allocationInfo.m_alignment);
            }
            else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
            {
                dataNode.Read(event->m_allocationInfo.m_size);
            }
            else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_allocationInfo.m_name = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("FileName", 0x3c0be965))
            {
                event->m_allocationInfo.m_fileName = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("FileLine", 0xb33c2395))
            {
                dataNode.Read(event->m_allocationInfo.m_fileLine);
            }
            else if (dataNode.m_name == AZ_CRC("Stack", 0x41a87b6a))
            {
                // TODO: we can pool that stack memory
                event->m_allocationInfo.m_stackFrames = reinterpret_cast<AZ::u64*>(azmalloc(dataNode.m_dataSize));
                dataNode.Read(event->m_allocationInfo.m_stackFrames, dataNode.m_dataSize);
            }
        } break;
        case ST_UNREGISTER_ALLOCATION:
        {
            MemoryDrillerUnregisterAllocationEvent* event = static_cast<MemoryDrillerUnregisterAllocationEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("RecordsId", 0x7caaca88))
            {
                dataNode.Read(event->m_recordsId);
            }
            else if (dataNode.m_name == AZ_CRC("Address", 0x0d4e6f81))
            {
                dataNode.Read(event->m_address);
            }
        } break;
        case ST_RESIZE_ALLOCATION:
        {
            MemoryDrillerResizeAllocationEvent* event = static_cast<MemoryDrillerResizeAllocationEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("RecordsId", 0x7caaca88))
            {
                dataNode.Read(event->m_recordsId);
            }
            else if (dataNode.m_name == AZ_CRC("Address", 0x0d4e6f81))
            {
                dataNode.Read(event->m_address);
            }
            else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
            {
                dataNode.Read(event->m_newSize);
            }
        } break;
        }
    }
} // namespace Driller
