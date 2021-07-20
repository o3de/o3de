/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ProfilerDataParser.h"
#include "ProfilerDataAggregator.hxx"
#include "ProfilerEvents.h"

namespace Driller
{
    AZ::Debug::DrillerHandlerParser* ProfilerDrillerHandlerParser::OnEnterTag(AZ::u32 tagName)
    {
        AZ_Assert(m_data, "You must set a valid memory aggregator before we can process the data!");

        if (tagName == AZ_CRC("NewRegister", 0xf0f2f287))
        {
            m_subTag = ST_NEW_REGSTER;
            m_data->AddEvent(aznew ProfilerDrillerNewRegisterEvent());
            return this;
        }
        else if (tagName == AZ_CRC("UpdateRegister", 0x6c00b890))
        {
            m_subTag = ST_UPDATE_REGSTER;
            m_data->AddEvent(aznew ProfilerDrillerUpdateRegisterEvent());
            return this;
        }
        else if (tagName == AZ_CRC("ThreadEnter", 0x60e4acfb))
        {
            m_subTag = ST_ENTER_THREAD;
            m_data->AddEvent(aznew ProfilerDrillerEnterThreadEvent());
            return this;
        }
        else if (tagName == AZ_CRC("OnThreadExit", 0x16042db9))
        {
            m_subTag = ST_EXIT_THREAD;
            m_data->AddEvent(aznew ProfilerDrillerExitThreadEvent());
            return this;
        }
        else if (tagName == AZ_CRC("RegisterSystem", 0x957739ef))
        {
            m_subTag = ST_REGISTER_SYSTEM;
            m_data->AddEvent(aznew ProfilerDrillerRegisterSystemEvent());
            return this;
        }
        else if (tagName == AZ_CRC("UnregisterSystem", 0xa20538e4))
        {
            m_subTag = ST_UNREGISTER_SYSTEM;
            m_data->AddEvent(aznew ProfilerDrillerUnregisterSystemEvent());
            return this;
        }
        else
        {
            m_subTag = ST_NONE;
        }
        return nullptr;
    }

    void ProfilerDrillerHandlerParser::OnExitTag(DrillerHandlerParser* handler, AZ::u32 tagName)
    {
        (void)tagName;
        if (handler != nullptr)
        {
            if (m_subTag != ST_NONE)
            {
                m_data->OnEventLoaded(m_data->GetEvents().back());
                m_subTag = ST_NONE; // we have only one level just go back to the default state
            }
        }
    }

