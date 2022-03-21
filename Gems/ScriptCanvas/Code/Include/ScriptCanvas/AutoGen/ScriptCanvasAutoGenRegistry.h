/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <string>
#include <unordered_map>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    class IScriptCanvasFunctionRegistry
    {
    public:
        virtual void Reflect(AZ::ReflectContext* context) = 0;
    };

    class AutoGenRegistry
    {
    public:
        AutoGenRegistry() = default;
        ~AutoGenRegistry();

        static AutoGenRegistry* GetInstance();
        static void ReflectFunctions(AZ::ReflectContext* context);

        void RegisterFunction(const char* className, IScriptCanvasFunctionRegistry* registry);

        std::unordered_map<std::string, IScriptCanvasFunctionRegistry*> m_functions;
    };
} // namespace ScriptCanvas
