/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            PatternMatcher::PatternMatcher(const char* pattern, MatchApproach matcher)
                : m_patterns({ AZStd::string(pattern) })
                , m_matcher(matcher)
            {
            }

            PatternMatcher::PatternMatcher(const AZStd::string& pattern, MatchApproach matcher)
                : m_patterns({ pattern })
                , m_matcher(matcher)
            {
            }

            PatternMatcher::PatternMatcher(AZStd::string&& pattern, MatchApproach matcher)
                : m_patterns({ AZStd::move(pattern) })
                , m_matcher(matcher)
            {
            }

            PatternMatcher::PatternMatcher(const AZStd::span<const AZStd::string_view> patterns, MatchApproach matcher)
                : m_matcher(matcher)
            {
                m_patterns.reserve(patterns.size());
                for (auto& pattern : patterns)
                {
                    m_patterns.emplace_back(pattern);
                }
            }

            PatternMatcher::PatternMatcher(const PatternMatcher& rhs)
                : m_patterns(rhs.m_patterns)
                , m_matcher(rhs.m_matcher)
            {
            }

            PatternMatcher::PatternMatcher(PatternMatcher&& rhs)
                : m_patterns(AZStd::move(rhs.m_patterns))
                , m_matcher(rhs.m_matcher)
            {
            }

            PatternMatcher& PatternMatcher::operator=(const PatternMatcher& rhs)
            {
                m_patterns = rhs.m_patterns;
                m_matcher = rhs.m_matcher;
                m_regexMatchers.clear();
                return *this;
            }

            PatternMatcher& PatternMatcher::operator=(PatternMatcher&& rhs)
            {
                m_patterns = AZStd::move(rhs.m_patterns);
                m_matcher = rhs.m_matcher;
                m_regexMatchers.clear();
                return *this;
            }

            bool PatternMatcher::LoadFromJson(rapidjson::Document::ConstMemberIterator member)
            {
                if (!member->value.HasMember("PatternMatcher"))
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Missing element 'PatternMatcher'.");
                    return false;
                }

                if (!member->value.HasMember("Pattern"))
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Missing element 'Pattern'.");
                    return false;
                }

                const auto& patternMatcherValue = member->value["PatternMatcher"];
                if (!patternMatcherValue.IsString())
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Element 'PatternMatcher' is not a string.");
                    return false;
                }

                const auto& patternValue = member->value["Pattern"];
                if (!patternValue.IsString())
                {
                    AZ_TracePrintf(Utilities::WarningWindow, "Element 'Pattern' is not a string.");
                    return false;
                }

                if (AzFramework::StringFunc::Equal(patternMatcherValue.GetString(), "postfix"))
                {
                    m_matcher = MatchApproach::PostFix;
                }
                else if (AzFramework::StringFunc::Equal(patternMatcherValue.GetString(), "prefix"))
                {
                    m_matcher = MatchApproach::PreFix;
                }
                else if (AzFramework::StringFunc::Equal(patternMatcherValue.GetString(), "regex"))
                {
                    m_matcher = MatchApproach::Regex;
                }
                else
                {
                    AZ_TracePrintf(Utilities::WarningWindow,
                        "Element 'PatternMatcher' is no one of the available options postfix, prefix or regex.");
                    return false;
                }

                m_patterns = { patternValue.GetString() };

                return true;
            }

            bool PatternMatcher::MatchesPattern(const char* name, size_t nameLength) const
            {
                return MatchesPattern(AZStd::string_view(name, nameLength));
            }

            bool PatternMatcher::MatchesPattern(AZStd::string_view name) const
            {
                constexpr bool caseSensitive = false;

                switch (m_matcher)
                {
                case MatchApproach::PreFix:
                    for (const auto& pattern : m_patterns)
                    {
                        // Perform a case-insensitive prefix match.
                        if (AZ::StringFunc::StartsWith(name, pattern, caseSensitive))
                        {
                            return true;
                        }
                    }
                    return false;
                case MatchApproach::PostFix:
                    for (const auto& pattern : m_patterns)
                    {
                        // Perform a case-insensitive postfix match.
                        if (AZ::StringFunc::EndsWith(name, pattern, caseSensitive))
                        {
                            return true;
                        }
                    }
                    return false;
                case MatchApproach::Regex:
                {
                    for (size_t index = 0; index < m_patterns.size(); index++)
                    {
                        // Because PatternMatcher can get default constructed and serialized into directly, there's no good place
                        // to initialize the regex matchers on construction, so we'll lazily initialize it here on first use.

                        if (m_regexMatchers.empty())
                        {
                            m_regexMatchers.resize(m_patterns.size());
                        }

                        if (m_regexMatchers[index] == nullptr)
                        {
                            m_regexMatchers[index] = AZStd::make_unique<AZStd::regex>(m_patterns[index], AZStd::regex::extended);
                        }
                        AZStd::cmatch match;
                        if (AZStd::regex_match(name.begin(), name.end(), match, *(m_regexMatchers[index])))
                        {
                            return true;
                        }
                    }
                    return false;
                }
                default:
                    AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_matcher);
                    return false;
                }
            }

            const AZStd::string& PatternMatcher::GetPattern() const
            {
                const static AZStd::string emptyString{};
                return !m_patterns.empty() ? m_patterns.front() : emptyString;
            }

            PatternMatcher::MatchApproach PatternMatcher::GetMatchApproach() const
            {
                return m_matcher;
            }

            void PatternMatcher::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<PatternMatcher>()
                        ->Version(2)
                        ->Field("patterns", &PatternMatcher::m_patterns)
                        ->Field("matcher", &PatternMatcher::m_matcher);

                    EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<PatternMatcher>("Pattern matcher", "")
                            ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->DataElement(Edit::UIHandlers::Default, &PatternMatcher::m_patterns, "Patterns", "The patterns the matcher will check against.")
                            ->DataElement(Edit::UIHandlers::ComboBox, &PatternMatcher::m_matcher, "Matcher", "The used approach for matching.")
                                ->EnumAttribute(MatchApproach::PreFix, "PreFix")
                                ->EnumAttribute(MatchApproach::PostFix, "PostFix")
                                ->EnumAttribute(MatchApproach::Regex, "Regex");
                    }
                }
            }
        } // SceneCore
    } // SceneAPI
} // AZ
