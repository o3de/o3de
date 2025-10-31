/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::SettingsRegistryScriptUtils::Internal
{
    static void RegisterScriptProxyForNotify(SettingsRegistryInterface* settingsRegistry,
        SettingsRegistryScriptProxy::NotifyEventProxy* notifyEventProxy)
    {
        if (settingsRegistry != nullptr)
        {
            auto ForwardSettingsUpdateToProxyEvent = [notifyEventProxy](
                const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
            {
                if (notifyEventProxy)
                {
                    notifyEventProxy->m_scriptNotifyEvent.Signal(notifyEventArgs.m_jsonKeyPath);
                }
            };
            // Register the forwarding function with the BehaviorContext
            notifyEventProxy->m_settingsUpdatedHandler = settingsRegistry->RegisterNotifier(ForwardSettingsUpdateToProxyEvent);
        }
    }

    SettingsRegistryScriptProxy::SettingsRegistryScriptProxy() = default;
    SettingsRegistryScriptProxy::SettingsRegistryScriptProxy(AZStd::nullptr_t)
        : SettingsRegistryScriptProxy()
    {}

    SettingsRegistryScriptProxy::SettingsRegistryScriptProxy(AZStd::shared_ptr<AZ::SettingsRegistryInterface> settingsRegistry)
        : m_settingsRegistry(AZStd::move(settingsRegistry))
        , m_notifyEventProxy(AZStd::make_shared<NotifyEventProxy>())
    {
        RegisterScriptProxyForNotify(m_settingsRegistry.get(), m_notifyEventProxy.get());
    }

    // Raw AZ::SettingsRegistryInterface pointer is not owned by the proxy, so it's deleter is a no-op
    SettingsRegistryScriptProxy::SettingsRegistryScriptProxy(AZ::SettingsRegistryInterface* const settingsRegistry)
        : m_settingsRegistry(settingsRegistry, [](AZ::SettingsRegistryInterface*) {})
        , m_notifyEventProxy(AZStd::make_shared<NotifyEventProxy>())
    {
        RegisterScriptProxyForNotify(m_settingsRegistry.get(), m_notifyEventProxy.get());
    }

    // SettingsRegistryScriptProxy function that determines if the SettingsRegistry object is valid
    bool SettingsRegistryScriptProxy::IsValid() const
    {
        return m_settingsRegistry;
    }

    // Proxy class around the Settings Registry Specializations class
    // It needed to allow a default value for a Specialization to be bound to the Behavior Context
    // The BehaviorContext currently has a 32-byte limit for a default value
    struct SpecializationsProxy
    {
        AZ_TYPE_INFO(SpecializationsProxy, "{EB6B8ADF-ABAA-4D22-B596-127F9C611740}");

        SpecializationsProxy()
            : m_specializations{ AZStd::make_unique<AZ::SettingsRegistryInterface::Specializations>() }
        {}

        SpecializationsProxy(const SpecializationsProxy& other)
            : SpecializationsProxy()
        {
            *m_specializations = *other.m_specializations;
        }

        SpecializationsProxy& operator=(const SpecializationsProxy& other)
        {
            *m_specializations = *other.m_specializations;
            return *this;
        }

        AZStd::unique_ptr<AZ::SettingsRegistryInterface::Specializations> m_specializations;
    };

    static constexpr const char* SettingsRegistryModuleName = "settingsregistry";
    static constexpr const char* InterfaceClassName = "SettingsRegistryInterface";
    static constexpr const char* ImplClassName = "SettingsRegistry";

    void ReflectSpecializationsProxy(AZ::BehaviorContext& behaviorContext)
    {
        // Reflect Specializations structure
        auto SpecializationsAppend = [](SpecializationsProxy* tagInst, AZStd::string_view specialization)
        {
            return tagInst && tagInst->m_specializations->Append(specialization);
        };
        auto SpecializationsContains = [](SpecializationsProxy* tagInst, AZStd::string_view specialization)
        {
            return tagInst && tagInst->m_specializations->Contains(specialization);
        };
        auto SpecializationsGetPriority = [](SpecializationsProxy* tagInst, AZStd::string_view specialization)
        {
            return tagInst ? tagInst->m_specializations->GetPriority(specialization) : 0U;
        };
        auto SpecializationsGetCount = [](SpecializationsProxy* tagInst)
        {
            return tagInst ? tagInst->m_specializations->GetCount() : 0U;
        };
        auto SpecializationsGetSpecialization = [](SpecializationsProxy* tagInst, size_t index)
        {
            return tagInst ? tagInst->m_specializations->GetSpecialization(index) : AZStd::string_view{};
        };
        behaviorContext.Class<SpecializationsProxy>("Specializations")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, SettingsRegistryModuleName)
            ->Attribute(AZ::Script::Attributes::Category, ImplClassName)
            ->Method("Append", SpecializationsAppend)
            ->Method("Contains", SpecializationsContains)
            ->Method("GetPriority", SpecializationsGetPriority)
            ->Method("GetCount", SpecializationsGetCount)
            ->Method("GetSpecialization", SpecializationsGetSpecialization)
            ;
    }

    static constexpr const char* GlobalSettingsRegistryProperty = "g_SettingsRegistry";
    void ReflectSettingsRegistryProxy(AZ::BehaviorContext& behaviorContext)
    {
        auto GlobalSettingsRegistryGetter = []() -> Internal::SettingsRegistryScriptProxy
        {
            auto settingsRegistry = AZ::SettingsRegistry::Get();
            AZ_Warning("SettingsRegistryScriptUtils", settingsRegistry, "There is currently no Global Settings Registry registered"
                " with an AZ Interface<T>");
            // Don't delete the global Settings Registry instance, it is not owned by the behavior context
            return settingsRegistry;
        };
        behaviorContext.ConstantProperty(GlobalSettingsRegistryProperty, GlobalSettingsRegistryGetter)
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, SettingsRegistryModuleName)
            ->Attribute(AZ::Script::Attributes::Category, ImplClassName)
            ;

        // Settings Registry Merge function wrappers
        // Merge Settings from string
        auto MergeSettings = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonData,
            AZ::SettingsRegistryInterface::Format format) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->MergeSettings(jsonData, format);
        };
        // Set a default value for the Setting Registry Merge Format parameter
        // This allows the function to be called from the BehaviorContext using only the json data parameter
        AZStd::array<AZ::BehaviorParameterOverrides, AZStd::function_traits<decltype(MergeSettings)>::arity> mergeSettingsOverrides;
        mergeSettingsOverrides.back().m_defaultValue = behaviorContext.MakeDefaultValue(AZ::SettingsRegistryInterface::Format::JsonMergePatch);

        // Merge Settings from file
        auto MergeSettingsFile = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view filePath,
            AZStd::string_view jsonRootKey, AZ::SettingsRegistryInterface::Format format) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->MergeSettingsFile(filePath, format, jsonRootKey);
        };
        using MergeSettingsFileFunctionTraits = AZStd::function_traits<decltype(MergeSettingsFile)>;
        AZStd::array<AZ::BehaviorParameterOverrides, MergeSettingsFileFunctionTraits::arity> mergeSettingsFileOverrides;
        {
            constexpr size_t MergeSettingsFileRootKeyIndex = 2;
            constexpr size_t MergeSettingsFileFormatIndex = 3;
            static_assert(AZStd::is_same_v<MergeSettingsFileFunctionTraits::get_arg_t<MergeSettingsFileFormatIndex>, AZ::SettingsRegistryInterface::Format>,
                "MergeSettingsFile Format parameter is not at the correct index");
            static_assert(AZStd::is_same_v<MergeSettingsFileFunctionTraits::get_arg_t<MergeSettingsFileRootKeyIndex>, AZStd::string_view>,
                "MergeSettingsFile Root Key parameter is not at the correct index");

            mergeSettingsFileOverrides[MergeSettingsFileFormatIndex].m_defaultValue = behaviorContext.MakeDefaultValue(AZ::SettingsRegistryInterface::Format::JsonMergePatch);
            mergeSettingsFileOverrides[MergeSettingsFileRootKeyIndex].m_defaultValue = behaviorContext.MakeDefaultValue(AZStd::string_view{});
        }

        // Merge Settings from folder
        auto MergeSettingsFolder = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view folderPath,
            const Internal::SpecializationsProxy& specProxy, AZStd::string_view platform) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->MergeSettingsFolder(folderPath, *specProxy.m_specializations, platform);
        };
        using MergeSettingsFolderFunctionTraits = AZStd::function_traits<decltype(MergeSettingsFolder)>;
        AZStd::array<AZ::BehaviorParameterOverrides, MergeSettingsFolderFunctionTraits::arity> mergeSettingsFolderOverrides;
        {
            constexpr size_t MergeSettingsFolderSpecializationsIndex = 2;
            constexpr size_t MergeSettingsFolderPlatformIndex = 3;
            static_assert(AZStd::is_same_v<AZStd::remove_cvref_t<MergeSettingsFolderFunctionTraits::get_arg_t<MergeSettingsFolderSpecializationsIndex>>,
                Internal::SpecializationsProxy>,
                "MergeSettingsFolder Specializations parameter is not at the correct index");
            static_assert(AZStd::is_same_v<MergeSettingsFolderFunctionTraits::get_arg_t<MergeSettingsFolderPlatformIndex>, AZStd::string_view>,
                "MergeSettingsFolder Platform Key parameter is not at the correct index");

            mergeSettingsFolderOverrides[MergeSettingsFolderSpecializationsIndex].m_defaultValue = behaviorContext.MakeDefaultValue(Internal::SpecializationsProxy{});
            mergeSettingsFolderOverrides[MergeSettingsFolderPlatformIndex].m_defaultValue = behaviorContext.MakeDefaultValue(AZStd::string_view{});
        }

        auto SetBool = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath, bool boolValue) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Set(jsonPath, boolValue);
        };
        auto SetInt = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath, AZ::s64 intValue) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Set(jsonPath, intValue);
        };
        auto SetUint = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath, AZ::u64 uintValue) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Set(jsonPath, uintValue);
        };
        auto SetFloat = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath, double floatValue) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Set(jsonPath, floatValue);
        };
        auto SetString = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath, AZStd::string_view stringValue)
            -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Set(jsonPath, stringValue);
        };

        // Query functors
        auto DumpSettings = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath)
            -> AZStd::optional<AZStd::string>
        {
            AZStd::string outputString;
            AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings{ true };
            AZ::IO::ByteContainerStream outputStream(&outputString);
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid() &&
                AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*settingsRegistryProxy->m_settingsRegistry,
                    jsonPath, outputStream, dumperSettings))
            {
                return outputString;
            }

            return AZStd::nullopt;
        };

        auto GetBool = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath) -> AZStd::optional<bool>
        {
            bool boolValue{};
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Get(boolValue, jsonPath))
            {
                return boolValue;
            }

            return AZStd::nullopt;
        };
        auto GetInt = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath) -> AZStd::optional<s64>
        {
            AZ::s64 intValue{};
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Get(intValue, jsonPath))
            {
                return intValue;
            }

            return AZStd::nullopt;
        };
        auto GetUint = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath) -> AZStd::optional<u64>
        {
            AZ::u64 uintValue{};
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Get(uintValue, jsonPath))
            {
                return uintValue;
            }

            return AZStd::nullopt;
        };
        auto GetFloat = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath) -> AZStd::optional<double>
        {
            double floatValue{};
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Get(floatValue, jsonPath))
            {
                return floatValue;
            }

            return AZStd::nullopt;
        };
        auto GetString = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath) -> AZStd::optional<AZStd::string>
        {
            AZStd::string stringValue;
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid() && settingsRegistryProxy->m_settingsRegistry->Get(stringValue, jsonPath))
            {
                return stringValue;
            }

            return AZStd::nullopt;
        };

        // SettingsRegistry::Remove wrapper
        auto RemoveKey = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy, AZStd::string_view jsonPath) -> bool
        {
            return settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_settingsRegistry->Remove(jsonPath);
        };

        // Reflect function which returns the ScriptNotifyEvent from the SettingsRegistryScriptProxy
        // in order to trigger the ScriptCanvas feature of allowing creation of an AZ EventHandler
        // from a BehaviorMethod node which returns an AZ::Event pointer
        auto GetScriptNotifyEvent = [](Internal::SettingsRegistryScriptProxy* settingsRegistryProxy)
            -> Internal::SettingsRegistryScriptProxy::ScriptNotifyEvent*
        {
            if (settingsRegistryProxy && settingsRegistryProxy->IsValid()
                && settingsRegistryProxy->m_notifyEventProxy)
            {
                return &settingsRegistryProxy->m_notifyEventProxy->m_scriptNotifyEvent;
            }

            return nullptr;
        };

        auto settingsRegistryClassBuilder = behaviorContext.Class<Internal::SettingsRegistryScriptProxy>(InterfaceClassName);
        settingsRegistryClassBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, SettingsRegistryModuleName)
            ->Attribute(AZ::Script::Attributes::Category, ImplClassName)
            ->Method("IsValid", &Internal::SettingsRegistryScriptProxy::IsValid)
            ->Method("MergeSettings", MergeSettings, mergeSettingsOverrides)
            ->Method("MergeSettingsFile", MergeSettingsFile, mergeSettingsFileOverrides)
            ->Method("MergeSettingsFolder", MergeSettingsFolder, mergeSettingsFolderOverrides)
            ->Method("SetBool", SetBool)
            ->Method("SetInt", SetInt)
            ->Method("SetUInt", SetUint)
            ->Method("SetFloat", SetFloat)
            ->Method("SetString", SetString)
            ->Method("DumpSettings", DumpSettings)
            ->Method("GetBool", GetBool)
            ->Method("GetInt", GetInt)
            ->Method("GetUInt", GetUint)
            ->Method("GetFloat", GetFloat)
            ->Method("GetString", GetString)
            ->Method("RemoveKey", RemoveKey)
            ;

        // Create the BehaviorAzEventDescription needed to reflect the
        // GetNotifyEvent method to the BehaviorContext without errors
        AZ::BehaviorAzEventDescription scriptNotifyEventDesc;
        scriptNotifyEventDesc.m_eventName = "SettingsRegistry Notify Event";
        scriptNotifyEventDesc.m_parameterNames.push_back("Json Path");

        settingsRegistryClassBuilder->Method("GetNotifyEvent", GetScriptNotifyEvent)
            ->Attribute(AZ::Script::Attributes::AzEventDescription, AZStd::move(scriptNotifyEventDesc))
            ;
    }

    void ReflectSettingsRegistryCreateMethod(AZ::BehaviorContext& behaviorContext)
    {
        // This instance has a shared_ptr with a deleter that cleans up the memory
        behaviorContext.Method(ImplClassName, []() -> Internal::SettingsRegistryScriptProxy
        {
            return Internal::SettingsRegistryScriptProxy(AZStd::make_shared<AZ::SettingsRegistryImpl>());
        })
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, SettingsRegistryModuleName)
            ->Attribute(AZ::Script::Attributes::Category, ImplClassName)
            ;
    }

    void ReflectSettingsRegistryMergeFormatEnum(AZ::BehaviorContext& behaviorContext)
    {
        behaviorContext.EnumProperty<AZ::SettingsRegistryInterface::Format::JsonPatch>("JsonPatch")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, SettingsRegistryModuleName)
            ->Attribute(AZ::Script::Attributes::Category, ImplClassName)
            ;
        behaviorContext.EnumProperty<AZ::SettingsRegistryInterface::Format::JsonMergePatch>("JsonMergePatch")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, SettingsRegistryModuleName)
            ->Attribute(AZ::Script::Attributes::Category, ImplClassName)
            ;
    }
}

namespace AZ::SettingsRegistryScriptUtils
{
    void ReflectSettingsRegistryToBehaviorContext(AZ::BehaviorContext& behaviorContext)
    {
        // Reflect the Settings Registry Specializations proxy
        Internal::ReflectSpecializationsProxy(behaviorContext);
        // Reflect Setting Registry Proxy
        Internal::ReflectSettingsRegistryProxy(behaviorContext);
        // Reflect a method to create SettingsRegistryImpl instance and store it within a SettingsRegistryScriptProxy
        Internal::ReflectSettingsRegistryCreateMethod(behaviorContext);
        // Reflect SettingsRegistryInterface Format enum
        Internal::ReflectSettingsRegistryMergeFormatEnum(behaviorContext);
    }

    void ReflectSettingsRegistry(AZ::ReflectContext* context)
    {
        if (auto serializeContext{ azrtti_cast<AZ::SerializeContext*>(context) }; serializeContext != nullptr)
        {
            serializeContext->Class<Internal::SettingsRegistryScriptProxy>();
        }
        if (auto behaviorContext{ azrtti_cast<AZ::BehaviorContext*>(context) }; behaviorContext != nullptr)
        {
            AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistryToBehaviorContext(*behaviorContext);
        }
    }
}
