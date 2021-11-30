/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/FileFunc/FileFunc.h>
#include <regex>

#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/string/string.h>

#include <AzFramework/StringFunc/StringFunc.h>

#define MAX_TEXT_LINE_BUFFER_SIZE   512

namespace AzFramework
{
    namespace FileFunc
    {
        namespace Internal
        {
            static bool FindFilesInPath(const AZStd::string& folder, const AZStd::string& filter, bool recurseSubFolders, AZStd::list<AZStd::string>& fileList)
            {
                AZStd::string file_filter = folder;
                file_filter.append(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING).append(filter.empty() ? "*" : filter.c_str());

                AZ::IO::SystemFile::FindFiles(file_filter.c_str(), [folder, filter, recurseSubFolders, &fileList](const char* item, bool is_file)
                    {
                        // Skip the '.' and '..' folders
                        if ((azstricmp(".", item) == 0) || (azstricmp("..", item) == 0))
                        {
                            return true;
                        }

                        // Continue if we can
                        AZStd::string fullPath;
                        if (!AzFramework::StringFunc::Path::Join(folder.c_str(), item, fullPath))
                        {
                            return false;
                        }

                        if (is_file)
                        {
                            if (AZ::IO::NameMatchesFilter(item, filter.c_str()))
                            {
                                fileList.push_back(fullPath);
                            }
                        }
                        else
                        {
                            if (recurseSubFolders)
                            {
                                return FindFilesInPath(fullPath, filter, recurseSubFolders, fileList);
                            }
                        }
                        return true;
                    });
                return true;
            }

            /**
            * Creates a regex pattern string to find anything matching the pattern "[key] = [value]", with all possible whitespace variations accounted for.
            *
            * \param[in] key         The key value for the pattern
            * \return string of the regex pattern based on the key
            */
            static AZStd::string BuildConfigKeyValueRegex(const AZStd::string& key)
            {
                return AZStd::string("(^|\\n\\s*)" + key + "([ \\t]*)=([ \\t]*)(.*)");
            }
        } // namespace Internal

        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> FindFilesInPath(const AZStd::string& folder, const AZStd::string& filter, bool recurseSubFolders)
        {
            AZStd::list<AZStd::string> fileList;

            if (!Internal::FindFilesInPath(folder, filter, recurseSubFolders, fileList))
            {
                return AZ::Failure(AZStd::string::format("Error finding files in path '%s'", folder.c_str()));
            }
            else
            {
                return AZ::Success(fileList);
            }
        }

        AZ::Outcome<void, AZStd::string> ReadTextFileByLine(const AZStd::string& filePath, AZStd::function<bool(const char* line)> perLineCallback)
        {
            FILE* file_handle = nullptr;
            azfopen(&file_handle, filePath.c_str(), "rt");
            if (!file_handle)
            {
                return AZ::Failure(AZStd::string::format("Error opening file '%s' for reading", filePath.c_str()));
            }
            char line[MAX_TEXT_LINE_BUFFER_SIZE] = { '\0' };
            while (fgets(line, sizeof(line), static_cast<FILE*>(file_handle)) != nullptr)
            {
                if (line[0])
                {
                    if (!perLineCallback(line))
                    {
                        break;
                    }
                }
            }
            fclose(file_handle);
            return AZ::Success();
        }

        AZ::Outcome<AZStd::string, AZStd::string> GetValueForKeyInCfgFile(const AZStd::string& filePath, const char* key)
        {
            AZ::Outcome<AZStd::string, AZStd::string> subCommandResult = GetCfgFileContents(filePath);
            if (!subCommandResult.IsSuccess())
            {
                return subCommandResult;
            }

            subCommandResult = GetValueForKeyInCfgFileContents(subCommandResult.GetValue(), key);
            if (!subCommandResult.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("%s in file %s", subCommandResult.GetError().c_str(), filePath.c_str()));
            }
            return subCommandResult;
        }

        AZ::Outcome<AZStd::string, AZStd::string> GetValueForKeyInCfgFileContents(const AZStd::string& configFileContents, const char* key)
        {
            // Generate regex str and replacement str
            AZStd::string lhsStr = key;
            AZStd::string regexStr = Internal::BuildConfigKeyValueRegex(lhsStr);

            std::regex sysGameRegex(regexStr.c_str());
            std::cmatch matchResult;
            if (!std::regex_search(configFileContents.c_str(), matchResult, sysGameRegex))
            {
                return AZ::Failure(AZStd::string::format("Could not find key %s", key));
            }

            if (matchResult.size() <= 4)
            {
                return AZ::Failure(AZStd::string::format("No results for key %s", key));
            }

            return AZ::Success(AZStd::string(matchResult[4].str().c_str()));
        }

        AZ::Outcome<AZStd::string, AZStd::string> GetCfgFileContents(const AZStd::string& filePath)
        {
            if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Cfg file '%s' does not exist.", filePath.c_str()));
            }

            // Load the settings file into a string
            AZ::IO::SystemFile settingsFile;
            if (!settingsFile.Open(filePath.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                return AZ::Failure(AZStd::string::format("Error reading cfg file '%s'.", filePath.c_str()));
            }

            AZStd::string settingsFileContents(settingsFile.Length(), '\0');
            settingsFile.Read(settingsFileContents.size(), settingsFileContents.data());
            settingsFile.Close();
            return AZ::Success(settingsFileContents);

        }

        AZ::Outcome < AZStd::list<AZStd::string>, AZStd::string> FindFileList(const AZStd::string& pathToStart, const char* pattern, bool recurse)
        {
            auto fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                AZ_Error("FindFilesList", false, "Couldn't get fileIO Instance");
                return AZ::Failure(AZStd::string("Couldn't get FileIO"));
            }

            AZStd::deque<AZStd::string> foldersToSearch;
            foldersToSearch.push_back(pathToStart);
            AZ::IO::Result searchResult = AZ::IO::ResultCode::Success;

            AZStd::list<AZStd::string> filesFound;
            while (foldersToSearch.size() && searchResult == AZ::IO::ResultCode::Success)
            {
                AZStd::string folderName = foldersToSearch.front();
                foldersToSearch.pop_front();

                searchResult = fileIO->FindFiles(folderName.c_str(), "*",
                    [&](const char* fileName)
                {
                    if (recurse && fileIO->IsDirectory(fileName))
                    {
                        foldersToSearch.push_back(fileName);
                    }
                    else if (AZ::IO::NameMatchesFilter(fileName, pattern))
                    {
                        filesFound.emplace_back(fileName);
                    }
                    return true;
                }
                );
            }
            return AZ::Success(filesFound);
        }
    } // namespace FileFunc
} // namespace AzFramework
