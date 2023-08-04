/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Quality/QualitySystemComponent.h>
#include <AzFramework/Quality/QualityCVarGroup.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h> 
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/StringFunc/StringFunc.h>


namespace AzFramework
{
    inline constexpr const char* QualitySettingsGroupsRootKey = "/O3DE/Quality/Groups";
    inline constexpr const char* QualitySettingsDefaultGroupKey = "/O3DE/Quality/DefaultGroup";

    // constructor and destructor defined here to prevent compiler errors
    // if default constructor/destructor is defined in header because of
    // the member vector of unique_ptrs
    QualitySystemComponent::QualitySystemComponent() = default;
    QualitySystemComponent::~QualitySystemComponent() = default;

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(QualityLevels);

    void QualitySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            QualityLevelsReflect(*serializeContext);

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
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ_Assert(settingsRegistry, "QualitySystemComponent requires a SettingsRegistry but no instance has been created.");

        auto console = AZ::Interface<AZ::IConsole>::Get();
        AZ_Assert(console, "QualitySystemComponent requires an IConsole interface but no instance has been created.");

        if (settingsRegistry && console)
        {
            settingsRegistry->Get(m_defaultGroupName, QualitySettingsDefaultGroupKey);
            RegisterCvars();
            QualitySystemEvents::Bus::Handler::BusConnect();
        }
    }

    void QualitySystemComponent::Deactivate()
    {
        QualitySystemEvents::Bus::Handler::BusDisconnect();
        m_settingsGroupCVars.clear();
    }

    void QualitySystemComponent::LoadDefaultQualityGroup(int qualityLevel)
    {
        if (m_defaultGroupName.empty())
        {
            AZ_Warning("QualitySystemComponent", false, "No default quality group settings will be loaded because no default group name was defined at %s", QualitySettingsDefaultGroupKey);
        }
        else if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->PerformCommand(AZStd::string_view(m_defaultGroupName),
                { AZStd::to_string(qualityLevel).c_str() }, AZ::ConsoleSilentMode::Silent);
        }
    }

    void QualitySystemComponent::RegisterCvars()
    {
        // walk the quality groups in the settings registry and create cvars for every group
        auto groupVisitorCallback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
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

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::SettingsRegistryVisitorUtils::VisitObject(*settingsRegistry, groupVisitorCallback, QualitySettingsGroupsRootKey);
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

