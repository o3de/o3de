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
    QualityCVarGroup::QualityCVarGroup(AZ::SettingsRegistryInterface* registry, AZ::IConsole* console, AZStd::string_view name, AZStd::string_view path)
        : m_console(console)
        , m_numQualityLevels(0)
        , m_path(path)
        , m_qualityLevel(0)
        , m_settingsRegistry(registry)
    {
        AZ_Assert(registry, "QualityCVarGroup requireds a valid settings registry.");
        AZ_Assert(console, "QualityCVarGroup requireds a valid settings registry.");

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        // optional default quality level
        registry->Get(m_qualityLevel, FixedValueString::format("%.*s/Default", AZ_STRING_ARG(path)));

        // count the number of quality levels for error checking
        auto callback = [&]([[maybe_unused]] const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            m_numQualityLevels++;
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };
        auto levelsKey = FixedValueString::format("%.*s/Levels", AZ_STRING_ARG(path));
        AZ::SettingsRegistryVisitorUtils::VisitArray(*registry, callback, levelsKey);

        // optional description
        FixedValueString description;
        registry->Get(description, FixedValueString::format("%.*s/Description", AZ_STRING_ARG(path)));

        m_functor = AZStd::make_unique<QualityCVarGroupFunctor>(
            name.data(),
            description.data(),
            AZ::ConsoleFunctorFlags::DontReplicate,
            AZ::AzTypeInfo<ValueType>::Uuid(),
            *this,
            &QualityCVarGroup::CvarFunctor);
    }

    QualityCVarGroup::~QualityCVarGroup()
    {
    }

    inline void QualityCVarGroup::CvarFunctor(const AZ::ConsoleCommandContainer& arguments)
    {
        StringToValue(arguments);
    }

    QualityCVarGroup::ValueType QualityCVarGroup::GetQualityLevelFromName(AZStd::string_view levelName)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        ValueType levelIndex = -1;
        ValueType currentIndex = 0;
        FixedValueString level;

        // walk the quality group levels and find the one matching name 
        auto callback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_registry.Get(level, visitArgs.m_jsonKeyPath))
            {
                if (levelName.compare(level.c_str()) == 0)
                {
                    levelIndex = currentIndex;
                    return AZ::SettingsRegistryInterface::VisitResponse::Done;
                }
            }

            currentIndex++;
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        auto key = FixedValueString::format("%s/Levels", m_path.c_str());
        AZ::SettingsRegistryVisitorUtils::VisitArray(*m_settingsRegistry, callback, key);
        return levelIndex;
    }

    QualityCVarGroup::ValueType QualityCVarGroup::GetHighestQualityForSetting(AZStd::string_view path)
    {
        ValueType maxQualityLevel = -1;
        auto callback = [&]([[maybe_unused]] const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            maxQualityLevel++;
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };
        AZ::SettingsRegistryVisitorUtils::VisitArray(*m_settingsRegistry, callback, path);
        return maxQualityLevel;
    }

    void QualityCVarGroup::LoadQualityLevel(ValueType qualityLevel)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;
        using Type = AZ::SettingsRegistryInterface::Type;

        if (qualityLevel >= m_numQualityLevels || qualityLevel < 0)
        {
            AZ_Warning("QualityCVarGroup",
                       false,
                       "Invalid quality level %lld requested for %s which only supports quality levels 0..%lld",
                       qualityLevel,
                       m_functor->GetName(),
                       m_numQualityLevels - 1);
            return;
        }

        auto key = FixedValueString::format("%s/Settings", m_path.c_str());
        auto settingsVisitorCallback = [&](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            if (visitArgs.m_type == Type::Object ||
                visitArgs.m_type == Type::Null ||
                visitArgs.m_type == Type::NoType)
            {
                AZ_Warning("QualityCVarGroup", false, "Invalid setting value type for %.*s, objects and null are not supported, only arrays, bool, string, or numbers.", AZ_STRING_ARG(visitArgs.m_fieldName));
                return VisitResponse::Skip;
            }

            AZStd::string_view command = visitArgs.m_fieldName;
            AZStd::string_view valueKey = visitArgs.m_jsonKeyPath;
            if (visitArgs.m_type == Type::Array)
            {
                valueKey = FixedValueString::format("%.*s/%d", AZ_STRING_ARG(valueKey), qualityLevel);
                if (visitArgs.m_registry.GetType(valueKey) == Type::NoType)
                {
                    // if the array index doesn't exist, use the value for the highest index
                    ValueType maxQualityLevel = GetHighestQualityForSetting(visitArgs.m_jsonKeyPath);
                    if (maxQualityLevel < 0)
                    {
                        AZ_Warning("QualityCVarGroup", false, "Not applying settings for %.s because the array is empty.", AZ_STRING_ARG(command));
                        return VisitResponse::Continue;
                    }
                    valueKey = FixedValueString::format("%.*s/%d", AZ_STRING_ARG(visitArgs.m_jsonKeyPath), maxQualityLevel);
                }
            }

            PerformConsoleCommand(command, valueKey);

            return VisitResponse::Continue;
        };
        AZ::SettingsRegistryVisitorUtils::VisitObject(*m_settingsRegistry, settingsVisitorCallback, key);
    }

    void QualityCVarGroup::PerformConsoleCommand(AZStd::string_view command, AZStd::string_view key)
    {
        AZStd::string value;
        switch (m_settingsRegistry->GetType(key))
        {
        case AZ::SettingsRegistryInterface::Type::FloatingPoint:
            {
                double floatingPointValue;
                if (m_settingsRegistry->Get(floatingPointValue, key))
                {
                    value = AZStd::to_string(floatingPointValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::Boolean:
            {
                bool boolValue;
                if (m_settingsRegistry->Get(boolValue, key))
                {
                    value = boolValue ? "True" : "False";
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::Integer:
            {
                AZ::s64 intValue;
                if (m_settingsRegistry->Get(intValue, key))
                {
                    value = AZStd::to_string(intValue);
                }
            }
            break;
        case AZ::SettingsRegistryInterface::Type::String:
        default:
            {
                m_settingsRegistry->Get(value, key);
            }
            break;
        }

        if (!value.empty())
        {
            m_console->PerformCommand(command, { value }, AZ::ConsoleSilentMode::Silent);
        }
        else
        {
            AZ_Warning("QualityCVarGroup", false, "Failed to convert the value for %.*s to a string, or the value is empty.", AZ_STRING_ARG(command));
        }
    }

    bool QualityCVarGroup::StringToValue(const AZ::ConsoleCommandContainer& arguments)
    {
        if (!arguments.empty())
        {
            AZ::CVarFixedString convertCandidate{ arguments.front() };
            char* endPtr = nullptr;

            // TODO support loading quality levels using names

            ValueType qualityLevel = aznumeric_cast<ValueType>(strtoll(convertCandidate.c_str(), &endPtr, 0));
            if (qualityLevel == aznumeric_cast<ValueType>(QualitySystemEvents::LevelFromDeviceRules))
            {
                using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

                // TODO use device rules

                // fall back to default
                m_settingsRegistry->Get(qualityLevel, FixedValueString::format("%.*s/Default", AZ_STRING_ARG(m_path)));
            }
            m_qualityLevel = qualityLevel;

            LoadQualityLevel(m_qualityLevel);

            return true;
        }

        return false;
    }

    void QualityCVarGroup::ValueToString(AZ::CVarFixedString& outString) const
    {
        AZStd::to_string(outString, m_qualityLevel);
    }
} // namespace AzFramework

