/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
                : m_pattern(pattern)
                , m_matcher(matcher)
            {
            }

            PatternMatcher::PatternMatcher(const AZStd::string& pattern, MatchApproach matcher)
                : m_pattern(pattern)
                , m_matcher(matcher)
            {
            }

            PatternMatcher::PatternMatcher(AZStd::string&& pattern, MatchApproach matcher)
                : m_pattern(AZStd::move(pattern))
                , m_matcher(matcher)
            {
            }

            PatternMatcher::PatternMatcher(PatternMatcher&& rhs)
                : m_pattern(AZStd::move(rhs.m_pattern))
                , m_matcher(rhs.m_matcher)
            {
            }

            PatternMatcher& PatternMatcher::operator=(PatternMatcher&& rhs)
            {
                m_pattern = AZStd::move(rhs.m_pattern);
                m_matcher = rhs.m_matcher;
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

                m_pattern = patternValue.GetString();

                return true;
            }

            bool PatternMatcher::MatchesPattern(const char* name, size_t nameLength) const
            {
                switch (m_matcher)
                {
                case MatchApproach::PreFix:
                    return AzFramework::StringFunc::Equal(name, m_pattern.c_str(), false, m_pattern.size());
                case MatchApproach::PostFix:
                {
                    if (m_pattern.size() > nameLength)
                    {
                        return false;
                    }
                    size_t offset = nameLength - m_pattern.size();
                    return AzFramework::StringFunc::Equal(name + offset, m_pattern.c_str());
                }
                case MatchApproach::Regex:
                {
                    AZStd::regex comparer(m_pattern, AZStd::regex::extended);
                    AZStd::smatch match;
                    return AZStd::regex_match(name, match, comparer);
                }
                default:
                    AZ_Assert(false, "Unknown option '%i' for pattern matcher.", m_matcher);
                    return false;
                }
            }

            bool PatternMatcher::MatchesPattern(const AZStd::string& name) const
            {
                return MatchesPattern(name.c_str(), name.length());
            }

            const AZStd::string& PatternMatcher::GetPattern() const
            {
                return m_pattern;
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
                        ->Version(1)
                        ->Field("pattern", &PatternMatcher::m_pattern)
                        ->Field("matcher", &PatternMatcher::m_matcher);

                    EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<PatternMatcher>("Pattern matcher", "")
                            ->ClassElement(Edit::ClassElements::EditorData, "")
                            ->Attribute(Edit::Attributes::AutoExpand, true)
                            ->DataElement(Edit::UIHandlers::Default, &PatternMatcher::m_pattern, "Pattern", "The pattern the matcher will check against.")
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
