/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Quality/QualityCVarGroup.h>
#include <AzFramework/Quality/QualitySystemBus.h>

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace AzFramework
{
    QualityCVarGroup::QualityCVarGroup(AZStd::string_view name, AZStd::string_view path)
        : m_name(name)
        , m_path(path)
        , m_qualityLevel(QualityLevels::LevelFromDeviceRules)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        FixedValueString description;

        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            // optional default quality level
            settingsRegistry->GetObject(m_qualityLevel, FixedValueString::format("%.*s/Default", AZ_STRING_ARG(path)));

            // optional description
            settingsRegistry->Get(description, FixedValueString::format("%.*s/Description", AZ_STRING_ARG(path)));
        }

        m_functor = AZStd::make_unique<QualityCVarGroupFunctor>(
            m_name.c_str(),
            description.c_str(),
            AZ::ConsoleFunctorFlags::DontReplicate,
            AZ::AzTypeInfo<ValueType>::Uuid(),
            *this,
            &QualityCVarGroup::CvarFunctor);
    }

    QualityCVarGroup::~QualityCVarGroup() = default;

    inline void QualityCVarGroup::CvarFunctor(const AZ::ConsoleCommandContainer& arguments)
    {
        StringToValue(arguments);
    }

    void QualityCVarGroup::LoadQualityLevel(ValueType qualityLevel)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;
        using Type = AZ::SettingsRegistryInterface::Type;

        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            return;
        }

        auto key = FixedValueString::format("%s/Settings", m_path.c_str());
        auto settingsVisitorCallback = [&, settingsRegistry](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string_view command = visitArgs.m_fieldName;

            if (visitArgs.m_type == Type::Object ||
                visitArgs.m_type == Type::Null ||
                visitArgs.m_type == Type::NoType)
            {
                AZ_Warning("QualityCVarGroup", false, "Invalid setting value type for %.*s, objects and null are not supported, only arrays, bool, string, or numbers.", AZ_STRING_ARG(command));
                return VisitResponse::Skip;
            }

            AZ::PerformCommandResult result{};
            if (visitArgs.m_type == Type::Array)
            {
                AzFramework::QualityLevels currentQualityLevel{ 0 };
                AZStd::string_view arrayValueKey = visitArgs.m_jsonKeyPath;
                auto getHighestQualityLevel = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArrayArg)
                {
                    if (currentQualityLevel == qualityLevel)
                    {
                        result = PerformConsoleCommand(command, visitArrayArg.m_jsonKeyPath);
                        return VisitResponse::Done;
                    }
                    currentQualityLevel++;
                    return VisitResponse::Continue;
                };
                AZ::SettingsRegistryVisitorUtils::VisitArray(*settingsRegistry, getHighestQualityLevel, visitArgs.m_jsonKeyPath);
                if (currentQualityLevel == AzFramework::QualityLevels(0))
                {
                    AZ_Error("QualityCVarGroup", false, "No valid value found for %.*s, the array is empty.", AZ_STRING_ARG(visitArgs.m_jsonKeyPath));
                }
                else if (!result)
                {
                    // the key was not found, use the highest quality level
                    PerformConsoleCommand(command, FixedValueString::format("%.*s/%d", AZ_STRING_ARG(visitArgs.m_jsonKeyPath), --currentQualityLevel));
                }
            }
            else
            {
                PerformConsoleCommand(command, visitArgs.m_jsonKeyPath);
            }

            return VisitResponse::Continue;
        };
        AZ::SettingsRegistryVisitorUtils::VisitObject(*settingsRegistry, settingsVisitorCallback, key);
    }

    AZ::PerformCommandResult QualityCVarGroup::PerformConsoleCommand(AZStd::string_view command, AZStd::string_view key)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        auto console = AZ::Interface<AZ::IConsole>::Get();
        if (!settingsRegistry || !console)
        {
            return AZ::Failure("Unable to perform console command without a valid Console and Settings Registry");
        }

        AZStd::string value;
        switch (settingsRegistry->GetType(key))
        {
        case AZ::SettingsRegistryInterface::Type::FloatingPoint:
            {
                double floatingPointValue;
                if (settingsRegistry->Get(floatingPointValue, key))
                {
                    value = AZ::ConsoleTypeHelpers::ToString(floatingPointValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::Boolean:
            {
                bool boolValue;
                if (settingsRegistry->Get(boolValue, key))
                {
                    value = AZ::ConsoleTypeHelpers::ToString(boolValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::Integer:
            {
                AZ::s64 intValue;
                if (settingsRegistry->Get(intValue, key))
                {
                    value = AZ::ConsoleTypeHelpers::ToString(intValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::String:
        default:
            {
                settingsRegistry->Get(value, key);
            }
            break;
        }

        if (!value.empty())
        {
            return console->PerformCommand(command, { value }, AZ::ConsoleSilentMode::Silent);
        }

        return AZ::Failure(AZStd::string::format("Failed to convert the value for %.*s to a string, or the value is empty.", AZ_STRING_ARG(command)));
    }

    bool QualityCVarGroup::StringToValue(const AZ::ConsoleCommandContainer& arguments)
    {
        if (!arguments.empty())
        {
            AZ::CVarFixedString convertCandidate{ arguments.front() };

            ValueType qualityLevel = QualityLevels::LevelFromDeviceRules;
            AZ::ConsoleTypeHelpers::ToValue(qualityLevel, convertCandidate);

            if (qualityLevel == QualityLevels::LevelFromDeviceRules)
            {
                // TODO use device rules

                // fall back to default
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
                    settingsRegistry->GetObject(qualityLevel, FixedValueString::format("%.*s/Default", AZ_STRING_ARG(m_path)));
                }
            }

            m_qualityLevel = qualityLevel;

            LoadQualityLevel(m_qualityLevel);

            return true;
        }

        return false;
    }

    void QualityCVarGroup::ValueToString(AZ::CVarFixedString& outString) const
    {
        outString = AZ::ConsoleTypeHelpers::ToString(m_qualityLevel);
    }
} // namespace AzFramework

