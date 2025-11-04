/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>
#include <PythonReflectionComponent.h>

#include <EditorPythonBindings/PythonUtility.h>
#include <EditorPythonBindings/PythonCommon.h>
#include <Source/PythonProxyBus.h>
#include <Source/PythonProxyObject.h>
#include <Source/PythonSymbolsBus.h>
#include <Source/PythonTypeCasters.h>
#include <pybind11/embed.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/IO/LocalFileIO.h>

namespace EditorPythonBindings
{
    namespace Internal
    {
        static constexpr const char* s_azlmbr = "azlmbr";
        static constexpr const char* s_default = "default";
        static constexpr const char* s_globals = "globals";

        // a structure for pybind11 to bind to hold constants, properties, and enums from the Behavior Context
        struct StaticPropertyHolder final
        {
            AZ_CLASS_ALLOCATOR(StaticPropertyHolder, AZ::SystemAllocator);

            StaticPropertyHolder() = default;
            ~StaticPropertyHolder() = default;

            bool AddToScope(pybind11::module scope)
            {
                m_behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(m_behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                AZ_Error("python", m_behaviorContext, "Behavior context not available");
                if (m_behaviorContext == nullptr)
                {
                    return false;
                }

                m_fullName = PyModule_GetName(scope.ptr());

                pybind11::setattr(scope, "__getattr__", pybind11::cpp_function([this](const char* attribute)
                {
                    return this->GetPropertyValue(attribute);
                }));
                pybind11::setattr(scope, "__setattr__", pybind11::cpp_function([this](const char* attribute, pybind11::object value)
                {
                    return this->SetPropertyValue(attribute, value);
                }));

                return true;
            }

            void AddProperty(AZStd::string_view name, AZ::BehaviorProperty* behaviorProperty)
            {
                AZStd::string baseName(name);
                Scope::FetchScriptName(behaviorProperty->m_attributes, baseName);
                AZ::Crc32 namedKey(baseName);
                if (m_properties.find(namedKey) == m_properties.end())
                {
                    m_properties[namedKey] = behaviorProperty;
                }
                else
                {
                    AZ_Warning("python", false, "Skipping duplicate property named %s\n", baseName.c_str());
                }
            }

        protected:

            void SetPropertyValue(const char* attributeName, pybind11::object value)
            {
                auto behaviorPropertyIter = m_properties.find(AZ::Crc32(attributeName));
                if (behaviorPropertyIter != m_properties.end())
                {
                    AZ::BehaviorProperty* property = behaviorPropertyIter->second;
                    AZ_Error("python", property->m_setter, "%s is not a writable property in %s.", attributeName, m_fullName.c_str());
                    if (property->m_setter)
                    {
                        EditorPythonBindings::Call::StaticMethod(property->m_setter, pybind11::args(pybind11::make_tuple(value)));
                    }
                }
            }

            pybind11::object GetPropertyValue(const char* attributeName)
            {
                AZ::Crc32 crcAttributeName(attributeName);
                auto behaviorPropertyIter = m_properties.find(crcAttributeName);
                if (behaviorPropertyIter != m_properties.end())
                {
                    AZ::BehaviorProperty* property = behaviorPropertyIter->second;
                    AZ_Error("python", property->m_getter, "%s is not a readable property in %s.", attributeName, m_fullName.c_str());
                    if (property->m_getter)
                    {
                        return EditorPythonBindings::Call::StaticMethod(property->m_getter, pybind11::args());
                    }
                }
                return pybind11::cast<pybind11::none>(Py_None);
            }

            AZ::BehaviorContext* m_behaviorContext = nullptr;
            AZStd::unordered_map<AZ::Crc32, AZ::BehaviorProperty*> m_properties;
            AZStd::string m_fullName;
        };

        using StaticPropertyHolderPointer = AZStd::unique_ptr<StaticPropertyHolder>;
        using StaticPropertyHolderMapEntry = AZStd::pair<pybind11::module, StaticPropertyHolderPointer>;