    void ProfilerDrillerHandlerParser::OnData(const AZ::Debug::DrillerSAXParser::Data& dataNode)
    {
        AZ_Assert(m_data, "You must set a valid memory aggregator before we can process the data!");

        switch (m_subTag)
        {
        case ST_NEW_REGSTER:
        {
            ProfilerDrillerNewRegisterEvent* event = static_cast<ProfilerDrillerNewRegisterEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_registerInfo.m_id);
            }
            else if (dataNode.m_name == AZ_CRC("ThreadId", 0xd0fd9043))
            {
                dataNode.Read(event->m_registerInfo.m_threadId);
            }
            else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_registerInfo.m_name = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("Function", 0xcaae163d))
            {
                event->m_registerInfo.m_function = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("Line", 0xd114b4f6))
            {
                dataNode.Read(event->m_registerInfo.m_line);
            }
            else if (dataNode.m_name == AZ_CRC("SystemId", 0x0dfecf6f))
            {
                dataNode.Read(event->m_registerInfo.m_systemId);
            }
            else if (dataNode.m_name == AZ_CRC("Type", 0x8cde5729))
            {
                dataNode.Read(event->m_registerInfo.m_type);
            }
            else if (dataNode.m_name == AZ_CRC("Time", 0x6f949845))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_time);
            }
            else if (dataNode.m_name == AZ_CRC("ChildrenTime", 0x46162d3f))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_childrenTime);
            }
            else if (dataNode.m_name == AZ_CRC("Calls", 0xdaa35c8f))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_calls);
            }
            else if (dataNode.m_name == AZ_CRC("ChildrenCalls", 0x6a5a4618))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_childrenCalls);
            }
            else if (dataNode.m_name == AZ_CRC("ParentId", 0x856a684c))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_lastParentRegisterId);
            }
            else if (dataNode.m_name == AZ_CRC("Value1", 0xa2756c5a))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value1);
            }
            else if (dataNode.m_name == AZ_CRC("Value2", 0x3b7c3de0))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value2);
            }
            else if (dataNode.m_name == AZ_CRC("Value3", 0x4c7b0d76))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value3);
            }
            else if (dataNode.m_name == AZ_CRC("Value4", 0xd21f98d5))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value4);
            }
            else if (dataNode.m_name == AZ_CRC("Value5", 0xa518a843))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value5);
            }
        } break;
        case ST_UPDATE_REGSTER:
        {
            ProfilerDrillerUpdateRegisterEvent* event = static_cast<ProfilerDrillerUpdateRegisterEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_registerId);
            }
            else if (dataNode.m_name == AZ_CRC("Time", 0x6f949845))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_time);
            }
            else if (dataNode.m_name == AZ_CRC("ChildrenTime", 0x46162d3f))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_childrenTime);
            }
            else if (dataNode.m_name == AZ_CRC("Calls", 0xdaa35c8f))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_calls);
            }
            else if (dataNode.m_name == AZ_CRC("ChildrenCalls", 0x6a5a4618))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_childrenCalls);
            }
            else if (dataNode.m_name == AZ_CRC("ParentId", 0x856a684c))
            {
                dataNode.Read(event->m_registerData.m_timeData.m_lastParentRegisterId);
            }
            else if (dataNode.m_name == AZ_CRC("Value1", 0xa2756c5a))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value1);
            }
            else if (dataNode.m_name == AZ_CRC("Value2", 0x3b7c3de0))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value2);
            }
            else if (dataNode.m_name == AZ_CRC("Value3", 0x4c7b0d76))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value3);
            }
            else if (dataNode.m_name == AZ_CRC("Value4", 0xd21f98d5))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value4);
            }
            else if (dataNode.m_name == AZ_CRC("Value5", 0xa518a843))
            {
                dataNode.Read(event->m_registerData.m_valueData.m_value5);
            }
        } break;
        case ST_ENTER_THREAD:
        {
            ProfilerDrillerEnterThreadEvent* event = static_cast<ProfilerDrillerEnterThreadEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_threadId);
            }
            else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_threadName = dataNode.ReadPooledString();
            }
            else if (dataNode.m_name == AZ_CRC("CpuId", 0xdf558508))
            {
                dataNode.Read(event->m_cpuId);
            }
            else if (dataNode.m_name == AZ_CRC("Priority", 0x62a6dc27))
            {
                dataNode.Read(event->m_priority);
            }
            else if (dataNode.m_name == AZ_CRC("StackSize", 0x9cfaf35b))
            {
                dataNode.Read(event->m_stackSize);
            }
        } break;
        case ST_EXIT_THREAD:
        {
            ProfilerDrillerExitThreadEvent* event = static_cast<ProfilerDrillerExitThreadEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_threadId);
            }
        } break;
        case ST_REGISTER_SYSTEM:
        {
            ProfilerDrillerRegisterSystemEvent* event = static_cast<ProfilerDrillerRegisterSystemEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_systemId);
            }
            else if (dataNode.m_name == AZ_CRC("Name", 0x5e237e06))
            {
                event->m_name = dataNode.ReadPooledString();
            }
        } break;
        case ST_UNREGISTER_SYSTEM:
        {
            ProfilerDrillerUnregisterSystemEvent* event = static_cast<ProfilerDrillerUnregisterSystemEvent*>(m_data->GetEvents().back());
            if (dataNode.m_name == AZ_CRC("Id", 0xbf396750))
            {
                dataNode.Read(event->m_systemId);
            }
        } break;
        }
    }
} // namespace Driller
