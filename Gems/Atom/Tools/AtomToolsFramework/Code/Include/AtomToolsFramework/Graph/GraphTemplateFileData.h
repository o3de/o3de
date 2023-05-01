/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AtomToolsFramework
{
    //! GraphTemplateFileData Is responsible for loading in the content of a text file, Tokenizing it into multiple lines that will be
    //! stored in hey victor strings. This allows for easier and individual processing of each line.
    class GraphTemplateFileData
    {
    public:
        AZ_RTTI(GraphTemplateFileData, "{4E2FA046-1D3F-4E5F-B1BA-6E874B3A31B7}");
        AZ_CLASS_ALLOCATOR(GraphTemplateFileData, AZ::SystemAllocator);

        GraphTemplateFileData(){};
        virtual ~GraphTemplateFileData(){};

        //! Loads and tokenizes template data from a text file at the specified, absolute path.
        bool Load(const AZStd::string& path);

        //! Concatenates and saves template lines to read text file at the specified, absolute path.
        bool Save(const AZStd::string& path) const;

        //! Return true if the file has been modified since the last time it was loaded.
        bool IsReloadRequired() const;

        //! Returns true if the file was loaded and not empty.
        bool IsLoaded() const;

        //! Returns true if settings are enabled for logging template file status and modifications.
        bool IsLoggingEnabled() const;

        //! Returns the last loaded file path.
        const AZStd::string& GetPath() const;

        //! Returns a container of all of the template data lines.
        const AZStd::vector<AZStd::string>& GetLines() const;

        using LineGenerationFn = AZStd::function<AZStd::vector<AZStd::string>(const AZStd::string&)>;

        //! Find and replace text using regular expressions.
        void ReplaceSymbol(const AZStd::string& findText, const AZStd::string& replaceText);

        //! Search for marked up blocks of text from a template and replace lines between them with lines provided by a function.
        void ReplaceLinesInBlock(
            const AZStd::string& blockBeginToken, const AZStd::string& blockEndToken, const LineGenerationFn& lineGenerationFn);

    private:
        AZStd::string m_path;
        AZStd::vector<AZStd::string> m_lines;
        AZ::u64 m_lastModifiedTime = {};
    };
} // namespace AtomToolsFramework
