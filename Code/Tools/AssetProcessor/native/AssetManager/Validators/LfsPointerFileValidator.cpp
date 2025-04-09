/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <native/AssetManager/Validators/LfsPointerFileValidator.h>
#include <native/assetprocessor.h>

#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>

namespace AssetProcessor
{
    LfsPointerFileValidator::LfsPointerFileValidator(const AZStd::vector<AZStd::string>& scanDirectories)
    {
        for (const AZStd::string& directory : scanDirectories)
        {
            ParseGitAttributesFile(directory.c_str());
        }
    }

    void LfsPointerFileValidator::ParseGitAttributesFile(const AZStd::string& directory)
    {
        constexpr const char* gitAttributesFileName = ".gitattributes";
        AZStd::string gitAttributesFilePath = AZStd::string::format("%s/%s", directory.c_str(), gitAttributesFileName);
        if (!AzFramework::StringFunc::Path::Normalize(gitAttributesFilePath))
        {
            AZ_Error(AssetProcessor::DebugChannel, false,
                "Failed to normalize %s file path %s.", gitAttributesFileName, gitAttributesFilePath.c_str());
        }

        if (!AZ::IO::FileIOBase::GetInstance()->Exists(gitAttributesFilePath.c_str()))
        {
            return;
        }

        // A gitattributes file is a simple text file that gives attributes to pathnames.
        // Each line in gitattributes file is of form: pattern attr1 attr2 ...
        // Example for LFS pointer file attributes: *.DLL filter=lfs diff=lfs merge=lfs -text
        auto result = AzFramework::FileFunc::ReadTextFileByLine(gitAttributesFilePath, [this](const char* line) -> bool
        {
            // Skip any empty or comment lines
            if (strlen(line) && line[0] != '#')
            {
                AZStd::regex lineRegex("^([^ ]+) filter=lfs diff=lfs merge=lfs -text\\n$");
                AZStd::cmatch matchResult;
                if (AZStd::regex_search(line, matchResult, lineRegex) && matchResult.size() == 2)
                {
                    // The current line matches the LFS attributes format. Record the LFS pointer file path pattern.
                    m_lfsPointerFilePatterns.insert(matchResult[1]);
                }
            }

            return true;
        });

        AZ_Error(AssetProcessor::DebugChannel, result.IsSuccess(), result.GetError().c_str());
    }

    bool LfsPointerFileValidator::IsLfsPointerFile(const AZStd::string& filePath)
    {
        return AZ::IO::FileIOBase::GetInstance()->Exists(filePath.c_str()) &&
            CheckLfsPointerFilePathPattern(filePath) &&
            CheckLfsPointerFileContent(filePath);
    }

    AZStd::set<AZStd::string> LfsPointerFileValidator::GetLfsPointerFilePathPatterns()
    {
        return m_lfsPointerFilePatterns;
    }

    bool LfsPointerFileValidator::CheckLfsPointerFilePathPattern(const AZStd::string& filePath)
    {
        bool matches = false;
        for (const AZStd::string& pattern : GetLfsPointerFilePathPatterns())
        {
            if (AZStd::wildcard_match(pattern, filePath.c_str()))
            {
                // The file path matches one of the known LFS pointer file path patterns.
                matches = true;
                break;
            }
        }

        return matches;
    }

    bool LfsPointerFileValidator::CheckLfsPointerFileContent(const AZStd::string& filePath)
    {
        // Content rules for LFS pointer files (https://github.com/git-lfs/git-lfs/blob/main/docs/spec.md):
        // 1. Pointer files are text files which MUST contain only UTF-8 characters.
        // 2. Each line MUST be of the format{key} {value}\n(trailing unix newline). The required keys are: "version", "oid" and "size".
        // 3. Only a single space character between {key} and {value}.
        // 4. Keys MUST only use the characters [a-z] [0-9] . -.
        // 5. The first key is always version.
        // 6. Lines of key/value pairs MUST be sorted alphabetically in ascending order (with the exception of version, which is always first).
        // 7. Values MUST NOT contain return or newline characters.
        // 8. Pointer files MUST be stored in Git with their executable bit matching that of the replaced file.
        // 9. Pointer files are unique: that is, there is exactly one valid encoding for a pointer file.
        AZStd::vector<AZStd::string> fileKeys;
        bool contentCheckSucceeded = true;
        auto result = AzFramework::FileFunc::ReadTextFileByLine(filePath,
            [&fileKeys, &contentCheckSucceeded](const char* line) -> bool
        {
            constexpr const char* lfsVersionKey = "version";
            AZStd::regex lineRegex("^([a-z0-9\\.-]+) ([^\\r\\n]+)\\n$");
            AZStd::cmatch matchResult;
            if (!AZStd::regex_search(line, matchResult, lineRegex) ||
                matchResult.size() <= 2 ||
                (fileKeys.size() == 0 && matchResult[1] != lfsVersionKey) ||
                (fileKeys.size() > 1 && matchResult[1] < fileKeys[fileKeys.size() - 1]))
            {
                // The current line doesn't match the LFS pointer file content rules above.
                // Return early in this case since the file is not an LFS content file.
                contentCheckSucceeded = false;
                return false;
            }

            fileKeys.emplace_back(matchResult[1]);
            return true;
        });
        contentCheckSucceeded &= result.IsSuccess();

        if (contentCheckSucceeded)
        {
            // Check whether all the required keys exist in the LFS pointer file.
            const AZStd::vector<AZStd::string> RequiredKeys = { "version", "oid", "size" };
            size_t requiredKeyIndex = 0, fileKeyIndex = 0;
            while (requiredKeyIndex < RequiredKeys.size() && fileKeyIndex < fileKeys.size())
            {
                if (RequiredKeys[requiredKeyIndex] == fileKeys[fileKeyIndex])
                {
                    ++requiredKeyIndex;
                }

                ++fileKeyIndex;
            }
            contentCheckSucceeded &= (requiredKeyIndex == RequiredKeys.size());
        }

        return contentCheckSucceeded;
    }
}
