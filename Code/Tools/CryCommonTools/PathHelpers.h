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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_PATHHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_PATHHELPERS_H
#pragma once

#include <CryString.h>

namespace PathHelpers
{
    // checks to see what the extension is in a string path
    // returns the extension if found or an empty string if not found
    string FindExtension(const string& path);
    wstring FindExtension(const wstring& path);

    // replace an extension of a string path with a new specified extension
    // returns a string with the replaced extension or the original string if unable to replace the extension
    string ReplaceExtension(const string& path, const string& newExtension);
    wstring ReplaceExtension(const wstring& path, const wstring& newExtension);

    // removes the extension of a specified string path
    // returns a string with the extension removed or the original string if no extension was found
    string RemoveExtension(const string& path);
    wstring RemoveExtension(const wstring& path);

    // "abc/def/ghi" -> "abc/def"
    // "abc/def/ghi/" -> "abc/def/ghi"
    // "/" -> "/"
    // gets the directory path out of a specified string path
    // returns a string of the directory path
    string GetDirectory(const string& path);
    wstring GetDirectory(const wstring& path);

    // gets the file name out of a specified string path
    // returns a string of the file name
    string GetFilename(const string& path);
    wstring GetFilename(const wstring& path);

    // add a backslash to a specified path if it doesn't already have a separator
    // returns a path with the appended backslash unless there was already a separator
    string AddSeparator(const string& path);
    wstring AddSeparator(const wstring& path);

    // removes a forward slash or backslash from the end of a specified string path if found
    // returns a string with the separator removed
    string RemoveSeparator(const string& path);
    wstring RemoveSeparator(const wstring& path);

    // removes extra forward slashes and backslashes if they're contained within the string path
    // returns a string with the extra forward slashes and backslashes removed
    string RemoveDuplicateSeparators(const string& path);
    wstring RemoveDuplicateSeparators(const wstring& path);

    // It's not allowed to pass an absolute path in path2.
    // Join(GetDirectory(fname), GetFilename(fname)) returns fname.
    // merges two string paths together into one
    // returns the merged string paths
    string Join(const string& path1, const string& path2);
    wstring Join(const wstring& path1, const wstring& path2);

    // checks to see if the path is a relative path
    // returns true if it is or false if it is not
    bool IsRelative(const string& path);
    bool IsRelative(const wstring& path);

    // converts a string path to a unix path format
    // returns the path in unix format
    string ToUnixPath(const string& path);
    wstring ToUnixPath(const wstring& path);

    // converts a string path to a dos path format
    // returns the path in dos format
    string ToDosPath(const string& path);
    wstring ToDosPath(const wstring& path);
    
    // converts a string to the platform's path format.
    // returns the path in the platform's path format.
    string ToPlatformPath(const string& path);
    wstring ToPlatformPath(const wstring& path);

    // char* pPath: in ASCII or UTF-8 encoding
    // wchar_t* pPath: in UTF-16 encoding
    // Non-ASCII components of pPath (everything from &pPath[0] to last non-ASCII
    // part, inclusively) should exist on disk, otherwise an empty string is returned.
    string GetAsciiPath(const char* pPath);
    string GetAsciiPath(const wchar_t* pPath);

    // pPath passed should be in ASCII or UTF-8 encoding
    string GetAbsoluteAsciiPath(const char* pPath);
    // pPath passed should be in UTF-16 encoding
    string GetAbsoluteAsciiPath(const wchar_t* pPath);

    // baseFolder and dependentPath passed should be in ASCII or UTF-8 encoding
    string GetShortestRelativeAsciiPath(const string& baseFolder, const string& dependentPath);

    string CanonicalizePath(const string& path);
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_PATHHELPERS_H
