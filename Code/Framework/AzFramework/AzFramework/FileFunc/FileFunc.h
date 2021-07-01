/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/std/any.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Utils/Utils.h>

//! Namespace for file functions.
/*!
The FileFunc namespace is where we put some higher level file reading and writing
operations.
*/
namespace AzFramework
{
    namespace FileFunc
    {
        struct WriteJsonSettings
        {
            int m_maxDecimalPlaces = -1; // -1 means use default
        };

        /*
        * Read a string into a JSON document
        *
        * \param[in] jsonText The string of JSON to parse into a JSON document.
        * \return Outcome<rapidjson::Document) Returns a failure with error message if the content is not valid JSON.
        */
        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFromString(AZStd::string_view jsonText);

        /*
        * Read a text file into a JSON document
        *
        * \param[in] jsonFilePath     The path to the JSON file path to open
        * \param[in] overrideFileIO   Optional file IO instance to use.  If null, use the Singleton instead
        * \param[in] maxFileSize      The maximum size, in bytes, of the file to read. Defaults to 1 MB.
        * \return Outcome(rapidjson::Document) Returns a failure with error message if the content is not valid JSON. 
        */
        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFile(const AZ::IO::Path& jsonFilePath, AZ::IO::FileIOBase* overrideFileIO = nullptr, size_t maxFileSize = AZ::Utils::DefaultMaxFileSize);

        /**
        * Write a JSON document to a string
        *
        * \param[in]  jsonDoc        The JSON document to write to a text file
        * \param[out] jsonFilePath   The string that the JSON text will be written to.
        * \param[in]  settings       Settings to pass along to the JSON writer. 
        * \return StringOutcome Saves the JSON document to text. Otherwise returns a failure with error message.
        */
        AZ::Outcome<void, AZStd::string> WriteJsonToString(const rapidjson::Document& document, AZStd::string& jsonText, WriteJsonSettings settings = WriteJsonSettings{});

        /**
        * Write a JSON document to a text file
        *
        * \param[in] jsonDoc        The JSON document to write to a text file
        * \param[in] jsonFilePath   The path to the JSON file path to write to
        * \param[in] settings       Settings to pass along to the JSON writer.
        * \return StringOutcome Saves the JSON document to a file. Otherwise returns a failure with error message.
        */
        AZ::Outcome<void, AZStd::string> WriteJsonFile(const rapidjson::Document& jsonDoc, const AZ::IO::Path& jsonFilePath, WriteJsonSettings settings = WriteJsonSettings{});

        /**
        * Find all the files in a path based on an optional filter.  Recurse if requested.
        *
        * \param[in]  folder               The folder to find the files in
        * \param[in]  filter               Optional file filter (i.e. wildcard *) to apply
        * \param[in]  recurseSubFolders    Option to recurse into any subfolders found in the search
        * \param[out] fileList             List of files found from the search
        * \return true if the files where found, false if an error occurred.
        */
        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> FindFilesInPath(const AZStd::string& folder, const AZStd::string& filter, bool recurseSubFolders);


        /**
        * Helper function to read a text file line by line and process each line through a function ptr input

        * \param[in] filePath          The path to the config file
        * \param[in] perLineCallback   Function ptr to a function that takes in a const char* as input, and returns false to stop processing or true to continue to the next line
        * \return true if the file was open and read successfully, false if not
        */
        AZ::Outcome<void, AZStd::string> ReadTextFileByLine(const AZStd::string& filePath, AZStd::function<bool(const char* line)> perLineCallback);

        /**
        * Replaces a value in an ini-style file.
        *
        * \param[in] filePath          The path to the config file to update
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
        AZ::Outcome<void, AZStd::string> ReplaceInCfgFile(const AZStd::string& filePath, const AZStd::list<AZStd::string>& updateRules);

        /**
        * Replaces a value in an ini-style file.
        *
        * \param[in] filePath       The path to the config file to update
        * \param[in] updateRule     The update rule (list of update strings, see below) to apply
        *
        * The replace string rule format follows the following:
        *       ([header]/)[key]=[value]
        *
        *    where:
        *       header (optional) : Optional group title for the key
        *       key               : The key to lookup or create for the value
        *       value             : The value to update or create
        */
        AZ::Outcome<void, AZStd::string> ReplaceInCfgFile(const AZStd::string& filePath, const char* header, const AZStd::string& key, const AZStd::string& newValue);

        /**
        * Gets the value(s) for a key in an INI style config file.
        *
        * \param[in] filePath          The path to the config file
        * \param[in] key               The key of the value to find
        *
        * \returns Value(s) on success, error message on failure.
        */
        AZ::Outcome<AZStd::string, AZStd::string> GetValueForKeyInCfgFile(const AZStd::string& filePath, const char* key);

        /**
        * Gets the value(s) for a key in an INI style config file with the given contents.
        *
        * \param[in] configFileContents The contents of a config file in a string
        * \param[in] key                The key of the value to find
        *
        * \returns Value(s) on success, error message on failure.
        */
        AZ::Outcome<AZStd::string, AZStd::string> GetValueForKeyInCfgFileContents(const AZStd::string& configFileContents, const char* key);

        /**
        * Gets the contents of an INI style config file as a string.
        *
        * \param[in] filePath          The path to the config file
        *
        * \returns Contents of filePath on success, error message on failure.
        */
        AZ::Outcome<AZStd::string, AZStd::string> GetCfgFileContents(const AZStd::string& filePath);

        /**
        * Find a list of files using FileIO GetInstance
        *
        * \param[in] pathToStart The folder to start at
        * \param[in] pattern The wildcard pattern to match against
        * \param[in] recurse - whether to search directories underneath pathToStart recursively
        * \returns AZ::Success and a list of files of matches found, an error string on an empty list
        */
        AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> FindFileList(const AZStd::string& pathToStart, const char* pattern, bool recurse);
    } // namespace FileFunc
} // namespace AzFramework


