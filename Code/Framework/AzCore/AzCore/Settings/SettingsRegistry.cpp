/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/conversions.h>

namespace AZ
{
    SettingsRegistryInterface::Specializations::Specializations(AZStd::initializer_list<AZStd::string_view> specializations)
    {
        for (AZStd::string_view specialization : specializations)
        {
            Append(specialization);
        }
    }

    SettingsRegistryInterface::Specializations& SettingsRegistryInterface::Specializations::operator=(
        AZStd::initializer_list<AZStd::string_view> specializations)
    {
        for (AZStd::string_view specialization : specializations)
        {
            Append(specialization);
        }
        return *this;
    }

    bool SettingsRegistryInterface::Specializations::Append(AZStd::string_view specialization)
    {
        if (m_hashes.size() < m_hashes.max_size())
        {
            constexpr size_t MaxTagSize = TagName{}.max_size();
            if (specialization.size() > MaxTagSize)
            {
                AZ_Error("Settings Registry", false, R"(Cannot append Specialization tag of "%.*s.")"
                    " It is longer than the MaxTagNameSize of %zu", AZ_STRING_ARG(specialization),
                    MaxTagSize);
                return false;
            }

            m_names.push_back(TagName{ specialization });
            TagName& tag = m_names.back();
            AZStd::to_lower(tag.begin(), tag.end());
            m_hashes.push_back(Hash(tag));
            return true;
        }
        else
        {
            AZ_Error("Settings Registry", false,
                "Too many specialization for the Settings Registry. The maximum is %zu.", m_hashes.max_size());
            return false;
        }
    }

    bool SettingsRegistryInterface::Specializations::Contains(AZStd::string_view specialization) const
    {
        return Contains(Hash(specialization));
    }

    bool SettingsRegistryInterface::Specializations::Contains(size_t hash) const
    {
        return AZStd::find(m_hashes.begin(), m_hashes.end(), hash) != m_hashes.end();
    }

    size_t SettingsRegistryInterface::Specializations::GetPriority(AZStd::string_view specialization) const
    {
        return GetPriority(Hash(specialization));
    }

    size_t SettingsRegistryInterface::Specializations::GetPriority(size_t hash) const
    {
        auto it = AZStd::find(m_hashes.begin(), m_hashes.end(), hash);
        return it != m_hashes.end() ? AZStd::distance(m_hashes.begin(), it) : NotFound;
    }

    size_t SettingsRegistryInterface::Specializations::Hash(AZStd::string_view specialization)
    {
        FixedValueString lowercaseSpecialization{ specialization };
        AZStd::to_lower(lowercaseSpecialization.begin(), lowercaseSpecialization.end());
        return AZStd::hash<FixedValueString>{}(lowercaseSpecialization);
    }

    size_t SettingsRegistryInterface::Specializations::GetCount() const
    {
        return m_names.size();
    }

    AZStd::string_view SettingsRegistryInterface::Specializations::GetSpecialization(size_t index) const
    {
        return index < m_names.size() ? m_names[index] : AZStd::string_view();
    }

    SettingsRegistryInterface::CommandLineArgumentSettings::CommandLineArgumentSettings()
    {
        m_delimiterFunc = [](AZStd::string_view line) -> JsonPathValue
        {
            constexpr AZStd::string_view CommandLineArgumentDelimiters{ "=:" };
            JsonPathValue pathValue;
            pathValue.m_value = line;

            // Splits the line on the first delimiter and stores that in the pathValue.m_path variable
            // The StringFunc::TokenizeNext function updates the pathValue.m_value parameter in place
            // to contain all the text after the first delimiter
            // So if pathValue.m_value="foo = Hello Ice Cream=World:17", the call to TokenizeNext would
            // split the value as follows
            // pathValue.m_path = "foo"
            // pathValue.m_value = "Hello Ice Cream=World:17"
            if (auto path = AZ::StringFunc::TokenizeNext(pathValue.m_value, CommandLineArgumentDelimiters); path.has_value())
            {
                pathValue.m_path = AZ::StringFunc::StripEnds(*path);
            }
            pathValue.m_value = AZ::StringFunc::StripEnds(pathValue.m_value);
            return pathValue;
        };
    }

    // Settings Registry MergeSettings result implementtion
    AZ_DEFINE_ENUM_RELATIONAL_OPERATORS(SettingsRegistryInterface::MergeSettingsReturnCode);

    SettingsRegistryInterface::MergeSettingsResult::operator bool() const
    {
        return m_returnCode >= MergeSettingsReturnCode::Unset;
    }

    auto SettingsRegistryInterface::MergeSettingsResult::Combine(MergeSettingsResult other) &
        -> MergeSettingsResult&
    {
        // The other return code isn't set so combining is not needed
        if (other.m_returnCode == MergeSettingsReturnCode::Unset)
        {
            return *this;
        }

        // The merge settings return code is unset so take its return code
        if (m_returnCode == MergeSettingsReturnCode::Unset)
        {
            m_returnCode = other.m_returnCode;
        }
        else
        {
            // Combining a successful rc with a failure rc
            // results in partial success
            // The above if blocks validates that neither
            // this->m_returnCode nor other.m_returnCode is equal to Unset
            // so the checks below reduces to a two-state bool
            bool selfSuccess = (m_returnCode > MergeSettingsReturnCode::Unset);
            bool otherSuccess = (other.m_returnCode > MergeSettingsReturnCode::Unset);

            if ((selfSuccess && !otherSuccess)
                || (!selfSuccess && otherSuccess))
            {
                m_returnCode = MergeSettingsReturnCode::PartialSuccess;
            }
            else if (selfSuccess)
            {
                m_returnCode = MergeSettingsReturnCode::Success;
            }
            else
            {
                m_returnCode = MergeSettingsReturnCode::Failure;
            }
        }

        // add a newline between messages if the operation message is not empty
        if (!m_operationMessages.empty())
        {
            m_operationMessages += '\n';
        }

        m_operationMessages += AZStd::move(other.m_operationMessages);

        return *this;
    }

    auto SettingsRegistryInterface::MergeSettingsResult::Combine(MergeSettingsResult other) &&
        -> MergeSettingsResult
    {
        return static_cast<MergeSettingsResult&>(*this).Combine(AZStd::move(other));
    }

    auto SettingsRegistryInterface::MergeSettingsResult::Combine(MergeSettingsReturnCode otherReturnCode)
        -> MergeSettingsResult
    {
        return Combine(MergeSettingsResult{ otherReturnCode });
    }

    const AZStd::string& SettingsRegistryInterface::MergeSettingsResult::GetMessages() const
    {
        return m_operationMessages;
    }
} // namespace AZ
