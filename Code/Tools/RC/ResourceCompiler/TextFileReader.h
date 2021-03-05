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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_TEXTFILEREADER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_TEXTFILEREADER_H
#pragma once

struct IPakSystem;

// Summary:
//  Utility class to read text files and split lines inplace. One allocation per one file.
class TextFileReader
{
public:
    bool Load(const char* filename, std::vector<char*>& lines);
    bool LoadFromPak(IPakSystem* system, const char* filename, std::vector<char*>& lines);

private:
    // fix line endings to zeros, so we can use strings from the same buffer
    void PrepareLines(std::vector<char*>& lines);

    std::vector<char> m_buffer;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_TEXTFILEREADER_H
