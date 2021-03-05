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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_LISTFILE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_LISTFILE_H
#pragma once

class CListFile
{
public:
    CListFile(IResourceCompiler* pRC);

    bool Process(
        const string& listFile,
        const string& formatList,
        const string& wildcardList,
        const string& defaultFolder,
        std::vector< std::pair<string, string> >& outFiles);

private:
    void ParseListFileInZip(
        const string& zipFilename,
        const string& listFilename,
        const std::vector<string>& formats,
        const std::vector<string>& wildcards,
        const string& defaultFolder,
        std::vector< std::pair<string, string> >& outFiles);

    bool ProcessLine(
        const string& line,
        const std::vector<string>& formats,
        const std::vector<string>& wildcards,
        const string& defaultFolder,
        std::vector< std::pair<string, string> >& outFiles);

    bool ReadLines(
        const string& listFile,
        std::vector<string>& lines);

private:
    IResourceCompiler* m_pRC;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_LISTFILE_H
