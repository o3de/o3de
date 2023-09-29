/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Quality/QualitySystemComponent.h>
#include <AzFramework/Quality/QualityCVarGroup.h>
#include <AzFramework/Device/DeviceAttributeInterface.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h> 
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/list.h>


namespace AzFramework
{
    inline constexpr const char* QualitySettingsGroupsRootKey = "/O3DE/Quality/Groups";
    inline constexpr const char* QualitySettingsDefaultGroupKey = "/O3DE/Quality/DefaultGroup";
    inline constexpr const char* QualitySettingsDevicesRootKey = "/O3DE/Quality/Devices";
    inline constexpr const char* QualitySettingsDevicesRulesResolutionKey = "/O3DE/Quality/DeviceRulesResolution";

    // constructor and destructor defined here to prevent compiler errors
    // if default constructor/destructor is defined in header because of
    // the member vector of unique_ptrs
    QualitySystemComponent::QualitySystemComponent() = default;
    QualitySystemComponent::~QualitySystemComponent() = default;

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(DeviceRuleResolution);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(QualityLevel);

    void QualitySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            DeviceRuleResolutionReflect(*serializeContext);
            QualityLevelReflect(*serializeContext);

            serializeContext->Class<QualitySystemComponent, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<QualitySystemComponent>(
                    "AzFramework Quality Component", "System component responsible for quality settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Editor")
                    ;
            }
        }
    }

    void QualitySystemComponent::Activate()
    {
        auto registry = AZ::SettingsRegistry::Get();
        AZ_Assert(registry, "QualitySystemComponent requires a SettingsRegistry but no instance has been created.");

        auto console = AZ::Interface<AZ::IConsole>::Get();
        AZ_Assert(console, "QualitySystemComponent requires an IConsole interface but no instance has been created.");

        if (registry && console)
        {
            registry->Get(m_defaultGroupName, QualitySettingsDefaultGroupKey);
            RegisterCvars();
            QualitySystemEvents::Bus::Handler::BusConnect();
        }
    }

    void QualitySystemComponent::Deactivate()
    {
        QualitySystemEvents::Bus::Handler::BusDisconnect();
        m_settingsGroupCVars.clear();
    }

    void QualitySystemComponent::LoadDefaultQualityGroup(QualityLevel qualityLevel)
    {
        if (m_defaultGroupName.empty())
        {
            AZ_Warning("QualitySystemComponent", false,
                "No default quality settings loaded because no default group name defined at %s",
                QualitySettingsDefaultGroupKey);
        }
        else if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->PerformCommand(AZStd::string_view(m_defaultGroupName),
                { AZ::ConsoleTypeHelpers::ToString(qualityLevel) }, AZ::ConsoleSilentMode::Silent);
        }

        // evaluate device rules on demand so device attributes and rules can be added
        // at runtime if needed
        EvaluateDeviceRules();
    }

    namespace Internal
    {
        bool HasMatchingDeviceAttributeRule(DeviceAttributeRegistrarInterface* deviceRegistrar, AZ::SettingsRegistryInterface* registry, AZStd::string_view jsonPath)
        {
            bool hasMatchingRule = false;

            auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                if (visitArgs.m_type != AZ::SettingsRegistryInterface::Type::String)
                {
                    // object entries (Lua) are not supported yet
                    AZ_Warning(
                        "QualitySystemComponent",
                        false,
                        "Skipping device attribute rule object entry '%.*s' that is not a string.",
                        AZ_STRING_ARG(visitArgs.m_fieldName));
                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                }

                // find the device attribute with this name and evaluate
                if (auto device = deviceRegistrar->FindDeviceAttribute(visitArgs.m_fieldName); device != nullptr)
                {
                    AZ::SettingsRegistryInterface::FixedValueString rule;
                    if (registry->Get(rule, visitArgs.m_jsonKeyPath))
                    {
                        hasMatchingRule = device->Evaluate(rule);

                        // in order for a rule to apply, all device attribute rules must evaluate true
                        if (!hasMatchingRule)
                        {
                            return AZ::SettingsRegistryInterface::VisitResponse::Done;
                        }
                    }
                }

                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            };

            AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, jsonPath);

            return hasMatchingRule;
        }

        bool HasMatchingRule(DeviceAttributeRegistrarInterface* deviceRegistrar, AZ::SettingsRegistryInterface* registry, AZStd::string_view jsonPath)
        {
            bool hasMatchingRule = false;

            auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                if (visitArgs.m_type != AZ::SettingsRegistryInterface::Type::Object)
                {
                    AZ_Warning(
                        "QualitySystemComponent",
                        false,
                        "Skipping device rules entry '%.*s' that is not an object type.",
                        AZ_STRING_ARG(visitArgs.m_fieldName));
                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                }

                if (HasMatchingDeviceAttributeRule(deviceRegistrar, registry, visitArgs.m_jsonKeyPath))
                {
                    // only one rule has to match
                    hasMatchingRule = true;
                    return AZ::SettingsRegistryInterface::VisitResponse::Done;
                }

                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            };

            AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, jsonPath);

            return hasMatchingRule;
        }

        struct RuleSetting
        {
            AZStd::string setting;
            AZStd::string value;
        };

        // order matters with rules, the user may choose to use the first matching rule or the last (default)
        // a list is used to maintain the order in which settings will be applied and a map is used to quickly
        // check if settings are in the list and retrieve an iterator to the setting within the list
        using RuleSettingList = AZStd::list<RuleSetting>;
        using RuleIteratorMap = AZStd::unordered_map<AZStd::string, RuleSettingList::iterator>;

        void UpdateRuleSettings(AZ::SettingsRegistryInterface* registry, RuleSettingList& settingsList, RuleIteratorMap& settingsMap, AZStd::string_view jsonPath, DeviceRuleResolution ruleResolution )
        {
            using Type = AZ::SettingsRegistryInterface::Type;
            auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                // has another rule provided a value for this setting already?
                auto iter = settingsMap.find(visitArgs.m_fieldName);
                if (iter != settingsMap.end() && ruleResolution == DeviceRuleResolution::First)
                {
                    // when using "First" we ignore duplicate setting entries 
                    return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                }

                // convert the value to a string to pass to console for execution
                AZStd::string stringValue;
                switch (visitArgs.m_type)
                {
                case Type::FloatingPoint:
                    if (double value; registry->Get(value, visitArgs.m_jsonKeyPath))
                    {
                        // handle Min/Max rule resolution for numbers
                        if (iter != settingsMap.end() && ruleResolution != DeviceRuleResolution::Last)
                        {
                            double currentValue = 0;
                            AZ::ConsoleTypeHelpers::StringToValue(currentValue, iter->second->value);
                            if ((ruleResolution == DeviceRuleResolution::Min && currentValue <= value) ||
                                (ruleResolution == DeviceRuleResolution::Max && currentValue >= value))
                            {
                                // disregard this entry because we already have a better value
                                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                            }
                        }
                        stringValue = AZ::ConsoleTypeHelpers::ToString(value);
                    }
                    break;
                case Type::Boolean:
                    if (bool value; registry->Get(value, visitArgs.m_jsonKeyPath))
                    {
                        stringValue = AZ::ConsoleTypeHelpers::ToString(value);
                    }
                    break;
                case Type::Integer:
                    if (AZ::s64 value; registry->Get(value, visitArgs.m_jsonKeyPath))
                    {
                        // handle Min/Max rule resolution for numbers
                        if (iter != settingsMap.end() && ruleResolution != DeviceRuleResolution::Last)
                        {
                            AZ::s64 currentValue = 0;
                            AZ::ConsoleTypeHelpers::StringToValue(currentValue, iter->second->value);
                            if ((ruleResolution == DeviceRuleResolution::Min && currentValue <= value) ||
                                (ruleResolution == DeviceRuleResolution::Max && currentValue >= value))
                            {
                                // disregard this entry because we already have a better value
                                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                            }
                        }
                        stringValue = AZ::ConsoleTypeHelpers::ToString(value);
                    }
                    break;
                case Type::String:
                    {
                        registry->Get(stringValue, visitArgs.m_jsonKeyPath);
                    }
                    break;
                default:
                    {
                        AZ_Warning(
                            "QualitySystemComponent",
                            false,
                            "Skipping settings entry '%.*s' that is not a string, number or bool type.",
                            AZ_STRING_ARG(visitArgs.m_fieldName));
                        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                    }
                }

                // if this setting already exists then we are replacing the existing setting value 
                // and should remove the old one from the ordered settings list
                if (iter != settingsMap.end())
                {
                    settingsList.erase(iter->second);
                }

                // add the setting to the back of the list
                settingsList.push_back({ AZStd::string(visitArgs.m_fieldName), stringValue.c_str() });

                // update/store a mapping of "setting" -> list iterator in the settings map
                settingsMap[visitArgs.m_fieldName] = --settingsList.end();

                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            };

            AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, jsonPath);
        }
    }

    void QualitySystemComponent::EvaluateDeviceRules()
    {
        using Type = AZ::SettingsRegistryInterface::Type;
        using VisitArgs = AZ::SettingsRegistryInterface::VisitArgs;
        using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        auto registry = AZ::SettingsRegistry::Get();
        auto deviceRegistrar = DeviceAttributeRegistrar::Get();
        auto console = AZ::Interface<AZ::IConsole>::Get();

        // query the settings registry for the rule resolution setting here
        // in case the value has changed since the last time device rules were evaluated
        DeviceRuleResolution ruleResolution = DeviceRuleResolution::Last;
        if (AZStd::string ruleValue; registry->Get(ruleValue, QualitySettingsDevicesRulesResolutionKey))
        {
            AZ::ConsoleTypeHelpers::StringToValue(ruleResolution, ruleValue);
        }

        Internal::RuleSettingList ruleSettings;
        Internal::RuleIteratorMap ruleIterators;

        // walk the devices in the settings registry and evaluate their rules 
        auto callback = [&](const VisitArgs& visitArgs)
        {
            if (visitArgs.m_type != Type::Object)
            {
                AZ_Warning(
                    "QualitySystemComponent",
                    false,
                    "Skipping device group entry '%.*s' that is not an object type.",
                    AZ_STRING_ARG(visitArgs.m_fieldName));
                return VisitResponse::Skip;
            }

            // iterate over all the device rules and see if any evaluate to true 
            const auto rulesPath = FixedValueString(visitArgs.m_jsonKeyPath) + "/Rules";
            if (Internal::HasMatchingRule(deviceRegistrar, registry, rulesPath))
            {
                // add all the valid settings found to the list of rule settings
                const auto settingsPath = FixedValueString(visitArgs.m_jsonKeyPath) + "/Settings";
                Internal::UpdateRuleSettings(registry, ruleSettings, ruleIterators, settingsPath, ruleResolution);
            }

            return VisitResponse::Continue;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, QualitySettingsDevicesRootKey);

        // go through rule settings list and apply them in order
        for (auto entry : ruleSettings)
        {
            console->PerformCommand(AZStd::string_view(entry.setting),
                { entry.value }, AZ::ConsoleSilentMode::Silent);
        }
    }

    void QualitySystemComponent::RegisterCvars()
    {
        // walk the quality groups in the settings registry and create cvars for every group
        auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_type != AZ::SettingsRegistryInterface::Type::Object)
            {
                AZ_Warning(
                    "QualitySystemComponent",
                    false,
                    "Skipping quality setting group entry '%.*s' that is not an object type.",
                    AZ_STRING_ARG(visitArgs.m_fieldName));
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }

            m_settingsGroupCVars.push_back(AZStd::make_unique<QualityCVarGroup>(visitArgs.m_fieldName, visitArgs.m_jsonKeyPath));
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        auto registry = AZ::SettingsRegistry::Get();
        AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, QualitySettingsGroupsRootKey);
    }

    void QualitySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("QualitySystemComponentService"));
    }

    void QualitySystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("QualitySystemComponentService"));
    }

    void QualitySystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

} // AzFramework

