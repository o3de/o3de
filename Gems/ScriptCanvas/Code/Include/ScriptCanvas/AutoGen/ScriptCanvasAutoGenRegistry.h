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

//! Macros to self-register AutoGen functions into ScriptCanvas
//! Which takes the same library name as provided in .ScriptCanvasFunction.xml
#define REGISTER_SCRIPTCANVAS_FUNCTION(LIBRARY)\
    static ScriptCanvas##LIBRARY s_ScriptCanvas##LIBRARY;

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

        // Reflect all autogen functions
        static void Reflect(AZ::ReflectContext* context);

        //! Reflect specified autogen function by given name
        static void ReflectFunction(AZ::ReflectContext* context, const char* functionName);

        void RegisterFunction(const char* functionName, IScriptCanvasFunctionRegistry* registry);
        void UnregisterFunction(const char* functionName);

        std::unordered_map<std::string, IScriptCanvasFunctionRegistry*> m_functions;
    private:
        static void ReflectFunctions(AZ::ReflectContext* context);
    };
} // namespace ScriptCanvas
