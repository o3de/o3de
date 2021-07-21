/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Woodpecker_precompiled.h"

#include "StreamerDataAggregator.hxx"

namespace Driller
{
    AZ::Debug::DrillerHandlerParser* StreamerDrillerHandlerParser::OnEnterTag(AZ::u32 tagName)
    {
        AZ_Assert(m_data, "You must set a valid memory aggregator before we can process the data!");

        if (tagName == AZ_CRC("OnDeviceMounted", 0xc6bdd55e))
        {
            m_subTag = ST_DEVICE_MOUNTED;
            m_data->AddEvent(aznew StreamerMountDeviceEvent());
            return this;
        }
        else if (tagName == AZ_CRC("OnRegisterStream", 0x893513c1))
        {
            m_subTag = ST_STREAM_REGISTER;
            m_data->AddEvent(aznew StreamerRegisterStreamEvent());
            return this;
        }
        else if (tagName == AZ_CRC("OnReadCacheHit", 0xd4535712))
        {
            m_subTag = ST_READ_CACHE_HIT;
            if (m_allowCacheHitsInReportedStream)
            {
                m_data->AddEvent(aznew StreamerReadCacheHit());
            }
            return this;
        }
        else if (tagName == AZ_CRC("OnAddRequest", 0xee41c96e))
        {
            m_subTag = ST_REQUEST_ADD;
            m_data->AddEvent(aznew StreamerAddRequestEvent());
            return this;
        }
        else if (tagName == AZ_CRC("OnCompleteRequest", 0x7f6b66f7))
        {
            m_subTag = ST_REQUEST_COMPLETE;
            m_data->AddEvent(aznew StreamerCompleteRequestEvent());
            return this;
        }
        else if (tagName == AZ_CRC("OnRescheduleRequest", 0x883b3e85))
        {
            m_subTag = ST_REQUEST_RESCHEDULE;
            m_data->AddEvent(aznew StreamerRescheduleRequestEvent());
            return this;
        }
        else if (tagName == AZ_CRC("OnRead", 0xd7714b7b))
        {
            m_subTag = ST_OPERATION_READ;
            m_data->AddEvent(aznew StreamerOperationStartEvent(Streamer::SOP_READ));
            return this;
        }
        else if (tagName == AZ_CRC("OnReadComplete", 0x0efa014b))
        {
            m_subTag = ST_OPERATION_READ_COMPLETE;
            m_data->AddEvent(aznew StreamerOperationCompleteEvent(Streamer::SOP_READ));
            return this;
        }
        else if (tagName == AZ_CRC("OnWrite", 0x6925001a))
        {
            m_subTag = ST_OPERATION_WRITE;
            m_data->AddEvent(aznew StreamerOperationStartEvent(Streamer::SOP_WRITE));
            return this;
        }
        else if (tagName == AZ_CRC("OnWriteComplete", 0x6c5f7c79))
        {
            m_subTag = ST_OPERATION_WRITE_COMPLETE;
            m_data->AddEvent(aznew StreamerOperationCompleteEvent(Streamer::SOP_WRITE));
            return this;
        }
        else if (tagName == AZ_CRC("OnCompressorRead", 0xbd093b22))
        {
            m_subTag = ST_OPERATION_COMPRESSOR_READ;
            m_data->AddEvent(aznew StreamerOperationStartEvent(Streamer::SOP_COMPRESSOR_READ));
            return this;
        }
        else if (tagName == AZ_CRC("OnCompressorReadComplete", 0x9c08d9cd))
        {
            m_subTag = ST_OPERATION_COMPRESSOR_READ_COMPLETE;
            m_data->AddEvent(aznew StreamerOperationCompleteEvent(Streamer::SOP_COMPRESSOR_READ));
            return this;
        }
        else if (tagName == AZ_CRC("OnCompressorWrite", 0x7bf8913a))
        {
            m_subTag = ST_OPERATION_COMPRESSOR_WRITE;
            m_data->AddEvent(aznew StreamerOperationStartEvent(Streamer::SOP_COMPRESSOR_WRITE));
            return this;
        }
        else if (tagName == AZ_CRC("OnCompressorWriteComplete", 0x6816a8b4))
        {
            m_subTag = ST_OPERATION_COMPRESSOR_WRITE_COMPLETE;
            m_data->AddEvent(aznew StreamerOperationCompleteEvent(Streamer::SOP_COMPRESSOR_WRITE));
            return this;
        }
        else
        {
            m_subTag = ST_NONE;
        }
        return NULL;
    }

