/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/fixed_string.h>
#include "ScriptCanvasAutoGenRegistry.h"

namespace ScriptCanvas
{
    static constexpr const char ScriptCanvasAutoGenRegistryName[] = "AutoGenRegistry";
    static constexpr int MaxMessageLength = 4096;
    static constexpr const char ScriptCanvasAutoGenFunctionRegistrationWarningMessage[] = "[Warning] Functions name %s is occupied already, ignore functions registration.\n";

    AutoGenRegistry::~AutoGenRegistry()
    {
        m_functions.clear();
    }

    AutoGenRegistry* AutoGenRegistry::GetInstance()
    {
        // Use static object so each module will keep its own registry collection
        static AutoGenRegistry s_autogenRegistry;

        return &s_autogenRegistry;
    }

    void AutoGenRegistry::Reflect(AZ::ReflectContext* context)
    {
        ReflectFunctions(context);
        // Expand here to add more reflections
        // ...
    }

    void AutoGenRegistry::ReflectFunctions(AZ::ReflectContext* context)
    {
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_functions)
            {
                if (iter.second)
                {
                    iter.second->Reflect(context);
                }
            }
        }
    }

    void AutoGenRegistry::RegisterFunction(const char* functionName, IScriptCanvasFunctionRegistry* registry)
    {
        if (m_functions.find(functionName) != m_functions.end())
        {
            AZ::Debug::Platform::OutputToDebugger(
                ScriptCanvasAutoGenRegistryName,
                AZStd::fixed_string<MaxMessageLength>::format(ScriptCanvasAutoGenFunctionRegistrationWarningMessage, functionName).c_str());
        }
        else if (registry != nullptr)
        {
            m_functions.emplace(functionName, registry);
        }
    }

    void AutoGenRegistry::UnregisterFunction(const char* functionName)
    {
        m_functions.erase(functionName);
    }
} // namespace ScriptCanvas
