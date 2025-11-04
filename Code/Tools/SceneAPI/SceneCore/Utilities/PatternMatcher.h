#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/JSON/document.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneCore
        {
            // PatternMatcher stores a pattern and a matching approach for later use.
            //      Strings can then be checked against the stored pattern.
            //      The supported approaches are:
            //      Prefix - Matches if the string starts with the stored pattern.
            //      Postfix - Matches if the string ends with the stored pattern.
            //      Regex - Matches if the string matches the given regular expression.
            class PatternMatcher
            {
            public:
                AZ_RTTI(PatternMatcher, "{F043EC4E-FA29-4A5E-BEF6-13C661048FC4}");
                virtual ~PatternMatcher() = default;

                enum class MatchApproach
                {
                    PreFix,
                    PostFix,
                    Regex
                };

                PatternMatcher() = default;
                SCENE_CORE_API PatternMatcher(const char* pattern, MatchApproach matcher);
                SCENE_CORE_API PatternMatcher(const AZStd::string& pattern, MatchApproach matcher);
                SCENE_CORE_API PatternMatcher(AZStd::string&& pattern, MatchApproach matcher);
                SCENE_CORE_API PatternMatcher(const AZStd::span<const AZStd::string_view> patterns, MatchApproach matcher);
                SCENE_CORE_API PatternMatcher(const PatternMatcher& rhs);
                SCENE_CORE_API PatternMatcher(PatternMatcher&& rhs);

                SCENE_CORE_API PatternMatcher& operator=(const PatternMatcher& rhs);
                SCENE_CORE_API PatternMatcher& operator=(PatternMatcher&& rhs);

                SCENE_CORE_API bool LoadFromJson(rapidjson::Document::ConstMemberIterator member);

                SCENE_CORE_API bool MatchesPattern(const char* name, size_t nameLength) const;
                SCENE_CORE_API bool MatchesPattern(AZStd::string_view name) const;

                SCENE_CORE_API const AZStd::string& GetPattern() const;
                SCENE_CORE_API MatchApproach GetMatchApproach() const;

                static void Reflect(AZ::ReflectContext* context);
            private:
                AZStd::vector<AZStd::string> m_patterns;
                MatchApproach m_matcher = MatchApproach::PostFix;
                mutable AZStd::vector<AZStd::unique_ptr<AZStd::regex>> m_regexMatchers;
            };
        } // SceneCore
    } // SceneAPI
} // AZ
