/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/StringFunc/StringFunc.h>

namespace AZ
{
    //////////////////////////////////////////////////////////////////////////
    // Specifier
    //////////////////////////////////////////////////////////////////////////
    template <size_t N>
    Specifier<N>::Specifier(const AZ::Uuid& id, const Version<N>& version)
        : m_id(id)
        , m_version(version)
    {}

    //////////////////////////////////////////////////////////////////////////
    // Dependency::Bound
    //////////////////////////////////////////////////////////////////////////
    template <size_t N>
    AZStd::string Dependency<N>::Bound::ToString() const
    {
        // use version string if it's set
        // Dependency::ParseVersions set this value
        // this is used to save user's version string configuration (eg ==1.8.01.001)
        if (m_parsedString.empty() == false)
        {
            return m_parsedString;
        }

        // if there is no version string set, we generate it based on the m_version and m_comparison
        if ((m_comparison& Comparison::TwiddleWakka) != Comparison::None)
        {
            AZ_Assert(m_parseDepth >= 2, "Internal Error: There should be "
                "or more than 2 parts to a TwiddleWakka dependency.");

            AZStd::string version = AZStd::string::format("~=%llu", m_version.m_parts[0]);
            for (AZ::u8 i = 1; i < m_parseDepth; ++i)
            {
                version += AZStd::string::format(".%llu", m_version.m_parts[i]);
            }
            return version;
        }

        AZStd::string op = "";

        if ((m_comparison& Comparison::GreaterThan) != Comparison::None)
        {
            op = ">";
        }
        else if ((m_comparison& Comparison::LessThan) != Comparison::None)
        {
            op = "<";
        }

        if ((m_comparison& Comparison::EqualTo) != Comparison::None)
        {
            op += op.length() == 0 ? "==" : "=";
        }

        return op + m_version.ToString();
    }

    template <size_t N>
    bool Dependency<N>::Bound::MatchesVersion(const Version<N>& version) const
    {
        bool satisfies = false;

#define CHECK_OPERATOR(comp, OP)                        \
            if (!satisfies && (m_comparison& Comparison::comp) != Comparison::None) \
            {                                                   \
                satisfies = version OP m_version;               \
            }

        CHECK_OPERATOR(EqualTo, == );
        CHECK_OPERATOR(GreaterThan, > );
        CHECK_OPERATOR(LessThan, < );
#undef CHECK_OPERATOR

        return satisfies;
    }

    template <size_t N>
    void Dependency<N>::Bound::SetVersion(const Version<N>& version)
    {
        m_parsedString = "";
        m_version = version;
    }

    template <size_t N>
    const Version<N>& Dependency<N>::Bound::GetVersion() const
    {
        return m_version;
    }

    template <size_t N>
    void Dependency<N>::Bound::SetComparison(Comparison comp)
    {
        m_parsedString = "";
        m_comparison = comp;
    }

    template <size_t N>
    typename Dependency<N>::Bound::Comparison Dependency<N>::Bound::GetComparison() const
    {
        return m_comparison;
    }

    //////////////////////////////////////////////////////////////////////////
    // Dependency
    //////////////////////////////////////////////////////////////////////////
    template <size_t N>
    Dependency<N>::Dependency()
        : m_dependencyRegex("(?:(~>|~=|==|===|[>=<]{1,2}) *([0-9]+(?:\\.[0-9]+)*))") // ?: denotes a non-capture group
        , m_namedDependencyRegex("(?:([^~>=<]*)(~>|~=|==|===|[>=<]{1,2}) *([0-9]+(?:\\.[0-9]+)*))")
        , m_versionRegex("([0-9]+)(?:\\.(.*)){0,1}")
    {
    }

    template <size_t N>
    Dependency<N>::Dependency(const Dependency& dep)
        : m_id(dep.m_id)
        , m_bounds(dep.m_bounds)
        , m_dependencyRegex(dep.m_dependencyRegex)
        , m_versionRegex(dep.m_versionRegex)
    {
    }

    template <size_t N>
    const AZ::Uuid& Dependency<N>::GetID() const
    {
        return m_id;
    }

    template <size_t N>
    void Dependency<N>::SetID(const AZ::Uuid& id)
    {
        m_id = id;
    }

    template <size_t N>
    const AZStd::string& Dependency<N>::GetName() const
    {
        return m_name;
    }

    template <size_t N>
    void Dependency<N>::SetName(const AZStd::string& name)
    {
        m_name = name;
    }

    template <size_t N>
    const AZStd::vector<typename Dependency<N>::Bound>& Dependency<N>::GetBounds() const
    {
        return m_bounds;
    }

