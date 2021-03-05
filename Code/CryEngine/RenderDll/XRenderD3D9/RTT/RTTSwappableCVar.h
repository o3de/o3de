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

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#ifndef AZ_RENDERDLL_RTTSWAPPABLECVAR_H
#define AZ_RENDERDLL_RTTSWAPPABLECVAR_H

#pragma once


#include "IConsole.h"

namespace AzRTT
{
    //! helper class to make it easy to swap cvars at runtime
    template <typename T>
    struct SwappableCVar
    {
        SwappableCVar() : m_cvar(nullptr), m_variable(nullptr) {}

        SwappableCVar(const char* cvarName, T* variable)
            : m_variable(variable)
        {
            m_cvar = gEnv->pConsole->GetCVar(cvarName);
        }

        //! backup the cvar value, usually done before changing it 
        void Backup() 
        {
            m_value = Get(); 
        }

        //!  disable the cvar value after backing it up so Restore works 
        void Disable() 
        {
            Backup();
            Set(m_disabledValue);
        }

        //!  restore a cvar value if swapped 
        void Restore()
        {
            if (m_swapped)
            {
                AZ_Assert(m_cvar, "SwappableCVar not found");
                Set(m_value);
                m_swapped = false;
            }
        }

        //!  get the cvar value 
        T Get()
        {
            if (m_variable != nullptr)
            {
                return *m_variable;
            }
            else
            {
                return T();
            }
        }

        //!  set the cvar value 
        void Set(T newValue) 
        { 
            if (m_variable != nullptr)
            {
                *m_variable = newValue;
            }
            else
            {
                AZ_Assert(m_cvar, "SwappableCVar not found");
                m_cvar->Set(newValue);  // ForceSet for release mode?
            }
        }

        //!  swap the cvar value, backing up the current value so we can swap back 
        void Swap(T newValue) 
        { 
            Backup(); 
            Set(newValue); 
            m_swapped = true; 
        }


        ICVar *m_cvar = nullptr;
        T m_value;
        T* m_variable;
        T m_disabledValue;
        bool m_swapped = false;
    };

    //! int specialization of Get calls GetIVal()
    template<>
    inline int SwappableCVar<int>::Get() 
    { 
        if (m_variable != nullptr)
        {
            return *m_variable;
        }
        else
        {
            AZ_Assert(m_cvar, "SwappableCVar not found");
            return m_cvar->GetIVal();
        }
    }

    //! int Disable specialization uses a value of 0 
    template<> 
    inline void SwappableCVar<int>::Disable() 
    { 
        Backup(); 
        Set(0); 
    }

    //! float specialization of Get calls GetFVal()
    template<>
    inline float SwappableCVar<float>::Get() 
    { 
        if (m_variable != nullptr)
        {
            return *m_variable;
        }
        else
        {
            AZ_Assert(m_cvar, "SwappableCVar not found");
            return m_cvar->GetFVal();
        }
    }

    //! float Disable specialization uses a value of 0.f
    template<> 
    inline void SwappableCVar<float>::Disable() 
    { 
        Backup(); 
        Set(0.f); 
    }
}

#endif // define AZ_RENDERDLL_RTTSWAPPABLECVAR_H

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED