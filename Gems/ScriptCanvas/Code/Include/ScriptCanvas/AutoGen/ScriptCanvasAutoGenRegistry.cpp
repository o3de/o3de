/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/string/fixed_string.h>
#include <ScriptCanvas/Libraries/ScriptCanvasNodeRegistry.h>

#include "ScriptCanvasAutoGenRegistry.h"

namespace ScriptCanvas
{
    static constexpr const char ScriptCanvasAutoGenDataRegistrySuffix[] = "DataRegistry";
    static constexpr const char ScriptCanvasAutoGenFunctionRegistrySuffix[] = "FunctionRegistry";
    static constexpr const char ScriptCanvasAutoGenNodeableRegistrySuffix[] = "NodeableRegistry";
    static constexpr const char ScriptCanvasAutoGenGrammarRegistrySuffix[] = "GrammarRegistry";
    static constexpr const char ScriptCanvasAutoGenRegistryName[] = "AutoGenRegistryManager";
    static constexpr int MaxMessageLength = 4096;
    static constexpr const char ScriptCanvasAutoGenRegistrationWarningMessage[] = "[Warning] Registry name %s is occupied already, ignore AutoGen registry registration.\n";

    void ScriptCanvasRegistry::ReleaseDescriptors()
    {
        for (AZ::ComponentDescriptor* descriptor : m_cachedDescriptors)
        {
            descriptor->ReleaseDescriptor();
        }
        m_cachedDescriptors = {};
    }

    AutoGenRegistryManager::~AutoGenRegistryManager()
    {
        // If any registries still exist at this point, make sure they get released. Otherwise, they can cause a crash on shutdown.

        AZ_Error("AutoGen", m_registries.empty(),
            "Auto-registered registries still exist on shutdown. This isn't harmful, but there is a programming or linking error causing "
            "destruction of the registries to happen in the wrong order relative to the AutoGenRegistryManager.");

        for (auto& registry : m_registries)
        {
            registry.second->ReleaseDescriptors();
        }
        
        m_registries.clear();
    }

    AutoGenRegistryManager* AutoGenRegistryManager::GetInstance()
    {
        // This needs to be declared inside of GetInstance() to ensure proper construction / destruction order relative to any
        // static registries. What happens is that inside the constructor of the first static registry loaded and processed, it
        // will call GetInstance(), which will construct this static variable. Since this variable finishes constructing before
        // the first static registry finishes, it won't be destroyed until *after* that registry is destroyed on shutdown.
        // If this were declared outside of the GetInstance() call, its construction order would be non-deterministic relative
        // to the static registries in this DLL and so it could potentially get destroyed too soon.
        static AZ::EnvironmentVariable<AutoGenRegistryManager> g_autogenRegistry;

        // Look up variable in AZ::Environment first
        // This is need if the Environment variable was already created
        if (!g_autogenRegistry)
        {
            g_autogenRegistry = AZ::Environment::FindVariable<AutoGenRegistryManager>(ScriptCanvasAutoGenRegistryName);
        }

        // Create the environment variable in O3DEKernel memory space if it has not been found
        if (!g_autogenRegistry)
        {
            g_autogenRegistry = AZ::Environment::CreateVariable<AutoGenRegistryManager>(ScriptCanvasAutoGenRegistryName);
        }

        return &(g_autogenRegistry.Get());
    }

    AZStd::vector<AZStd::string> AutoGenRegistryManager::GetRegistryNames(const char* registryName)
    {
        AZStd::vector<AZStd::string> result;
        result.push_back(AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenDataRegistrySuffix).c_str());
        result.push_back(AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenFunctionRegistrySuffix).c_str());
        result.push_back(AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenNodeableRegistrySuffix).c_str());
        result.push_back(AZStd::string::format("%s%s", registryName, ScriptCanvasAutoGenGrammarRegistrySuffix).c_str());
        return result;
    }

    void AutoGenRegistryManager::Init()
    {
        auto registry = GetInstance();
        auto nodeRegistry = NodeRegistry::GetInstance();
        if (registry && nodeRegistry)
        {
            for (auto& iter : registry->m_registries)
            {
                if (iter.second)
                {
                    iter.second->Init(nodeRegistry);
                }
            }
        }
    }

    void AutoGenRegistryManager::Init(const char* registryName)
    {
        auto registry = GetInstance();
        auto nodeRegistry = NodeRegistry::GetInstance();
        if (registry && nodeRegistry)
        {
            auto registryNames = registry->GetRegistryNames(registryName);
            for (auto name : registryNames)
            {
                auto registryIter = registry->m_registries.find(name.c_str());
                if (registryIter != registry->m_registries.end())
                {
                    registryIter->second->Init(nodeRegistry);
                }
            }
        }
    }

    AZStd::vector<AZ::ComponentDescriptor*> AutoGenRegistryManager::GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors;
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_registries)
            {
                if (iter.second)
                {
                    auto nodeableDescriptors = iter.second->GetComponentDescriptors();
                    descriptors.insert(descriptors.end(), nodeableDescriptors.begin(), nodeableDescriptors.end());
                }
            }
        }
        return descriptors;
    }

    AZStd::vector<AZ::ComponentDescriptor*> AutoGenRegistryManager::GetComponentDescriptors(const char* registryName)
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors;
        if (auto registry = GetInstance())
        {
            auto registryNames = registry->GetRegistryNames(registryName);
            for (auto name : registryNames)
            {
                auto registryIter = registry->m_registries.find(name.c_str());
                if (registryIter != registry->m_registries.end())
                {
                    auto registryDescriptors = registryIter->second->GetComponentDescriptors();
                    descriptors.insert(descriptors.end(), registryDescriptors.begin(), registryDescriptors.end());
                }
            }
        }
        return descriptors;
    }

    void AutoGenRegistryManager::Reflect(AZ::ReflectContext* context)
    {
        if (auto registry = GetInstance())
        {
            for (auto& iter : registry->m_registries)
            {
                if (iter.second)
                {
                    iter.second->Reflect(context);
                }
            }
        }
    }

    void AutoGenRegistryManager::Reflect(AZ::ReflectContext* context, const char* registryName)
    {
        if (auto registry = GetInstance())
        {
            auto registryNames = registry->GetRegistryNames(registryName);
            for (auto name : registryNames)
            {
                auto registryIter = registry->m_registries.find(name.c_str());
                if (registryIter != registry->m_registries.end())
                {
                    registryIter->second->Reflect(context);
                }
            }
        }
    }

    void AutoGenRegistryManager::RegisterRegistry(const char* registryName, ScriptCanvasRegistry* registry)
    {
        if (m_registries.find(registryName) != m_registries.end())
        {
            // This can happen if multiple Gems link to ScriptCanvas, since the ScriptCanvas registries will try to
            // register themselves with each Gem that loads, causing redundant registration requests.
            AZ::Debug::Platform::OutputToDebugger(
                ScriptCanvasAutoGenRegistryName,
                AZStd::fixed_string<MaxMessageLength>::format(ScriptCanvasAutoGenRegistrationWarningMessage, registryName).c_str());
        }
        else if (registry != nullptr)
        {
            m_registries.emplace(registryName, registry);
        }
    }

    void AutoGenRegistryManager::UnregisterRegistry(const char* registryName)
    {
        if (auto it = m_registries.find(registryName);
            it != m_registries.end())
        {
            it->second->ReleaseDescriptors();
            m_registries.erase(it);
        }
    }

} // namespace ScriptCanvas