    void StreamerDrillerHandlerParser::OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName)
    {
        (void)tagName;
        if (handler != nullptr)
        {
            m_subTag = ST_NONE; // we have only one level just go back to the default state
        }
    }

    void StreamerDrillerHandlerParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        AZ_Assert(m_data, "You must set a valid memory aggregator before we can process the data!");
        (void)dataNode;
        switch (m_subTag)
        {
        case ST_NONE:
        {
            if (dataNode.m_name == AZ_CRC("OnDeviceUnmounted", 0x7395545a))
            {
                StreamerUnmountDeviceEvent* event = aznew StreamerUnmountDeviceEvent();
                dataNode.Read(event->m_deviceId);
                m_data->AddEvent(event);
            }
            else if (dataNode.m_name == AZ_CRC("OnUnregisterStream", 0x3374d0cb))
            {
                StreamerUnregisterStreamEvent* event = aznew StreamerUnregisterStreamEvent();
                dataNode.Read(event->m_streamId);
                m_data->AddEvent(event);
            }
            else if (dataNode.m_name == AZ_CRC("OnCancelRequest", 0x89d4ea74))
            {
                StreamerCancelRequestEvent* event = aznew StreamerCancelRequestEvent();
                dataNode.Read(event->m_requestId);
                m_data->AddEvent(event);
            }
        } break;
        case ST_DEVICE_MOUNTED:
        {
            StreamerMountDeviceEvent* event = static_cast<StreamerMountDeviceEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("DeviceId", 0x383bcd03))
            {
                dataNode.Read(event->m_deviceData.m_id);
            }
            else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_deviceData.m_name = dataNode.ReadPooledString();
            }
        } break;
        case ST_STREAM_REGISTER:
        {
            StreamerRegisterStreamEvent* event = static_cast<StreamerRegisterStreamEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("DeviceId", 0x383bcd03))
            {
                dataNode.Read(event->m_streamData.m_deviceId);
            }
            else if (dataNode.m_name == AZ_CRC("StreamId", 0x7597546f))
            {
                dataNode.Read(event->m_streamData.m_id);
            }
            else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_streamData.m_name = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("Flags", 0x0b0541ba))
            {
                dataNode.Read(event->m_streamData.m_flags);
            }
            else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
            {
                dataNode.Read(event->m_streamData.m_size);
            }
            else if (dataNode.m_name == AZ_CRC("IsCompressed", 0xdd32876c))
            {
                dataNode.Read(event->m_streamData.m_isCompressed);
            }
        } break;
        case ST_READ_CACHE_HIT:
        {
            if (m_allowCacheHitsInReportedStream)
            {
                StreamerReadCacheHit* event = static_cast<StreamerReadCacheHit*>(m_data->GetEvents().back());
                if (dataNode.m_name == AZ_CRC("StreamId", 0x7597546f))
                {
                    dataNode.Read(event->m_streamId);
                }
                else if (dataNode.m_name == AZ_CRC("Offset", 0x590acad0))
                {
                    dataNode.Read(event->m_offset);
                }
                else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
                {
                    dataNode.Read(event->m_size);
                }
                else if (dataNode.m_name == AZ_CRC("DebugName", 0x6c3ea120))
                {
                    event->m_debugName = dataNode.ReadPooledString();
                }
            }
        } break;
        case ST_REQUEST_ADD:
        {
            StreamerAddRequestEvent* event = static_cast<StreamerAddRequestEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("RequestId", 0x34e754a3))
            {
                dataNode.Read(event->m_requestData.m_id);
            }
            else if (dataNode.m_name == AZ_CRC("StreamId", 0x7597546f))
            {
                dataNode.Read(event->m_requestData.m_streamId);
            }
            else if (dataNode.m_name == AZ_CRC("Offset", 0x590acad0))
            {
                dataNode.Read(event->m_requestData.m_offset);
            }
            else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
            {
                dataNode.Read(event->m_requestData.m_size);
            }
            else if (dataNode.m_name == AZ_CRC("Deadline", 0xb74774f2))
            {
                dataNode.Read(event->m_requestData.m_deadline);
            }
            else if (dataNode.m_name == AZ_CRC("Priority", 0x62a6dc27))
            {
                dataNode.Read(event->m_requestData.m_priority);
            }
            else if (dataNode.m_name == AZ_CRC("Operation", 0x1981a66d))
            {
                dataNode.Read(event->m_requestData.m_operation);
            }
            else if (dataNode.m_name == AZ_CRC("DebugName", 0x6c3ea120))
            {
                event->m_requestData.m_debugName = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("Timestamp", 0xa5d6e63e))
            {
                dataNode.Read(event->m_timeStamp);
            }
        } break;
        case ST_REQUEST_COMPLETE:
        {
            StreamerCompleteRequestEvent* event = static_cast<StreamerCompleteRequestEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("RequestId", 0x34e754a3))
            {
                dataNode.Read(event->m_requestId);
            }
            else if (dataNode.m_name == AZ_CRC("State", 0xa393d2fb))
            {
                dataNode.Read(event->m_state);
            }
            else if (dataNode.m_name == AZ_CRC("Timestamp", 0xa5d6e63e))
            {
                dataNode.Read(event->m_timeStamp);
            }
        } break;
        case ST_REQUEST_RESCHEDULE:
        {
            StreamerRescheduleRequestEvent* event = static_cast<StreamerRescheduleRequestEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("RequestId", 0x34e754a3))
            {
                dataNode.Read(event->m_requestId);
            }
            else if (dataNode.m_name == AZ_CRC("NewDeadLine", 0x184cc661))
            {
                dataNode.Read(event->m_newDeadline);
            }
            else if (dataNode.m_name == AZ_CRC("NewPriority", 0xcdad6eb4))
            {
                dataNode.Read(event->m_newPriority);
            }
        } break;
        case ST_OPERATION_READ:
        case ST_OPERATION_WRITE:
        case ST_OPERATION_COMPRESSOR_READ:
        case ST_OPERATION_COMPRESSOR_WRITE:
        {
            StreamerOperationStartEvent* event = static_cast<StreamerOperationStartEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("StreamId", 0x7597546f))
            {
                dataNode.Read(event->m_streamId);
            }
            else if (dataNode.m_name == AZ_CRC("Size", 0xf7c0246a))
            {
                dataNode.Read(event->m_operation.m_size);
            }
            else if (dataNode.m_name == AZ_CRC("Offset", 0x590acad0))
            {
                dataNode.Read(event->m_operation.m_offset);
            }
            else if (dataNode.m_name == AZ_CRC("Timestamp", 0xa5d6e63e))
            {
                dataNode.Read(event->m_timeStamp);
            }
        } break;

        case ST_OPERATION_READ_COMPLETE:
        case ST_OPERATION_WRITE_COMPLETE:
        case ST_OPERATION_COMPRESSOR_READ_COMPLETE:
        case ST_OPERATION_COMPRESSOR_WRITE_COMPLETE:
        {
            StreamerOperationCompleteEvent* event = static_cast<StreamerOperationCompleteEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("StreamId", 0x7597546f))
            {
                dataNode.Read(event->m_streamId);
            }
            else if (dataNode.m_name == AZ_CRC("bytesTransferred", 0x35684b99))
            {
                dataNode.Read(event->m_bytesTransferred);
            }
            else if (dataNode.m_name == AZ_CRC("Timestamp", 0xa5d6e63e))
            {
                dataNode.Read(event->m_timeStamp);
            }
        } break;
        }
    }
} // namespace Driller
