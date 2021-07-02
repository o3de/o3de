/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            //! Save a JSON document to a stream. Otherwise returns a failure with error message.
            AZ::Outcome<void, AZStd::string> WriteJsonToStream(const rapidjson::Document& document, AZ::IO::GenericStream& stream, WriteJsonSettings settings = WriteJsonSettings{})
            {
                AZ::IO::RapidJSONStreamWriter jsonStreamWriter(&stream);

                rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter> writer(jsonStreamWriter);

                if (settings.m_maxDecimalPlaces >= 0)
                {
                    writer.SetMaxDecimalPlaces(settings.m_maxDecimalPlaces);
                }

                if (document.Accept(writer))
                {
                    return AZ::Success();
                }
                else
                {
                    return AZ::Failure(AZStd::string{ "Json Writer failed" });
                }
            }

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

            /**
            * Replaces a value in an ini-style content.
            *
            * \param[in] cfgContents       The contents of the ini-style file
            * \param[in] header            The optional group title for the key
            * \param[in] key               The key of the value to replace
            * \param[in] newValue          The value to assign to the key
            *
            * \returns Void on success, error message on failure.
            *
            * If key has multiple values, it does not preserve other values.
            *      enabled_game_projects = oldProject1, oldProject2, oldProject3
            *      becomes
            *      enabled_game_projects = newProject
            * Cases to handle:
            *      1. Key exists, value is identical -> do nothing
            *          This can occur if a user modifies a config file while the tooling is open.
            *          Example:
            *              1) Load Project Configurator.
            *              2) Modify your bootstrap.cfg to a different project.
            *              3) Tell Project Configurator to set your project to the project you set in #2.
            *      2. Key exists, value is missing -> stomps over key with new key value pair.
            *      3. Key exists, value is different -> stomps over key with new key value pair. Ignores previous values.
            *      4. Key does not exist, header is not required -> insert "key=value" at the end of file.
            *      5. Key does not exist, required header does not exist -> insert "\nHeader\n" at end of file and continue to #6.
            *      6. Key does not exist, required header does exist -> insert "key=value" at next line under the header.
            *          The header will always exist at this point, it was created in the previous check if it was missing.
            */
            AZ::Outcome<void, AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::string& header, const AZStd::string& key, const AZStd::string& value)
            {
                // Generate regex str and replacement str
                AZStd::string lhsStr = key;
                AZStd::string regexStr = BuildConfigKeyValueRegex(lhsStr);
                AZStd::string gameFolderAssignment = "$1" + lhsStr + "$2=$03" + value;

                // Replace the current key with the new value
                // AZStd's regex was not functional at the time this was authored, so convert to std from AZStd.
                std::regex sysGameRegex(regexStr.c_str());
                std::smatch matchResults;
                std::string settingsFileContentsStdStr = cfgContents.c_str();
                bool matchFound = std::regex_search(settingsFileContentsStdStr, matchResults, sysGameRegex);

                std::string result;
                if (matchFound)
                {
                    // Case 1, 2, and 3 - Key value pair exists, stomp over the key with a new value pair.
                    result = std::regex_replace(cfgContents.c_str(), sysGameRegex, gameFolderAssignment.c_str());
                }
                else
                {
                    // Cases 4 through 6 - the key does not exist.
                    result = cfgContents.c_str();
                    std::size_t insertLocation = 0;
                    // Case 5 & 6 - key does not exist, header is required
                    if (!header.empty())
                    {
                        insertLocation = result.find(header.c_str());
                        // Case 6 - key does not exist, header does exist.
                        //      Set the key insertion point to right after the header.
                        if (insertLocation != std::string::npos)
                        {
                            insertLocation += header.length();
                        }
                        // Case 5 - key does not exist, required header does not exist.
                        //      Insert the header at the top of the file, set the key insertion point after the header.
                        else
                        {
                            AZStd::string headerAndCr = AZStd::string::format("\n%s\n", header.c_str());
                            result.insert(result.length(), headerAndCr.c_str());
                            insertLocation = result.length() - 1;
                        }
                    }
                    // Case 4 - key does not exist, header is not required.
                    // Case 5 & 6 - the header has been added.
                    //  Insert the key at the tracked insertion point
                    AZStd::string newConfigData = AZStd::string::format("\n%s=%s", key.c_str(), value.c_str());
                    result.insert(insertLocation, newConfigData.c_str());
                }
                cfgContents = result.c_str();
                return AZ::Success();
            }

            /**
            * Updates a configuration (ini) style contents string representation with a list of replacement string rules
            *
            * \param[in] cfgContents    The contents of the ini file to update
            * \param[in] updateRules    The update rules (list of update strings, see below) to apply
            *
            * The replace string rule format follows the following:
            *       ([header]/)[key]=[value]
            *
            *    where:
            *       header (optional) : Optional group title for the key
            *       key               : The key to lookup or create for the value
            *       value             : The value to update or create
            */
            AZ::Outcome<void, AZStd::string> UpdateCfgContents(AZStd::string& cfgContents, const AZStd::list<AZStd::string>& updateRules)
            {
                AZStd::string updateCfgContents = cfgContents;
                for (const AZStd::string& updateRule : updateRules)
                {
                    // Since we are parsing manually anyways, no need to do a regex validation
                    AZStd::string::size_type headerKeySep = updateRule.find_first_of('/');
                    bool hasHeader = (headerKeySep != AZStd::string::npos);
                    AZStd::string::size_type keyValueSep = updateRule.find_first_of('=', !hasHeader ? 0 : headerKeySep + 1);
                    if (keyValueSep == AZStd::string::npos)
                    {
                        return AZ::Failure(AZStd::string::format("Invalid update config rule '%s'.  Must be in the format ([header]/)[key]=[value]", updateRule.c_str()));
                    }
                    AZStd::string header = hasHeader ? updateRule.substr(0, headerKeySep) : AZStd::string("");
                    AZStd::string key = hasHeader ? updateRule.substr(headerKeySep + 1, keyValueSep - (headerKeySep + 1)) : updateRule.substr(0, keyValueSep);
                    AZStd::string value = updateRule.substr(keyValueSep + 1);

                    auto updateResult = UpdateCfgContents(updateCfgContents, header, key, value);
                    if (!updateResult.IsSuccess())
                    {
                        return updateResult;
                    }
                }
                cfgContents = updateCfgContents;
                return AZ::Success();
            }
        } // namespace Internal

        AZ::Outcome<void, AZStd::string> WriteJsonToString(const rapidjson::Document& document, AZStd::string& jsonText, WriteJsonSettings settings)
        {
            AZ::IO::ByteContainerStream<AZStd::string> stream{ &jsonText };
            return Internal::WriteJsonToStream(document, stream, settings);
        }

        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFromString(AZStd::string_view jsonText)
        {
            if (jsonText.empty())
            {
                return AZ::Failure(AZStd::string("Failed to parse JSON: input string is empty."));
            }

            rapidjson::Document jsonDocument;
            jsonDocument.Parse<rapidjson::kParseCommentsFlag>(jsonText.data(), jsonText.size());
            if (jsonDocument.HasParseError())
            {
                size_t lineNumber = 1;

                const size_t errorOffset = jsonDocument.GetErrorOffset();
                for (size_t searchOffset = jsonText.find('\n');
                    searchOffset < errorOffset && searchOffset < AZStd::string::npos;
                    searchOffset = jsonText.find('\n', searchOffset + 1))
                {
                    lineNumber++;
                }

                return AZ::Failure(AZStd::string::format("JSON parse error at line %zu: %s", lineNumber, rapidjson::GetParseError_En(jsonDocument.GetParseError())));
            }
            else
            {
                return AZ::Success(AZStd::move(jsonDocument));
            }
        }

        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFile(const AZ::IO::Path& jsonFilePath, AZ::IO::FileIOBase* overrideFileIO /*= nullptr*/, size_t maxFileSize /*= AZ::Utils::DefaultMaxFileSize*/)
        {
            AZ::IO::FileIOBase* fileIo = overrideFileIO != nullptr ? overrideFileIO : AZ::IO::FileIOBase::GetInstance();
            if (fileIo == nullptr)
            {
                return AZ::Failure(AZStd::string("No FileIO instance present."));
            }

            // Read into memory first and then parse the JSON, rather than passing a file stream to rapidjson.
            // This should avoid creating a large number of micro-reads from the file.

            auto readResult = AZ::Utils::ReadFile(jsonFilePath.String(), maxFileSize);
            if (!readResult.IsSuccess())
            {
                return AZ::Failure(readResult.GetError());
            }

            AZStd::string jsonContent = readResult.TakeValue();

            auto result = ReadJsonFromString(jsonContent);
            if (!result.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("Failed to load '%s'. %s", jsonFilePath.c_str(), result.GetError().c_str()));
            }
            else
            {
                return result;
            }
        }

        AZ::Outcome<void, AZStd::string> WriteJsonFile(const rapidjson::Document& jsonDoc, const AZ::IO::Path& jsonFilePath, WriteJsonSettings settings)
        {
            // Write the JSON into memory first and then write the file, rather than passing a file stream to rapidjson.
            // This should avoid creating a large number of micro-writes to the file.
            AZStd::string fileContent;
            auto outcome = WriteJsonToString(jsonDoc, fileContent, settings);
            if (!outcome.IsSuccess())
            {
                return outcome;
            }

            return AZ::Utils::WriteFile(fileContent, jsonFilePath.String());
        }

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

        AZ::Outcome<void, AZStd::string> ReplaceInCfgFile(const AZStd::string& filePath, const AZStd::list<AZStd::string>& updateRules)
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

            auto updateContentResult = Internal::UpdateCfgContents(settingsFileContents, updateRules);
            if (!updateContentResult.IsSuccess())
            {
                return updateContentResult;
            }

            // Write the result.
            if (!settingsFile.ReOpen(AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY | AZ::IO::SystemFile::SF_OPEN_TRUNCATE))
            {
                return AZ::Failure(AZStd::string::format("Failed to open settings file %s.", filePath.c_str()));
            }
            auto bytesWritten = settingsFile.Write(settingsFileContents.c_str(), settingsFileContents.size());
            settingsFile.Close();

            if (bytesWritten != settingsFileContents.size())
            {
                return AZ::Failure(AZStd::string::format("Failed to write to config file %s.", filePath.c_str()));
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> ReplaceInCfgFile(const AZStd::string& filePath, const char* header, const AZStd::string& key, const AZStd::string& newValue)
        {
            if (key.length() <= 0)
            {
                return AZ::Failure(AZStd::string("'key' parameter for ReplaceInCfgFile must not be empty"));
            }

            AZStd::string updateRule = (header != nullptr) ? AZStd::string::format("%s/%s=%s", header, key.c_str(), newValue.c_str()) : AZStd::string::format("%s=%s", key.c_str(), newValue.c_str());
            AZStd::list<AZStd::string> updateRules{
                updateRule
            };
            auto replaceResult = ReplaceInCfgFile(filePath, updateRules);
            return replaceResult;
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
