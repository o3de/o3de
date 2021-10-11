/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>

#include <GraphCanvas/Utils/StateControllers/StateController.h>

namespace GraphCanvas
{
    template<class T>
    class StackStateController
        : public StateController<T>
    {
    public:
        StackStateController(const T& defaultValue)
            : StateController<T>(defaultValue)
        {
        }
        
        ~StackStateController()
        {
            while (!m_states.empty())
            {
                auto mapIter = m_states.rbegin();
                StateSetter<T>* stateSetter = mapIter->first;
                
                bool releasedState = stateSetter->RemoveStateController(this);

                AZ_Error("StackStateController", releasedState, "Failed to properly release StateSetter state from owning StateController.");
                if (!releasedState)
                {
                    m_states.erase(mapIter.base());
                }
            }
            
            m_states.clear();
        }

        bool HasState() const override
        {
            return !m_states.empty();
        }

    protected:
        
        bool OnPushState(StateSetter<T>* stateSetter, const T& state) override
        {
            if (OnReleaseState(stateSetter))
            {
                AZ_Error("StackedStateController",true,"Trying to set two values from a single state setter.");            
            }
            
            m_states.emplace_back(stateSetter,state);
            
            return true;
        }
        
        bool OnReleaseState(StateSetter<T>* stateSetter) override
        {
            bool releasedValue = false;

            for (auto stateIter = m_states.begin(); stateIter != m_states.end(); ++stateIter)
            {
                if (stateIter->first == stateSetter)
                {
                    m_states.erase(stateIter);
                    releasedValue = true;
                    break;
                }
            }
            
            return releasedValue;
        }

        const T& GetCalculatedState() const override
        {
            return m_states.back().second;
        }
    
    private:
    
        AZStd::vector< AZStd::pair< StateSetter<T>*, T> > m_states;
    };
}
