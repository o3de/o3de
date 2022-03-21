/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>

#include "ScriptCanvasAutoGenRegistry.h"

namespace ScriptCanvas
{
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

    void AutoGenRegistry::RegisterFunction(const char* className, IScriptCanvasFunctionRegistry* registry)
    {
        if (m_functions.find(className) == m_functions.end() && registry != nullptr)
        {
            m_functions.emplace(className, registry);
        }
    }
} // namespace ScriptCanvas
