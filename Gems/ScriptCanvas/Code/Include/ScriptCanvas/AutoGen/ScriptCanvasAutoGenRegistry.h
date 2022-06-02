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

//! Macros to self-register AutoGen node into ScriptCanvas
#define REGISTER_SCRIPTCANVAS_AUTOGEN_FUNCTION(LIBRARY)\
    static ScriptCanvas::LIBRARY##FunctionRegistry s_AutoGenFunctionRegistry;

//! AutoGen registry util macros
#define REFLECT_SCRIPTCANVAS_AUTOGEN(LIBRARY, CONTEXT)\
    ScriptCanvas::AutoGenRegistry::Reflect(CONTEXT, #LIBRARY);

namespace ScriptCanvas
{
    class IScriptCanvasFunctionRegistry
    {
    public:
        virtual void Reflect(AZ::ReflectContext* context) = 0;
    };

    //! AutoGenRegistry
    //! The registry contains all autogen functions which will be registered
    //! for ScriptCanvas
    class AutoGenRegistry
    {
    public:
        AutoGenRegistry() = default;
        ~AutoGenRegistry();

        static AutoGenRegistry* GetInstance();

        //! Reflect all AutoGen regiestries
        static void Reflect(AZ::ReflectContext* context);

        //! Reflect specified AutoGen registry by given name
        static void Reflect(AZ::ReflectContext* context, const char* registryName);

        //! Register function registry with its name
        void RegisterFunction(const char* functionName, IScriptCanvasFunctionRegistry* registry);

        //! Unregister function registry by using its name
        void UnregisterFunction(const char* functionName);

        std::unordered_map<std::string, IScriptCanvasFunctionRegistry*> m_functions;
    private:
        static void ReflectFunctions(AZ::ReflectContext* context);
    };
} // namespace ScriptCanvas
