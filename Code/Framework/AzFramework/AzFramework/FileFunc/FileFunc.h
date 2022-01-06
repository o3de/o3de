/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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


