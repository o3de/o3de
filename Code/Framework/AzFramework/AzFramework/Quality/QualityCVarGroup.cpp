/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Quality/QualityCVarGroup.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/utility/charconv.h>

namespace AzFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(QualityCVarGroup, AZ::SystemAllocator);

    QualityCVarGroup::QualityCVarGroup(AZStd::string_view name, AZStd::string_view path)
        : m_name(name)
        , m_path(path)
        , m_numQualityLevels(0)
        , m_qualityLevel(QualityLevel::LevelFromDeviceRules)
    {
        if (auto registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;

            // optional default quality level
            registry->GetObject(m_qualityLevel, FixedValueString::format("%s/Default",
                m_path.c_str()));

            // optional description
            registry->Get(m_description, FixedValueString::format("%s/Description",
                m_path.c_str()));

            // count the number of defined quality levels for error checking
            AZ::SettingsRegistryVisitorUtils::VisitArray(*registry,
                [&]([[maybe_unused]] const auto& visitArrayArg)
                {
                    m_numQualityLevels++;
                    return VisitResponse::Continue;
                },
                FixedValueString::format("%s/Levels", m_path.c_str()));
        }

        m_functor = AZStd::make_unique<QualityCVarGroupFunctor>(
            m_name.c_str(),
            m_description.c_str(),
            AZ::ConsoleFunctorFlags::DontReplicate,
            AZ::AzTypeInfo<QualityLevel>::Uuid(),
            *this,
            &QualityCVarGroup::CvarFunctor);
    }

    QualityCVarGroup::~QualityCVarGroup() = default;

    inline void QualityCVarGroup::CvarFunctor(const AZ::ConsoleCommandContainer& arguments)
    {
        StringToValue(arguments);
    }

    QualityLevel QualityCVarGroup::GetQualityLevel(const AZ::CVarFixedString& value) const
    {
        // We don't use AZ::ConsoleTypeHelpers::ToValue because it will print
        // a warning when it fails to convert a string to int32_t
        // The QualityLevel strings are user-defined and not known at compile
        // time so only the pre-defined values can be converted.

        // handle integer values (fast path)
        QualityLevel qualityLevel = FromNumber(value);

        if (qualityLevel == QualityLevel::Invalid)
        {
            // handle enum pre-defined values
            qualityLevel = FromEnum(value);
        }

        if (qualityLevel == QualityLevel::Invalid)
        {
            // handle user-defined level name values
            qualityLevel = FromName(value);
        }

        return qualityLevel;
    }

    QualityLevel QualityCVarGroup::FromNumber(const AZ::CVarFixedString& value) const
    {
        int32_t intValue{ 0 };
        auto result = AZStd::from_chars(value.begin(), value.end(), intValue);

        // don't allow values outside the known quality level ranges
        constexpr int32_t minValue = static_cast<int32_t>(QualityLevel::LevelFromDeviceRules);
        if (result.ec != AZStd::errc() || intValue >= m_numQualityLevels || intValue < minValue)
        {
            return QualityLevel::Invalid;
        }

        return QualityLevel(intValue);
    }

    QualityLevel QualityCVarGroup::FromEnum(const AZ::CVarFixedString& value) const
    {
        using EnumMemberPair = typename AZ::AzEnumTraits<QualityLevel>::MembersArrayType::value_type;
        auto findEnumOptionValue = [&value](EnumMemberPair enumPair)
        {
            // case-insensitive comparison
            constexpr bool caseSensitive{ false };
            return AZ::StringFunc::Equal(value, enumPair.m_string, caseSensitive);
        };
        if (auto foundIt = AZStd::ranges::find_if(AZ::AzEnumTraits<QualityLevel>::Members,
            findEnumOptionValue); foundIt != AZ::AzEnumTraits<QualityLevel>::Members.end())
        {
            return foundIt->m_value;
        }

        return QualityLevel::Invalid;
    }

    QualityLevel QualityCVarGroup::FromName(const AZ::CVarFixedString& value) const
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;

        auto registry = AZ::SettingsRegistry::Get();
        if (registry == nullptr)
        {
            return QualityLevel::Invalid;
        }

        QualityLevel qualityLevel{ QualityLevel::Invalid };

        // visit each user-defined level name and perform a case-insensitive comparison
        int32_t arrayIndex{ 0 };
        auto callback = [&](const auto& visitArrayArg)
        {
            FixedValueString qualityLevelName;
            if (registry->Get(qualityLevelName, visitArrayArg.m_jsonKeyPath))
            {
                // case-insensitive comparison
                constexpr bool caseSensitive{ false };
                if (AZ::StringFunc::Equal(qualityLevelName, value, caseSensitive))
                {
                    qualityLevel = QualityLevel(arrayIndex);
                    return VisitResponse::Done;
                }
            }
            arrayIndex++;
            return VisitResponse::Continue;
        };
        AZ::SettingsRegistryVisitorUtils::VisitArray(*registry, callback,
            FixedValueString::format("%s/Levels", m_path.c_str()));

        return qualityLevel;
    }

    void QualityCVarGroup::LoadQualityLevel(QualityLevel qualityLevel)
    {
        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;
        using Type = AZ::SettingsRegistryInterface::Type;

        auto registry = AZ::SettingsRegistry::Get();
        if (registry == nullptr)
        {
            return;
        }

        auto callback = [&](const auto& visitArgs)
        {
            AZStd::string_view command = visitArgs.m_fieldName;

            if (visitArgs.m_type == Type::Object ||
                visitArgs.m_type == Type::Null ||
                visitArgs.m_type == Type::NoType)
            {
                AZ_Warning("QualityCVarGroup", false,
                    "Invalid type for %.*s, valid types are array, bool, string, and number.",
                    AZ_STRING_ARG(command));
                return VisitResponse::Skip;
            }

            AZ::PerformCommandResult result{};
            if (visitArgs.m_type == Type::Array)
            {
                // the qualityLevel is the index containing the value to use for
                // the CVAR command, however, if the array does not contain the
                // qualityLevel index, use the highest available
                QualityLevel currentQualityLevel{ QualityLevel::LevelFromDeviceRules };
                auto applyQualityLevel = [&](const auto& visitArrayArg)
                {
                    currentQualityLevel++;
                    if (currentQualityLevel == qualityLevel)
                    {
                        result = PerformConsoleCommand(command, visitArrayArg.m_jsonKeyPath);
                        return VisitResponse::Done;
                    }
                    return VisitResponse::Continue;
                };

                AZ::SettingsRegistryVisitorUtils::VisitArray(*registry,
                    applyQualityLevel, visitArgs.m_jsonKeyPath);

                if (currentQualityLevel == QualityLevel::LevelFromDeviceRules)
                {
                    AZ_Error("QualityCVarGroup", false,
                        "No valid value found for %.*s, the array is empty.",
                        AZ_STRING_ARG(visitArgs.m_jsonKeyPath));
                }
                else if (!result)
                {
                    // the requested qualityLevel index wasn't found, use highest available
                    PerformConsoleCommand(command, FixedValueString::format("%.*s/%d",
                        AZ_STRING_ARG(visitArgs.m_jsonKeyPath), static_cast<int>(currentQualityLevel)));
                }
            }
            else
            {
                PerformConsoleCommand(command, visitArgs.m_jsonKeyPath);
            }

            return VisitResponse::Continue;
        };
        auto key = FixedValueString::format("%s/Settings", m_path.c_str());
        AZ::SettingsRegistryVisitorUtils::VisitObject(*registry, callback, key);
    }

    AZ::PerformCommandResult QualityCVarGroup::PerformConsoleCommand(AZStd::string_view command, AZStd::string_view key)
    {
        using Type = AZ::SettingsRegistryInterface::Type;

        auto registry = AZ::SettingsRegistry::Get();
        auto console = AZ::Interface<AZ::IConsole>::Get();
        if (!registry || !console)
        {
            return AZ::Failure("Missing required console or settings registry");
        }

        AZStd::string stringValue;
        switch (registry->GetType(key))
        {
        case Type::FloatingPoint:
            if (double value; registry->Get(value, key))
            {
                stringValue = AZ::ConsoleTypeHelpers::ToString(value);
            }
            break;
        case Type::Boolean:
            if (bool value; registry->Get(value, key))
            {
                stringValue = AZ::ConsoleTypeHelpers::ToString(value);
            }
            break;
        case Type::Integer:
            if (AZ::s64 value; registry->Get(value, key))
            {
                stringValue = AZ::ConsoleTypeHelpers::ToString(value);
            }
            break;
        case Type::String:
        default:
            {
                registry->Get(stringValue, key);
            }
            break;
        }

        if (!stringValue.empty())
        {
            return console->PerformCommand(command, { stringValue }, AZ::ConsoleSilentMode::Silent);
        }

        return AZ::Failure(AZStd::string::format("Failed to stringify %.*s value, or it's empty.",
            AZ_STRING_ARG(command)));
    }

    bool QualityCVarGroup::StringToValue(const AZ::ConsoleCommandContainer& arguments)
    {
        auto registry = AZ::SettingsRegistry::Get();
        if (registry == nullptr)
        {
            return false;
        }

        if (!arguments.empty())
        {
            AZ::CVarFixedString value{ arguments.front() };

            // GetQualityLevel will return QualityLevel::Invalid if unable
            // to convert the provided value to a valid QualityLevel
            QualityLevel qualityLevel = GetQualityLevel(value);

            if (qualityLevel == QualityLevel::LevelFromDeviceRules)
            {
                // TODO use device rules

                // fall back to default
                using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
                registry->GetObject(qualityLevel, FixedValueString::format("%s/Default", m_path.c_str()));
            }

            if (qualityLevel != QualityLevel::Invalid)
            {
                m_qualityLevel = qualityLevel;
                LoadQualityLevel(m_qualityLevel);
            }

            return true;
        }

        return false;
    }

    void QualityCVarGroup::ValueToString(AZ::CVarFixedString& outString) const
    {
        outString = AZ::ConsoleTypeHelpers::ToString(m_qualityLevel);
    }
} // namespace AzFramework