    template <size_t N>
    bool Dependency<N>::IsFullfilledBy(const Specifier<N>& spec) const
    {
        using Comp = typename Dependency::Bound::Comparison;

        if (!m_id.IsNull() && !spec.m_id.IsNull())
        {
            if (spec.m_id != m_id)
            {
                return false;
            }
        }

        for (auto && bound : m_bounds)
        {
            if (bound.m_comparison == Comp::TwiddleWakka)
            {
                // Lower bound
                Bound lower;
                lower.m_comparison = Comp::EqualTo | Comp::GreaterThan;
                lower.m_version = bound.m_version;
                lower.m_parseDepth = bound.m_parseDepth;

                // Upper bound
                Bound upper;
                upper.m_comparison = Comp::LessThan;
                upper.m_version = lower.m_version;
                upper.m_parseDepth = bound.m_parseDepth;
                // ~=1.0    becomes >=1.0   <2.0
                // ~=1.2.0  becomes >=1.2.0 <1.3.0
                // ~=1.2.3  becomes >=1.2.3 <1.3.0
                upper.m_version.m_parts[lower.m_parseDepth - 1] = 0;
                upper.m_version.m_parts[lower.m_parseDepth - 2]++;

                if (!lower.MatchesVersion(spec.m_version)
                    || !upper.MatchesVersion(spec.m_version))
                {
                    return false;
                }
            }
            else
            {
                if (!bound.MatchesVersion(spec.m_version))
                {
                    return false;
                }
            }
        }

        return true;
    }

    template <size_t N>
    AZ::Outcome<void, AZStd::string> Dependency<N>::ParseVersions(const AZStd::vector<AZStd::string>& deps)
    {
        using Comp = typename Dependency::Bound::Comparison;

        AZStd::smatch match;
        AZStd::string depStr;

        for (const auto& depStrRaw : deps)
        {
            depStr = depStrRaw;
            AZ::StringFunc::Strip(depStr, " \t");

            if (depStr == "*")
            {
                // If * is in the constraints, allow ANY version of the dependency
                m_bounds.clear();
                return AZ::Success();
            }

            if (AZStd::regex_match(depStr, match, m_namedDependencyRegex) && match.size() >= 4)
            {
                m_name = match[1].str();

                // remove the name prefix before parsing op and version
                AZ::StringFunc::LChop(depStr, m_name.length());
                AZ::StringFunc::Strip(depStr, " \t");
            }

            if (AZStd::regex_match(depStr, match, m_dependencyRegex) && match.size() >= 3)
            {
                AZStd::string op = match[1].str();
                AZStd::string versionStr = match[2].str();

                static const AZStd::array<const char*, 4> invalid_operators{ {
                    "><", "<>", ">>", "<<"
                } };

                for (const char* invalid_op : invalid_operators)
                {
                    if (op == invalid_op)
                    {
                        // invalid operators detected
                        goto invalid_version_str;
                    }
                }

                // Check for twiddle wakka, it's a special case
                if (op == "~=" || op == "~>")
                {
                    // Lower bound
                    Bound bound;
                    bound.m_comparison = Comp::TwiddleWakka;
                    bound.m_parsedString = op + versionStr;
                    auto parseOutcome = ParseVersion(versionStr, bound.m_version);
                    if (!parseOutcome.IsSuccess() || parseOutcome.GetValue() < 2)
                    {
                        // ~=1 not allowed, must specifiy ~=1.0
                        goto invalid_version_str;
                    }
                    bound.m_parseDepth = parseOutcome.GetValue();
                    m_bounds.push_back(bound);
                }
                else
                {
                    Bound current;

                    auto findOperator = [&op, &current](Comp comp, const char* str) -> void
                    {
                        if (op.find(str) != AZStd::string::npos)
                        {
                            current.m_comparison |= comp;
                        }
                    };

                    findOperator(Comp::EqualTo, "=");
                    findOperator(Comp::LessThan, "<");
                    findOperator(Comp::GreaterThan, ">");

                    auto parseOutcome = ParseVersion(versionStr, current.m_version);
                    if (!parseOutcome.IsSuccess())
                    {
                        goto invalid_version_str;
                    }
                    current.m_parseDepth = parseOutcome.GetValue();

                    // fix incomplete version parts
                    // 1.8 -> 1.8.0.0
                    for (AZ::u8 praseDepth = current.m_parseDepth; praseDepth < N; ++praseDepth)
                    {
                        versionStr += ".0";
                    }

                    // since "=" is a valid comparsion string, we want to standardize it to ==
                    // Note: this also converts === to ==
                    if (current.m_comparison == Comp::EqualTo)
                    {
                        op = "==";
                    }

                    current.m_parsedString = op + versionStr;

                    m_bounds.push_back(current);
                }
            }
            else
            {
                goto invalid_version_str;
            }
        }

        return AZ::Success();

    invalid_version_str:
        // Error message
        m_bounds.clear();
        return AZ::Failure(AZStd::string::format(
            "Failed to parse dependency version string \"%s\": invalid format.",
            depStr.c_str()));
    }

    template <size_t N>
    AZ::Outcome<AZ::u8> Dependency<N>::ParseVersion(AZStd::string str, Version<N>& ver)
    {
        AZStd::smatch match;
        AZ::u8 depth = 0;

        while (!str.empty() && depth < N)
        {
            if (AZStd::regex_match(str, match, m_versionRegex))
            {
                ver.m_parts[depth++] = static_cast<typename Version<N>::ComponentType>(AZStd::stoull(match[1].str()));
                str = match[2].str();
            }
            else
            {
                break;
            }
        }

        if (!str.empty())
        {
            return AZ::Failure();
        }

        return AZ::Success(depth);
    }
}
