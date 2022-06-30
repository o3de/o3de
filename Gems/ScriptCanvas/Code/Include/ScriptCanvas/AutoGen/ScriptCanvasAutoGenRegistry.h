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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
    class ComponentDescriptor;
}

//! Macros to self-register AutoGen node into ScriptCanvas
#define REGISTER_SCRIPTCANVAS_AUTOGEN_FUNCTION(LIBRARY)\
    static ScriptCanvas::LIBRARY##FunctionRegistry s_AutoGenFunctionRegistry;
#define REGISTER_SCRIPTCANVAS_AUTOGEN_NODEABLE(LIBRARY)\
    static ScriptCanvas::LIBRARY##NodeableRegistry s_AutoGenNodeableRegistry;
#define REGISTER_SCRIPTCANVAS_AUTOGEN_GRAMMAR(LIBRARY)\
    static ScriptCanvas::LIBRARY##GrammarRegistry s_AutoGenGrammarRegistry;

//! AutoGen registry util macros
#define INIT_SCRIPTCANVAS_AUTOGEN(LIBRARY)\
    ScriptCanvas::AutoGenRegistryManager::Init(#LIBRARY);
#define REFLECT_SCRIPTCANVAS_AUTOGEN(LIBRARY, CONTEXT)\
    ScriptCanvas::AutoGenRegistryManager::Reflect(CONTEXT, #LIBRARY);
#define GET_SCRIPTCANVAS_AUTOGEN_COMPONENT_DESCRIPTORS(LIBRARY)\
    ScriptCanvas::AutoGenRegistryManager::GetComponentDescriptors(#LIBRARY)

namespace ScriptCanvas
{
    struct NodeRegistry;

    class ScriptCanvasRegistry
    {
    public:
        virtual void Init(NodeRegistry* nodeRegistry) = 0;
        virtual void Reflect(AZ::ReflectContext* context) = 0;
        virtual AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors() = 0;
    };

    //! AutoGenRegistryManager
    //! The registry manager contains all autogen functions, nodeables and grammars metadata which will be registered
    //! for ScriptCanvas
    class AutoGenRegistryManager
    {
    public:
        AutoGenRegistryManager() = default;
        ~AutoGenRegistryManager();

        static AutoGenRegistryManager* GetInstance();

        //! Init all AutoGen registries
        static void Init();

        //! Init specified AutoGen registry by given name
        static void Init(const char* registryName);

        //! Get component descriptors from all AutoGen registries
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors();

        //! Get component descriptors from specified AutoGen registries
        static AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors(const char* registryName);

        //! Reflect all AutoGen registries
        static void Reflect(AZ::ReflectContext* context);

        //! Reflect specified AutoGen registry by given name
        static void Reflect(AZ::ReflectContext* context, const char* registryName);

        //! Get all expected autogen registry names
        AZStd::vector<AZStd::string> GetRegistryNames(const char* registryName);

        //! Register autogen registry with its name
        void RegisterRegistry(const char* registryName, ScriptCanvasRegistry* registry);

        //! Unregister autogen function registry by using its name
        void UnregisterRegistry(const char* registryName);

        std::unordered_map<std::string, ScriptCanvasRegistry*> m_registries;
    };
} // namespace ScriptCanvas
