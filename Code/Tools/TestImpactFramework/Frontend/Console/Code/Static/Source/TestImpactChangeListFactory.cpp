/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <TestImpactChangeListFactory.h>
#include <TestImpactFramework/TestImpactException.h>

#include <AzCore/std/optional.h>

namespace TestImpact
{
    namespace Utils
    {
        AZStd::vector<AZStd::string> split(const AZStd::string& str, const AZStd::string& delimiter)
        {
            AZStd::vector<AZStd::string> strings;

            for (auto p = str.data(), end = p + str.length(); p != end; p += ((p == end) ? 0 : delimiter.length()))
            {
                const auto pre = p;
                p = AZStd::search(pre, end, delimiter.cbegin(), delimiter.cend());

                if (p != pre)
                {
                    strings.emplace_back(pre, p - pre);
                }
            }

            return strings;
        }
    } // namespace Utils

    namespace UnifiedDiff
    {
        class UnifiedDiffParser
        {
        public:
            ChangeList Parse(const AZStd::string& unifiedDiff);

        private:
            AZStd::optional<AZStd::string> GetTargetFile(const AZStd::string& targetFile);
            ChangeList GenerateChangelist(
                const AZStd::vector<AZStd::optional<AZStd::string>>& src,
                const AZStd::vector<AZStd::optional<AZStd::string>>& dst);

            const AZStd::string m_srcFilePrefix = "--- ";
            const AZStd::string m_dstFilePrefix = "+++ ";
            const AZStd::string m_gitTargetPrefix = "b/";
            const AZStd::string m_perforceTargetPrefix = "/b/";
            const AZStd::string m_renameFromPrefix = "rename from ";
            const AZStd::string m_renameToPrefix = "rename to ";
            const AZStd::string m_nullFile = "/dev/null";
            bool m_hasGitHeader = false;
        };

        AZStd::optional<AZStd::string> UnifiedDiffParser::GetTargetFile(const AZStd::string& targetFile)
        {
            size_t startIndex = 0;

            if (targetFile.starts_with(m_renameFromPrefix))
            {
                startIndex = m_renameFromPrefix.length();
            }
            else if (targetFile.starts_with(m_renameToPrefix))
            {
                startIndex = m_renameToPrefix.length();
            }
            else if (targetFile.find(m_nullFile) != AZStd::string::npos)
            {
                return AZStd::nullopt;
            }
            else
            {
                startIndex = m_hasGitHeader ? m_gitTargetPrefix.size() + m_dstFilePrefix.size()
                                            : m_perforceTargetPrefix.size() + m_dstFilePrefix.size();
            }

            const auto endIndex = targetFile.find('\t');

            if (endIndex != AZStd::string::npos)
            {
                return targetFile.substr(startIndex, endIndex - startIndex);
            }

            return targetFile.substr(startIndex);
        }

        ChangeList UnifiedDiffParser::GenerateChangelist(
            const AZStd::vector<AZStd::optional<AZStd::string>>& src, const AZStd::vector<AZStd::optional<AZStd::string>>& dst)
        {
            AZ_TestImpact_Eval(src.size() == dst.size(), Exception, "Change list source and destination file count mismatch");
            ChangeList changelist;

            for (size_t i = 0; i < src.size(); i++)
            {
                if (!src[i].has_value())
                {
                    changelist.m_createdFiles.emplace_back(dst[i].value());
                }
                else if (!dst[i].has_value())
                {
                    changelist.m_deletedFiles.emplace_back(src[i].value());
                }
                else if (src[i] != dst[i])
                {
                    changelist.m_deletedFiles.emplace_back(src[i].value());
                    changelist.m_createdFiles.emplace_back(dst[i].value());
                }
                else
                {
                    changelist.m_updatedFiles.emplace_back(src[i].value());
                }
            }

            return changelist;
        }

        ChangeList UnifiedDiffParser::Parse(const AZStd::string& unifiedDiff)
        {
            const AZStd::string GitHeader = "diff --git";
            const auto lines = Utils::split(unifiedDiff, "\n");

            AZStd::vector<AZStd::optional<AZStd::string>> src;
            AZStd::vector<AZStd::optional<AZStd::string>> dst;

            for (const auto& line : lines)
            {
                if (line.starts_with(GitHeader))
                {
                    m_hasGitHeader = true;
                }
                else if (line.starts_with(m_srcFilePrefix))
                {
                    src.emplace_back(GetTargetFile(line));
                }
                else if (line.starts_with(m_dstFilePrefix))
                {
                    dst.emplace_back(GetTargetFile(line));
                }
                else if (line.starts_with(m_renameFromPrefix))
                {
                    src.emplace_back(GetTargetFile(line));
                }
                else if (line.starts_with(m_renameToPrefix))
                {
                    dst.emplace_back(GetTargetFile(line));
                }
            }

            return GenerateChangelist(src, dst);
        }

        ChangeList ChangeListFactory(const AZStd::string& unifiedDiff)
        {
            AZ_TestImpact_Eval(!unifiedDiff.empty(), Exception, "Unified diff is empty");
            UnifiedDiffParser diff;
            ChangeList changeList = diff.Parse(unifiedDiff);
            AZ_TestImpact_Eval(
                !changeList.m_createdFiles.empty() ||
                !changeList.m_updatedFiles.empty() ||
                !changeList.m_deletedFiles.empty(),
                Exception, "The unified diff contained no changes");
            return changeList;
        }
    } // namespace UnifiedDiff
} // namespace TestImpact
