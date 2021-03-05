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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_DEPENDENCYLIST_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_DEPENDENCYLIST_H
#pragma once

//////////////////////////////////////////////////////////////////////////
class CDependencyList
{
public:
    struct SFile
    {
        string inputFile;
        string outputFile;
    };

private:
    std::vector<SFile> m_files;
    bool m_bDuplicatesWereRemoved;

public:
    CDependencyList();

    CDependencyList(const CDependencyList& obj);

    static string NormalizeFilename(const char* filename);

    size_t GetCount() const
    {
        return m_files.size();
    }

    const SFile& GetElement(size_t index) const
    {
        return m_files[index];
    }

    void Add(const char* inputFilename, const char* outputFilename);

    void RemoveDuplicates();

    void RemoveInputFiles(const std::vector<string>& inputFilesToRemove);

    void Save(const char* filename) const;

    void SaveOutputOnly(const char* filename) const;

    void Load(const char* filename);
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_DEPENDENCYLIST_H
