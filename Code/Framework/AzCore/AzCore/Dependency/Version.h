/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>

#include <initializer_list>
#include <sstream>

namespace AZ
{
#define VERSION_SEPARATOR_CHAR '.'
#define VERSION_SEPARATOR_STR "."

    template <size_t N>
    struct Version
    {
        /// The type each component is stored as.
        using ComponentType = AZ::u64;

        /// Store size of Version object
        enum
        {
            parts_count = N
        };

        Version()
        {
            static_assert(N > 0, "Size for Version must be more than 0.");
            m_parts.fill(0);
        }

        Version(const Version& other)
            : m_parts(other.m_parts)
        {
        }

        Version(const AZStd::array<ComponentType, N>& parts)
            : m_parts(parts) { }

        Version(const std::initializer_list<ComponentType>& values)
        {
            AZ_Assert(values.size() == N,
                "Initializer size does not matches Version size. "
                "Expected: %u, got: %u",
                N, values.size());

            AZStd::transform(values.begin(), values.end(), m_parts.begin(), [](const ComponentType& c) { return c; });
        }

        /**
        * Parses a version from a string in the format "[part].[part].[part] ...".
        *
        * \param[in] versionStr     The string to parse for a version.
        * \returns                  On success, the parsed Version; on failure, a message describing the error.
        */
        static AZ::Outcome<Version, AZStd::string> ParseFromString(const AZStd::string& versionStr)
        {
            // there is 1 more part than there are dots in the string. (1.2.3 has 3 parts but 2 dots)
            size_t partCount = AZStd::accumulate(versionStr.begin(), versionStr.end(), size_t(1),
                [](size_t currentValue, const char elem) -> size_t
            {
                return currentValue + (elem == VERSION_SEPARATOR_CHAR ? 1 : 0);
            });
            if (N != partCount)
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to parse invalid version string \"%s\". "
                    "Number of parts in the string doesn't match the size. "
                    "Expected: %zu, got: %zu"
                    , versionStr.c_str(), N, partCount));
            }

            Version<N> result;
            std::istringstream iss(versionStr.c_str());
            for (int i = 0; i < partCount; ++i)
            {
                iss >> result.m_parts[i];

                // remove the dot before the next iteration
                char throwaway;
                iss >> throwaway;
                if (throwaway != VERSION_SEPARATOR_CHAR)
                {
                    return AZ::Failure(AZStd::string::format(
                        "Failed to parse invalid version string \"%s\". "
                        "Unexpected separator character encountered. "
                        "Expected: \"%d\", got: \"%d\""
                        , versionStr.c_str(), VERSION_SEPARATOR_CHAR, throwaway));
                }
            }

            if (!iss.eof())
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to parse invalid version string \"%s\". "
                    , versionStr.c_str()));
            }

            return AZ::Success(result);
        }

        /**
        * Returns the version in string form.
        *
        * \returns                  The version as a string formatted as "[major].[minor].[patch]".
        */
        AZStd::string ToString() const
        {
            std::stringstream ss;
            const char* separator = "";
            for (int i = 0; i < N; ++i)
            {
                ss << separator << m_parts[i];
                separator = VERSION_SEPARATOR_STR;
            }

            return ss.str().c_str();
        }

        /**
        * Compare two versions.
        *
        * Returns:                  0 if a == b, <0 if a < b, and >0 if a > b.
        */
        static int Compare(const Version& a, const Version& b)
        {
            for (int i = 0; i < N; ++i)
            {
                if (a.m_parts[i] != b.m_parts[i])
                {
                    return static_cast<int>(a.m_parts[i]) - static_cast<int>(b.m_parts[i]);
                }
            }

            return 0;
        }

        /**
        * Check if current version is all zero-ed out
        *
        * Returns:                  True if the current version is all zero-ed out, else false
        */
        bool IsZero() const
        {
            for (int i = 0; i < N; ++i)
            {
                if (m_parts[i] != 0)
                {
                    return false;
                }
            }

            return true;
        }

        AZStd::array<ComponentType, N> m_parts;
    };

     /**
      * Represents a version conforming to the Semantic Versioning standard (http://semver.org/)
      */
    struct SemanticVersion
        : public Version<3>
    {
        SemanticVersion()
            : Version<3>() { }
        SemanticVersion(const Version<3>& other)
            : Version<3>(other) { }
        SemanticVersion(ComponentType major, ComponentType minor, ComponentType patch)
        {
            m_parts[0] = major;
            m_parts[1] = minor;
            m_parts[2] = patch;
        }

        ComponentType GetMajor() const { return m_parts[0]; }
        ComponentType GetMinor() const { return m_parts[1]; }
        ComponentType GetPatch() const { return m_parts[2]; }
    };

    template <size_t N>
    inline bool operator< (const Version<N>& a, const Version<N>& b) { return Version<N>::Compare(a, b) < 0; }
    template <size_t N>
    inline bool operator> (const Version<N>& a, const Version<N>& b) { return Version<N>::Compare(a, b) > 0; }
    template <size_t N>
    inline bool operator<=(const Version<N>& a, const Version<N>& b) { return Version<N>::Compare(a, b) <= 0; }
    template <size_t N>
    inline bool operator>=(const Version<N>& a, const Version<N>& b) { return Version<N>::Compare(a, b) >= 0; }
    template <size_t N>
    inline bool operator==(const Version<N>& a, const Version<N>& b) { return Version<N>::Compare(a, b) == 0; }
    template <size_t N>
    inline bool operator!=(const Version<N>& a, const Version<N>& b) { return Version<N>::Compare(a, b) != 0; }
} // namespace AZ
