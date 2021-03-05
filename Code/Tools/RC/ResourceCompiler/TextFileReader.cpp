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

#include "TextFileReader.h"
#include "IPakSystem.h"

bool TextFileReader::Load(const char* filename, std::vector<char*>& lines)
{
    FILE* file = nullptr; 
    azfopen(&file, filename, "rb");
    if (!file)
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    const size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    m_buffer.resize(size + 1); // +1 for last terminating zero
    if (fread(&m_buffer[0], 1, size, file) != size)
    {
        fclose(file);
        return false;
    }
    m_buffer[size] = '\0';

    fclose(file);

    PrepareLines(lines);
    return true;
}

bool TextFileReader::LoadFromPak(IPakSystem* system, const char* filename, std::vector<char*>& lines)
{
    PakSystemFile* const file = system->Open(filename, "rb");
    if (!file)
    {
        return false;
    }

    const size_t size = system->GetLength(file);
    m_buffer.resize(size + 1); // +1 for last terminating zero

    if (system->Read(file, &m_buffer[0], size) != size)
    {
        system->Close(file);
        return false;
    }
    m_buffer[size] = '\0';

    system->Close(file);

    PrepareLines(lines);
    return true;
}

// fix line endings to zeros, so we can use strings from the same buffer
void TextFileReader::PrepareLines(std::vector<char*>& lines)
{
    lines.clear();

    char* p = &m_buffer[0];
    const char* const end = p + m_buffer.size();
    char* line = p;
    while (p != end)
    {
        if (*p == '\r' || *p == '\n')
        {
            *p = '\0';
            if (*line)
            {
                lines.push_back(line);
            }
            ++p;
            line = p;
        }
        else
        {
            ++p;
        }
    }
    if (*line)
    {
        lines.push_back(line);
    }
}

