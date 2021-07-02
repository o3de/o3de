/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ProfilerEvents.h"

#include "ProfilerDataAggregator.hxx"

namespace Driller
{
    //=========================================================================
    // ProfilerDrillerUpdateRegisterEvent::StepForward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerUpdateRegisterEvent::StepForward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);

        ProfilerDataAggregator::RegisterMapType::iterator it = aggr->m_registers.find(m_registerId);
        if (it != aggr->m_registers.end())
        {
            m_register = it->second;
            m_previousSample = m_register->m_lastUpdate;
            m_register->m_lastUpdate = this;
        }
    }

    //=========================================================================
    // ProfilerDrillerUpdateRegisterEvent::PreProcess
    //=========================================================================
    void ProfilerDrillerUpdateRegisterEvent::PreComputeForward(ProfilerDrillerNewRegisterEvent* newEvt)
    {
        m_register = newEvt;
        m_previousSample = m_register->m_lastPrecomputed;
        m_register->m_lastPrecomputed = this;
    }

    //=========================================================================
    // ProfilerDrillerUpdateRegisterEvent::StepBackward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerUpdateRegisterEvent::StepBackward(Aggregator* data)
    {
        (void)data;

        if (m_register)
        {
            m_register->m_lastUpdate = m_previousSample;
        }
    }

    //=========================================================================
    // ProfilerDrillerNewRegisterEvent::StepForward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerNewRegisterEvent::StepForward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        aggr->m_registers.insert(AZStd::make_pair(m_registerInfo.m_id, this));
    }

    //=========================================================================
    // ProfilerDrillerNewRegisterEvent::StepBackward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerNewRegisterEvent::StepBackward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        // NOTE: we can store the iterator in this register class, this way we can avoid this search
        // as of now search should be fast (at least as fast as insert) plus I am avoiding including "ProfilerDataAggregator.hxx"
        // into the header file.
        aggr->m_registers.erase(m_registerInfo.m_id);
    }

    //=========================================================================
    // ProfilerDrillerEnterThreadEvent::StepForward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerEnterThreadEvent::StepForward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        aggr->m_threads.insert(AZStd::make_pair(m_threadId, this));
    }

    //=========================================================================
    // ProfilerDrillerEnterThreadEvent::StepBackward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerEnterThreadEvent::StepBackward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        // NOTE: we can store the iterator in this register class, this way we can avoid this search
        // as of now search should be fast (at least as fast as insert) plus I am avoiding including "ProfilerDataAggregator.hxx"
        // into the header file.
        aggr->m_threads.erase(m_threadId);
    }

    //=========================================================================
    // ProfilerDrillerExitThreadEvent::StepForward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerExitThreadEvent::StepForward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        ProfilerDataAggregator::ThreadMapType::iterator it = aggr->m_threads.find(m_threadId);
        m_threadData = it->second;
        aggr->m_threads.erase(it);
    }

    //=========================================================================
    // ProfilerDrillerEnterThreadEvent::StepBackward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerExitThreadEvent::StepBackward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        aggr->m_threads.insert(AZStd::make_pair(m_threadId, m_threadData));
    }

    //=========================================================================
    // ProfilerDrillerRegisterSystemEvent::StepForward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerRegisterSystemEvent::StepForward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        aggr->m_systems.insert(AZStd::make_pair(m_systemId, this));
    }

    //=========================================================================
    // ProfilerDrillerEnterThreadEvent::StepBackward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerRegisterSystemEvent::StepBackward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        // NOTE: we can store the iterator in this register class, this way we can avoid this search
        // as of now search should be fast (at least as fast as insert) plus I am avoiding including "ProfilerDataAggregator.hxx"
        // into the header file.
        aggr->m_threads.erase(m_systemId);
    }

    //=========================================================================
    // ProfilerDrillerUnregisterSystemEvent::StepForward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerUnregisterSystemEvent::StepForward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        ProfilerDataAggregator::SystemMapType::iterator it = aggr->m_systems.find(m_systemId);
        m_systemData = it->second;
        aggr->m_systems.erase(it);
    }

    //=========================================================================
    // ProfilerDrillerUnregisterSystemEvent::StepBackward
    // [6/3/2013]
    //=========================================================================
    void ProfilerDrillerUnregisterSystemEvent::StepBackward(Aggregator* data)
    {
        ProfilerDataAggregator* aggr = static_cast<ProfilerDataAggregator*>(data);
        aggr->m_systems.insert(AZStd::make_pair(m_systemId, m_systemData));
    }
} // namespace Driller