        struct StaticPropertyHolderMap final
            : public AZStd::unordered_map<AZStd::string, StaticPropertyHolderMapEntry>
        {
            Module::PackageMapType m_packageMap;

            void AddToScope()
            {
                for (auto&& element : *this)
                {
                    StaticPropertyHolderMapEntry& entry = element.second;
                    entry.second->AddToScope(entry.first);
                }
            }

            void AddProperty(pybind11::module scope, const AZStd::string& propertyName, AZ::BehaviorProperty* behaviorProperty)
            {
                AZStd::string scopeName = PyModule_GetName(scope.ptr());
                auto&& iter = find(scopeName);
                if (iter == end())
                {
                    StaticPropertyHolder* holder = aznew StaticPropertyHolder();
                    insert(AZStd::make_pair(scopeName, StaticPropertyHolderMapEntry{ scope, holder }));
                    holder->AddProperty(propertyName, behaviorProperty);
                }
                else
                {
                    StaticPropertyHolderMapEntry& entry = iter->second;
                    entry.second->AddProperty(propertyName, behaviorProperty);
                }
                PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogGlobalProperty, scopeName, propertyName, behaviorProperty);
            }

            pybind11::module DetermineScope(pybind11::module scope, const AZStd::string& fullName)
            {
                return Module::DeterminePackageModule(m_packageMap, fullName, scope, scope, false);
            }
        };

