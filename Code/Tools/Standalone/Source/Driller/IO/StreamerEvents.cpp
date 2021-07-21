/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Woodpecker_precompiled.h"

#include "StreamerEvents.h"

#include "StreamerDataAggregator.hxx"

namespace Driller
{
    void    StreamerMountDeviceEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        aggr->m_devices.push_back(&m_deviceData);
    }

    void    StreamerMountDeviceEvent::StepBackward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::DeviceArrayType::iterator it = AZStd::find(aggr->m_devices.begin(), aggr->m_devices.end(), &m_deviceData);
        if (it != aggr->m_devices.end())   // we can potentially not have registered the device if we did NOT capture the full state! TODO: warn about this
        {
            aggr->m_devices.erase(it);
        }
    }

    void    StreamerUnmountDeviceEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        for (StreamerDataAggregator::DeviceArrayType::iterator it = aggr->m_devices.begin(); it != aggr->m_devices.end(); ++it)
        {
            if ((*it)->m_id == m_deviceId)
            {
                m_unmountedDeviceData = *it;
                aggr->m_devices.erase(it);
                break;
            }
        }
    }

    void    StreamerUnmountDeviceEvent::StepBackward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        aggr->m_devices.push_back(m_unmountedDeviceData);
    }

    void    StreamerRegisterStreamEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        aggr->m_streams.insert(AZStd::make_pair(m_streamData.m_id, &m_streamData));
    }

    void    StreamerRegisterStreamEvent::StepBackward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        aggr->m_streams.erase(m_streamData.m_id);
    }

    void    StreamerUnregisterStreamEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::StreamMapType::iterator it = aggr->m_streams.find(m_streamId);
        if (it != aggr->m_streams.end())
        {
            m_removedStreamData = it->second;
            aggr->m_streams.erase(it);
        }
    }

    void    StreamerUnregisterStreamEvent::StepBackward(Aggregator* data)
    {
        if (m_removedStreamData != nullptr)
        {
            StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
            aggr->m_streams.insert(AZStd::make_pair(m_streamId, m_removedStreamData));
        }
    }

    void    StreamerReadCacheHit::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        (void)aggr; // Add to read cache hit map
    }

    void    StreamerReadCacheHit::StepBackward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        (void)aggr; // Remove from read cache hit map
    }

    void    StreamerAddRequestEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        aggr->m_requests.insert(AZStd::make_pair(m_requestData.m_id, &m_requestData));
    }

    void    StreamerAddRequestEvent::StepBackward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        aggr->m_requests.erase(m_requestData.m_id);
    }

    void    StreamerCompleteRequestEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::RequestMapType::iterator it = aggr->m_requests.find(m_requestId);
        if (it != aggr->m_requests.end())
        {
            m_removedRequest = it->second;
            m_oldState = m_removedRequest->m_completeState;
            m_removedRequest->m_completeState = m_state;
            aggr->m_requests.erase(it);
        }
        else
        {
            // TODO warn, this is possible if we did not capture the full state from the beginning
        }
    }

    void    StreamerCompleteRequestEvent::StepBackward(Aggregator* data)
    {
        if (m_removedRequest != nullptr)
        {
            StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
            m_removedRequest->m_completeState = m_oldState;
            aggr->m_requests.insert(AZStd::make_pair(m_requestId, m_removedRequest));
        }
    }

    void    StreamerCancelRequestEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::RequestMapType::iterator it = aggr->m_requests.find(m_requestId);
        if (it != aggr->m_requests.end())
        {
            m_cancelledRequestData = it->second;
            aggr->m_requests.erase(it);
        }
    }

    void    StreamerCancelRequestEvent::StepBackward(Aggregator* data)
    {
        if (m_cancelledRequestData != nullptr)
        {
            StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
            aggr->m_requests.insert(AZStd::make_pair(m_requestId, m_cancelledRequestData));
        }
    }


    void    StreamerRescheduleRequestEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::RequestMapType::iterator it = aggr->m_requests.find(m_requestId);
        if (it != aggr->m_requests.end())
        {
            m_rescheduledRequestData = it->second;
            m_oldDeadline = m_rescheduledRequestData->m_deadline;
            m_oldPriority = m_rescheduledRequestData->m_priority;
            m_rescheduledRequestData->m_deadline = m_newDeadline;
            m_rescheduledRequestData->m_priority = m_newPriority;
        }
        else
        {
            // TODO warn, this is possible if we did not capture the full state from the beginning
        }
    }

    void    StreamerRescheduleRequestEvent::StepBackward(Aggregator* data)
    {
        (void)data;
        if (m_rescheduledRequestData != nullptr)
        {
            m_rescheduledRequestData->m_deadline = m_oldDeadline;
            m_rescheduledRequestData->m_priority = m_oldPriority;
        }
    }

    void    StreamerOperationStartEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::StreamMapType::iterator it = aggr->m_streams.find(m_streamId);
        if (it != aggr->m_streams.end())
        {
            m_stream = it->second;
            m_previousOperation = m_stream->m_operation;
            m_stream->m_operation = &m_operation;
        }
        else
        {
            // TODO warn in a smart way too many warnings while scrubbing
            //AZ_Warning("Streamer Driller",false,"Operation could not find a stream 0x%08x this can be ok deoending on the streamer mode (if it captures the inital state or not)!",m_streamId);
        }
    }

    void    StreamerOperationStartEvent::StepBackward(Aggregator* data)
    {
        (void)data;
        if (m_stream != nullptr)
        {
            m_stream->m_operation = m_previousOperation;
        }
    }

    void    StreamerOperationCompleteEvent::StepForward(Aggregator* data)
    {
        StreamerDataAggregator* aggr = static_cast<StreamerDataAggregator*>(data);
        StreamerDataAggregator::StreamMapType::iterator it = aggr->m_streams.find(m_streamId);
        if (it !=  aggr->m_streams.end())
        {
            m_stream = it->second;
            m_stream->m_operation->m_bytesTransferred = m_bytesTransferred;
        }
    }

    void    StreamerOperationCompleteEvent::StepBackward(Aggregator* data)
    {
        (void)data;
        if (m_stream != nullptr)
        {
            m_stream->m_operation->m_bytesTransferred = 0;
        }
    }
} // namespace Driller
