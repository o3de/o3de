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


#pragma optimize("", off)

namespace AzFramework
{
    inline constexpr const char* QualitySettingsGroupsRootKey = "/O3DE/Quality/Groups";
    inline constexpr const char* QualitySettingsDefaultGroupKey = "/O3DE/Quality/DefaultGroup";
    inline constexpr const char* QualitySettingsDevicesRootKey = "/O3DE/Quality/Devices";

    // TODO CVAR for evaluating and printing device rule logic

    // constructor and destructor defined here to prevent compiler errors
    // if default constructor/destructor is defined in header because of
    // the member vector of unique_ptrs
    QualitySystemComponent::QualitySystemComponent() = default;
    QualitySystemComponent::~QualitySystemComponent() = default;

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(QualityLevel);

    void QualitySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
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
        // evaluate device rules on demand so device attributes and rules can be added
        // at runtime if needed
        EvaluateDeviceRules();

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
    }

    void EvaluateDeviceAttributeRules(DeviceAttributeRegistrarInterface* deviceRegistrar, AZ::SettingsRegistryInterface* registry, [[maybe_unused]] AZStd::string_view ruleName, AZStd::string_view jsonPath)
    {
        auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_type != AZ::SettingsRegistryInterface::Type::String)
            {
                // object entries (Lua) are not supported yet
                AZ_Warning("QualitySystemComponent", false, "Skipping device attribute rule object entry '%.*s' that is not a string.", AZ_STRING_ARG(visitArgs.m_fieldName));
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }

            // find the device attribute with this name and evaluate
            if (auto device = deviceRegistrar->FindDeviceAttribute(visitArgs.m_fieldName); device != nullptr)
            {
                AZ::SettingsRegistryInterface::FixedValueString rule;
                if (registry->Get(rule, visitArgs.m_jsonKeyPath))
                {
                    if (device->Evaluate(rule))
                    {
                        AZ_Info("QualitySystemComponent", "'%.*s' evaluates to true", AZ_STRING_ARG(rule));
                    }
                }
            }

            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, jsonPath);
    }

    void QualitySystemComponent::EvaluateDeviceRules()
    {
        auto registry = AZ::SettingsRegistry::Get();
        auto deviceRegistrar = DeviceAttributeRegistrar::Get();

        // walk the devices in the settings registry and evaluate their rules 
        auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_type != AZ::SettingsRegistryInterface::Type::Object)
            {
                AZ_Warning(
                    "QualitySystemComponent",
                    false,
                    "Skipping device group entry '%.*s' that is not an object type.",
                    AZ_STRING_ARG(visitArgs.m_fieldName));
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            }

            // iterate over all the device rules and evaluate them
            const auto rulesPath = AZ::SettingsRegistryInterface::FixedValueString(visitArgs.m_jsonKeyPath) + "/Rules";
            AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, [&](const auto& ruleVisitArgs)
                {
                    if (ruleVisitArgs.m_type != AZ::SettingsRegistryInterface::Type::Object)
                    {
                        AZ_Warning(
                            "QualitySystemComponent",
                            false,
                            "Skipping device rules entry '%.*s' that is not an object type.",
                            AZ_STRING_ARG(ruleVisitArgs.m_fieldName));
                        return AZ::SettingsRegistryInterface::VisitResponse::Skip;
                    }

                    // iterate over the device attribute rules and evaluate them
                    EvaluateDeviceAttributeRules(deviceRegistrar, registry, ruleVisitArgs.m_fieldName, ruleVisitArgs.m_jsonKeyPath);

                    return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                }, rulesPath);

            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, QualitySettingsDevicesRootKey);
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

#pragma optimize("", on)
