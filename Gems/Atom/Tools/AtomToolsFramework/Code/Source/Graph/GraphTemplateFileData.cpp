/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphTemplateFileData.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/FileIO.h>

namespace AtomToolsFramework
{
    bool GraphTemplateFileData::Load(const AZStd::string& path)
    {
        m_path.clear();
        m_lines.clear();

        AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "Loading template file: %s\n", path.c_str());

        // Attempt to load the template file to do symbol substitution and inject any code or data
        if (auto result = AZ::Utils::ReadFile(path))
        {
            m_path = path;
            m_lastModifiedTime = AZ::IO::FileIOBase::GetInstance()->ModificationTime(path.c_str());

            // Tokenize the entire template file into individual lines that can be evaluated, removed, replaced, and have
            // content injected between them
            m_lines.reserve(1000);
            AZ::StringFunc::Tokenize(result.GetValue(), m_lines, '\n', true, true);
            AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "Loading template file succeeded: %s\n", path.c_str());
            return true;
        }

        AZ_Error("GraphTemplateFileData", false, "Loading template file failed: %s\n", path.c_str());
        return false;
    }

    bool GraphTemplateFileData::Save(const AZStd::string& path) const
    {
        AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "Saving generated file: %s\n", path.c_str());

        AZStd::string templateOutputText;
        AZ::StringFunc::Join(templateOutputText, m_lines, '\n');
        templateOutputText += '\n';

        // Save the file generated from the template to the same folder as the graph.
        if (AZ::Utils::WriteFile(templateOutputText, path).IsSuccess())
        {
            AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "Saving generated file succeeded: %s\n", path.c_str());
            return true;
        }

        AZ_Error("GraphTemplateFileData", false, "Saving generated file failed: %s\n", path.c_str());
        return false;
    }

    bool GraphTemplateFileData::IsReloadRequired() const
    {
        return !IsLoaded() || m_lastModifiedTime < AZ::IO::FileIOBase::GetInstance()->ModificationTime(m_path.c_str());
    }

    bool GraphTemplateFileData::IsLoaded() const
    {
        return !m_path.empty() && !m_lines.empty();
    }

    bool GraphTemplateFileData::IsLoggingEnabled() const
    {
        return GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/EnableLogging", false);
    }

    const AZStd::string& GraphTemplateFileData::GetPath() const
    {
        return m_path;
    }

    const AZStd::vector<AZStd::string>& GraphTemplateFileData::GetLines() const
    {
        return m_lines;
    }

    void GraphTemplateFileData::ReplaceSymbol(const AZStd::string& findText, const AZStd::string& replaceText)
    {
        ReplaceSymbolsInContainer(findText, replaceText, m_lines);
    }

    void GraphTemplateFileData::ReplaceLinesInBlock(
        const AZStd::string& blockBeginToken, const AZStd::string& blockEndToken, const LineGenerationFn& lineGenerationFn)
    {
        AZ_TracePrintf_IfTrue(
            "GraphTemplateFileData",
            IsLoggingEnabled(),
            "Inserting %s lines into template file: %s\n",
            blockBeginToken.c_str(),
            m_path.c_str());

        auto blockBeginItr = AZStd::find_if(
            m_lines.begin(),
            m_lines.end(),
            [&blockBeginToken](const AZStd::string& line)
            {
                return AZ::StringFunc::Contains(line, blockBeginToken);
            });

        while (blockBeginItr != m_lines.end())
        {
            AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "*blockBegin: %s\n", (*blockBeginItr).c_str());

            // We have to insert one line at a time because AZStd::vector does not include a standard
            // range insert that returns an iterator
            const auto& linesToInsert = lineGenerationFn(*blockBeginItr);
            for (const auto& lineToInsert : linesToInsert)
            {
                ++blockBeginItr;
                blockBeginItr = m_lines.insert(blockBeginItr, lineToInsert);

                AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "lineToInsert: %s\n", lineToInsert.c_str());
            }

            if (linesToInsert.empty())
            {
                AZ_TracePrintf_IfTrue(
                    "GraphTemplateFileData", IsLoggingEnabled(), "Nothing was generated. This block will remain unmodified.\n");
            }

            ++blockBeginItr;

            // From the last line that was inserted, locate the end of the insertion block
            auto blockEndItr = AZStd::find_if(
                blockBeginItr,
                m_lines.end(),
                [&blockEndToken](const AZStd::string& line)
                {
                    return AZ::StringFunc::Contains(line, blockEndToken);
                });

            AZ_TracePrintf_IfTrue("GraphTemplateFileData", IsLoggingEnabled(), "*blockEnd: %s\n", (*blockEndItr).c_str());

            if (!linesToInsert.empty())
            {
                // If any new lines were inserted, erase pre-existing lines the template might have had between the begin and end blocks
                blockEndItr = m_lines.erase(blockBeginItr, blockEndItr);
            }

            // Search for another insertion point
            blockBeginItr = AZStd::find_if(
                blockEndItr,
                m_lines.end(),
                [&blockBeginToken](const AZStd::string& line)
                {
                    return AZ::StringFunc::Contains(line, blockBeginToken);
                });
        }
    }
} // namespace AtomToolsFramework
