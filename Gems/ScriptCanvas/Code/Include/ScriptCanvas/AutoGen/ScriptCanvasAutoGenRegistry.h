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

namespace AZ
{
    class ReflectContext;
}

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
        static void Reflect(AZ::ReflectContext* context);

        void RegisterFunction(const char* functionName, IScriptCanvasFunctionRegistry* registry);
        void UnregisterFunction(const char* functionName);

        std::unordered_map<std::string, IScriptCanvasFunctionRegistry*> m_functions;
    private:
        static void ReflectFunctions(AZ::ReflectContext* context);
    };
} // namespace ScriptCanvas