        void RegisterPaths(pybind11::module parentModule)
        {
            pybind11::module pathsModule = parentModule.def_submodule("paths");
            pathsModule.def("resolve_path", [](const char* path) -> AZStd::string
            {
                AZStd::optional<AZ::IO::FixedMaxPath> pyPath = AZ::IO::FileIOBase::GetInstance()->ResolvePath(path);
                return pyPath ? pyPath->String() : AZStd::string{};
            });
            pathsModule.def("ensure_alias", [](const char* alias, const char* path)
            {
                const char* aliasPath = AZ::IO::FileIOBase::GetInstance()->GetAlias(alias);
                if (aliasPath == nullptr)
                {
                    AZ::IO::FileIOBase::GetInstance()->SetAlias(alias, path);
                }
            });

            pathsModule.attr("engroot") = AZ::Utils::GetEnginePath().c_str();
            pathsModule.attr("products") = AZ::Utils::GetProjectProductPathForPlatform().c_str();
            pathsModule.attr("projectroot") = AZ::Utils::GetProjectPath().c_str();
            pathsModule.attr("log") = AZ::Utils::GetProjectLogPath().c_str();

            // Add a gemroot method for querying gem paths
            pathsModule.def("gemroot", [](const char* gemName) -> AZStd::string
            {
                return AZStd::string(AZ::Utils::GetGemPath(gemName));
            });

            pathsModule.attr("executableFolder") = AZ::Utils::GetExecutableDirectory().c_str();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // PythonReflectionComponent

    void PythonReflectionComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto&& serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PythonReflectionComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>{AZ_CRC_CE("AssetBuilder")})
                ;
        }
    }

    void PythonReflectionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(PythonReflectionService);
    }

    void PythonReflectionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(PythonReflectionService);
    }

    void PythonReflectionComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(PythonEmbeddedService);
    }

    void PythonReflectionComponent::Activate()
    {
        EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusConnect();
    }

    void PythonReflectionComponent::Deactivate()
    {
        OnPreFinalize();
    }

    void PythonReflectionComponent::ExportGlobalsFromBehaviorContext(pybind11::module parentModule)
    {
        AZ::BehaviorContext* behaviorContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Error("Editor", behaviorContext, "Behavior context not available");
        if (!behaviorContext)
        {
            return;
        }

        // when a global method does not have a Module attribute put into the 'azlmbr.globals' module
        auto globalsModule = parentModule.def_submodule(Internal::s_globals);
        Module::PackageMapType modulePackageMap;

        // add global methods flagged for Automation as Python global functions
        for (const auto& methodEntry : behaviorContext->m_methods)
        {
            const AZStd::string& methodName = methodEntry.first;
            AZ::BehaviorMethod* behaviorMethod = methodEntry.second;
            if (Scope::IsBehaviorFlaggedForEditor(behaviorMethod->m_attributes))
            {
                pybind11::module targetModule;
                auto moduleNameResult = Module::GetName(behaviorMethod->m_attributes);
                if(moduleNameResult)
                {
                    targetModule = Module::DeterminePackageModule(modulePackageMap, *moduleNameResult, parentModule, globalsModule, false);
                }
                else
                {
                    targetModule = globalsModule;
                }

                if (behaviorMethod->HasResult())
                {
                    targetModule.def(methodName.c_str(), [behaviorMethod](pybind11::args args)
                    {
                        return Call::StaticMethod(behaviorMethod, args);
                    });
                }
                else
                {
                    targetModule.def(methodName.c_str(), [behaviorMethod](pybind11::args args)
                    {
                        Call::StaticMethod(behaviorMethod, args);
                    });
                }

                // log global method symbol
                AZStd::string subModuleName = pybind11::cast<AZStd::string>(targetModule.attr("__name__"));
                PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogGlobalMethod, subModuleName, methodName, behaviorMethod);
            }
        }

        // add global properties flagged for Automation as Python static class properties
        m_staticPropertyHolderMap = AZStd::make_shared<Internal::StaticPropertyHolderMap>();
        struct GlobalPropertyHolder {};
        pybind11::class_<GlobalPropertyHolder> staticPropertyHolder(globalsModule, "property");
        for (const auto& propertyEntry : behaviorContext->m_properties)
        {
            const AZStd::string& propertyName = propertyEntry.first;
            AZ::BehaviorProperty* behaviorProperty = propertyEntry.second;
            if (Scope::IsBehaviorFlaggedForEditor(behaviorProperty->m_attributes))
            {
                auto propertyScopeName = Module::GetName(behaviorProperty->m_attributes);
                if (propertyScopeName)
                {
                    pybind11::module scope = m_staticPropertyHolderMap->DetermineScope(parentModule, *propertyScopeName);
                    m_staticPropertyHolderMap->AddProperty(scope, propertyName, behaviorProperty);
                }

                //  log global property symbol
                AZStd::string subModuleName = pybind11::cast<AZStd::string>(globalsModule.attr("__name__"));
                PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::LogGlobalProperty, subModuleName, propertyName, behaviorProperty);

                if (behaviorProperty->m_getter && behaviorProperty->m_setter)
                {
                    staticPropertyHolder.def_property_static(
                        propertyName.c_str(),
                        [behaviorProperty](pybind11::object) { return Call::StaticMethod(behaviorProperty->m_getter, {}); },
                        [behaviorProperty](pybind11::object, pybind11::args args) { return Call::StaticMethod(behaviorProperty->m_setter, args); }
                    );
                }
                else if (behaviorProperty->m_getter)
                {
                    staticPropertyHolder.def_property_static(
                        propertyName.c_str(),
                        [behaviorProperty](pybind11::object) { return Call::StaticMethod(behaviorProperty->m_getter, {}); },
                        pybind11::cpp_function()
                    );
                }
                else if (behaviorProperty->m_setter)
                {
                    AZ_Warning("python", false, "Global property %s only has a m_setter; write only properties not supported", propertyName.c_str());
                }
                else
                {
                    AZ_Error("python", false, "Global property %s has neither a m_getter or m_setter", propertyName.c_str());
                }
            }
        }

        m_staticPropertyHolderMap->AddToScope();
    }

    void PythonReflectionComponent::OnPreFinalize()
    {
        m_staticPropertyHolderMap.reset();
        EditorPythonBindings::EditorPythonBindingsNotificationBus::Handler::BusDisconnect();
    }

    void PythonReflectionComponent::OnImportModule(PyObject* module)
    {
        pybind11::module parentModule = pybind11::cast<pybind11::module>(module);
        std::string pythonModuleName = pybind11::cast<std::string>(parentModule.attr("__name__"));
        if (AZ::StringFunc::Equal(pythonModuleName.c_str(), Internal::s_azlmbr))
        {
            // declare the default module to capture behavior that did not define a "Module" attribute
            pybind11::module defaultModule = parentModule.def_submodule(Internal::s_default);

            ExportGlobalsFromBehaviorContext(parentModule);
            PythonProxyObjectManagement::CreateSubmodule(parentModule, defaultModule);
            PythonProxyBusManagement::CreateSubmodule(parentModule);
            Internal::RegisterPaths(parentModule);

            PythonSymbolEventBus::QueueBroadcast(&PythonSymbolEventBus::Events::Finalize);
        }
    }
}
