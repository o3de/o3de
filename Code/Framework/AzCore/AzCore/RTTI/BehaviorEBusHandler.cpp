/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    AZStd::string BehaviorEBusHandler::GetScriptPath() const
    {
#if !defined(AZ_RELEASE_BUILD) // m_scriptPath is only available in non-Release mode
        return m_scriptPath;
#else
        return{};
#endif
    }

    void BehaviorEBusHandler::SetScriptPath(const char* scriptPath)
    {
#if !defined(AZ_RELEASE_BUILD) // m_scriptPath is only available in non-Release mode
        m_scriptPath = scriptPath;
#else
        AZ_UNUSED(scriptPath);
#endif
    }

    //=========================================================================
    // InstallGenericHook
    //=========================================================================
    bool BehaviorEBusHandler::InstallGenericHook(int index, GenericHookType hook, void* userData)
    {
        if (index != -1)
        {
            // Check parameters
            m_events[index].m_isFunctionGeneric = true;
            m_events[index].m_function = reinterpret_cast<void*>(hook);
            m_events[index].m_userData = userData;
            return true;
        }

        return false;
    }

    //=========================================================================
    // InstallGenericHook
    //=========================================================================
    bool BehaviorEBusHandler::InstallGenericHook(const char* name, GenericHookType hook, void* userData)
    {
        return InstallGenericHook(GetFunctionIndex(name), hook, userData);
    }

    //=========================================================================
    // GetEvents
    //=========================================================================
    const BehaviorEBusHandler::EventArray& BehaviorEBusHandler::GetEvents() const
    {
        return m_events;
    }

    bool BehaviorEBusHandler::BusForwarderEvent::HasResult() const
    {
        return !m_parameters.empty() && !m_parameters.front().m_typeId.IsNull() && m_parameters.front().m_typeId != azrtti_typeid<void>();
    }
} // namespace AZ
