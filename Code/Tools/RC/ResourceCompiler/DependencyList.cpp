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

#include "ResourceCompiler_precompiled.h"

#include "DependencyList.h"

#include "IRCLog.h"
#include "PathHelpers.h"
#include "StringHelpers.h"

#include <stdio.h>      // FILE


CDependencyList::CDependencyList()
    : m_bDuplicatesWereRemoved(true)
{
}

CDependencyList::CDependencyList(const CDependencyList& obj)
{
    m_files = obj.m_files;
    m_bDuplicatesWereRemoved = obj.m_bDuplicatesWereRemoved;
}

string CDependencyList::NormalizeFilename(const char* filename)
{
    return PathHelpers::ToDosPath(PathHelpers::GetAbsoluteAsciiPath(string(filename)));
}

void CDependencyList::Add(const char* sInputFilename, const char* sOutputFilename)
{
    SFile f;
    f.inputFile = sInputFilename[0] ? NormalizeFilename(sInputFilename) : string();
    f.outputFile = sOutputFilename[0] ? NormalizeFilename(sOutputFilename) : string();
    m_files.push_back(f);
    m_bDuplicatesWereRemoved = false;
}

void CDependencyList::RemoveDuplicates()
{
    if (m_bDuplicatesWereRemoved)
    {
        return;
    }

    m_bDuplicatesWereRemoved = true;

    if (m_files.size() <= 1)
    {
        return;
    }

    struct CompareLess
    {
        bool operator()(const SFile& left, const SFile& right) const
        {
            const int res = StringHelpers::Compare(left.inputFile, right.inputFile);
            if (res != 0)
            {
                return res < 0;
            }
            return (StringHelpers::Compare(left.outputFile, right.outputFile) < 0);
        }
    };

    struct CompareEqual
    {
        bool operator()(const SFile& left, const SFile& right) const
        {
            return
                StringHelpers::Equals(left.inputFile, right.inputFile) &&
                StringHelpers::Equals(left.outputFile, right.outputFile);
        }
    };

    std::sort(m_files.begin(), m_files.end(), CompareLess());

    std::vector<SFile>::iterator end = std::unique(m_files.begin(), m_files.end(), CompareEqual());
    if (end != m_files.end())
    {
        m_files.erase(end, m_files.end());
    }
}

void CDependencyList::RemoveInputFiles(const std::vector<string>& inputFilesToRemove)
{
    if (m_files.empty())
    {
        return;
    }

    struct CompareLess
    {
        bool operator()(const SFile& left, const SFile& right) const
        {
            return (left.inputFile < right.inputFile);
        }
    };

    std::sort(m_files.begin(), m_files.end(), CompareLess());

    bool bDeleted = false;

    const size_t count = inputFilesToRemove.size();
    for (size_t i = 0; i < count; ++i)
    {
        SFile searchFile;
        searchFile.inputFile = NormalizeFilename(inputFilesToRemove[i].c_str());

        std::vector<SFile>::iterator it = std::lower_bound(m_files.begin(), m_files.end(), searchFile, CompareLess());

        while (it != m_files.end())
        {
            if (!StringHelpers::Equals(it->inputFile, searchFile.inputFile))
            {
                break;
            }
            // Mark the entry for deleting
            it->outputFile.clear();
            ++it;
            bDeleted = true;
        }
    }

    // Delete marked entries
    if (bDeleted)
    {
        struct MatchEmptyOutputFile
        {
            bool operator()(const SFile& file) const
            {
                return file.outputFile.empty();
            }
        };

        m_files.erase(std::remove_if(m_files.begin(), m_files.end(), MatchEmptyOutputFile()), m_files.end());
    }
}

void CDependencyList::Save(const char* filename) const
{
    FILE* file = nullptr; 
    azfopen(&file, filename, "wt");

    if (!file)
    {
        RCLogError("Cannot write filelist '%s'", filename);
        return;
    }

    for (size_t i = 0; i < m_files.size(); ++i)
    {
        fprintf(file, "%s=%s\n", m_files[i].inputFile.c_str(), m_files[i].outputFile.c_str());
    }
    fclose(file);
}

void CDependencyList::SaveOutputOnly(const char* filename) const
{
    FILE* file = nullptr; 
    azfopen(&file, filename, "wt");

    if (!file)
    {
        RCLogError("Cannot write filelist (output files only) '%s'", filename);
        return;
    }

    for (size_t i = 0; i < m_files.size(); ++i)
    {
        fprintf(file, "%s\n", m_files[i].outputFile.c_str());
    }
    fclose(file);
}

void CDependencyList::Load(const char* filename)
{
    FILE* file = nullptr; 
    azfopen(&file, filename, "rt");

    if (!file)
    {
        RCLogError("Cannot read filelist '%s' (probably the file does not exist)", filename);
        return;
    }

    char linebuf[MAX_PATH * 4];
    while (!feof(file))
    {
        char* const line = fgets(linebuf, sizeof(linebuf), file);
        if (line && line[0])
        {
            char* const pos = strchr(line, '=');
            if (pos)
            {
                *pos = 0;
                const string sIn = string(line).Trim();
                const string sOut = string(pos + 1).Trim();
                if (!sIn.empty() && !sOut.empty())
                {
                    Add(sIn.c_str(), sOut.c_str());
                }
            }
        }
    }
    fclose(file);
}
