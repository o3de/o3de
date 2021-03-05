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
#include "CfgFile.h"
#include "Config.h"
#include "DebugLog.h"
#include "IRCLog.h"

//#define  Log DebugLog
#define  Log while (false)


CfgFile::CfgFile()
{
    // Create empty section.
    Section section;
    section.name = "";
    m_sections.push_back(section);

    m_modified = false;
}

CfgFile::~CfgFile()
{
}

void CfgFile::Release()
{
    delete this;
}

// Load configuration file.
bool CfgFile::Load(const string& fileName)
{
    m_fileName = fileName;
    m_modified = false;

    FILE* file = nullptr;
    azfopen(&file, fileName.c_str(), "rb");
    if (!file)
    {
        RCLog("Can't open \"%s\"", fileName.c_str());
        return false;
    }

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read whole file to memory.
    char* s = (char*)malloc(size + 1);
    memset(s, 0, size + 1);
    fread(s, 1, size, file);
    LoadFromBuffer(s);
    free(s);

    fclose(file);

    return true;
}

// Save configuration file, with the stored name in m_fileName
bool CfgFile::Save()
{
    FILE* file = nullptr;
    azfopen(&file, m_fileName.c_str(), "wb");

    if (!file)
    {
        return(false);
    }

    // Loop on sections.
    for (std::vector<Section>::iterator si = m_sections.begin(); si != m_sections.end(); si++)
    {
        Section& sec = *si;

        if (sec.name != "")
        {
            fprintf(file, "[%s]\r\n", sec.name.c_str());                                                                                      // section
        }
        for (std::list<Entry>::iterator it = sec.entries.begin(); it != sec.entries.end(); ++it)
        {
            if ((*it).key == "")
            {
                fprintf(file, "%s\r\n", (*it).value.c_str());                                                         // comment
            }
            else
            {
                fprintf(file, "%s=%s\r\n", (*it).key.c_str(), (*it).value.c_str());        // key=value
            }
        }
    }

    fclose(file);
    return(true);
}

void CfgFile::UpdateOrCreateEntry(const char* inszSection, const char* inszKey, const char* inszValue)
{
    const int sectionIndex = FindSection(inszSection);
    Section* const sec = &m_sections[(sectionIndex < 0) ? 0 : sectionIndex];

    for (std::list<Entry>::iterator it = sec->entries.begin(); it != sec->entries.end(); ++it)
    {
        if (azstricmp(it->key.c_str(), inszKey) == 0)              // Key found
        {
            if (!it->IsComment())                                    // update key
            {
                if (it->value == inszValue)
                {
                    return;
                }

                it->value = inszValue;
                m_modified = true;
                return;
            }
        }
    }

    // Create new key
    Entry entry;
    entry.key = inszKey;
    entry.value = inszValue;

    sec->entries.push_back(entry);
    m_modified = true;
}

void CfgFile::RemoveEntry(const char* inszSection, const char* inszKey)
{
    const int sectionIndex = FindSection(inszSection);
    Section* const sec = &m_sections[(sectionIndex < 0) ? 0 : sectionIndex];

    for (std::list<Entry>::iterator it = sec->entries.begin(); it != sec->entries.end(); ++it)
    {
        if (azstricmp(it->key.c_str(), inszKey) == 0)
        {
            if (!it->IsComment())
            {
                sec->entries.erase(it);
                return;
            }
        }
    }
}

void CfgFile::LoadFromBuffer(const char* buf)
{
    // Read entries from config string buffer.
    Section* curr_section = &m_sections.front();    // Empty section.

    while (*buf != '\0')
    {
        size_t count = 0;
        // Find the first line terminator
        while (buf[count] != '\0' && buf[count] != '\r' && buf[count] != '\n')
        {
            count++;
        }
        
        Entry entry;
        entry.value = string(buf, count);

        // Move buffer forward and skip any trailing line endings
        buf += count;
        while (*buf != '\0' && (*buf == '\r' || *buf == '\n'))
        {
            buf++;
        }

        Log("Parsing line: \"%s\"", entry.value.c_str());

        bool isComment = entry.IsComment();
        if (!isComment)
        {
            entry.value.Trim();
        }

        if (isComment || entry.value.empty())
        {
            Log(isComment ? "It's a comment" : "It's empty");
            // Add this comment to current section.
            curr_section->entries.push_back(entry);
            continue;
        }

        // First check if the line is a section to avoid equal signs in the section name to cause incorrect parsing
        if (entry.value[0] == '[' && entry.value[entry.value.size() - 1] == ']')
        {
            Section section;
            section.name = entry.value.Mid(1, entry.value.size() - 2); // Remove braces.
            Log("Section! name: %s", section.name.c_str());
            m_sections.push_back(section);
            
            // Set current section.
            curr_section = &m_sections.back();
        }
        else
        {
            size_t splitter = entry.value.find('=');
            if (splitter != string::npos)
            {
                Log("found splitter at %d", splitter);

                entry.key = entry.value.Mid(0, splitter);  // Before splitter is key name.
                entry.value = entry.value.Mid(splitter + 1);   // Everything after splitter is value string.
                entry.key.Trim();
                entry.value.Trim();

                Log("Key: %s, Value: %s", entry.key.c_str(), entry.value.c_str());

                // Add this entry to current section.
                curr_section->entries.push_back(entry);
            }
            else
            {
                // Nameless value
                curr_section->entries.push_back(entry);
            }
        }
    }
}

void CfgFile::CopySectionKeysToConfig(const EConfigPriority ePri, int sectionIndex, const char* keySuffixes, IConfigSink* config) const
{
    if (sectionIndex < 0 || sectionIndex >= m_sections.size())
    {
        return;
    }

    std::vector<string> suffixes;
    if (keySuffixes)
    {
        StringHelpers::SplitByAnyOf(keySuffixes, ", ", false, suffixes);
    }

    const Section* const sec = &m_sections[sectionIndex];

    for (std::list<Entry>::const_iterator it = sec->entries.begin(); it != sec->entries.end(); ++it)
    {
        const Entry& e = (*it);

        if (e.IsComment())
        {
            continue;
        }

        const size_t delimiterPos = e.key.find(':');
        if (delimiterPos == string::npos)
        {
            // The key has no suffix. Add key & value without any checks.
            config->SetKeyValue(ePri, e.key.c_str(), e.value.c_str());
        }
        else
        {
            // The key has a suffix.
            if (keySuffixes == 0)
            {
                // keySuffixes == 0 means that we must copy all keys, does not matter if they have
                // suffix or not. Suffix (if exists) is preserved as part of the name of the key.
                config->SetKeyValue(ePri, e.key.c_str(), e.value.c_str());
            }
            else
            {
                for (size_t i = 0; i < suffixes.size(); ++i)
                {
                    if (azstricmp(suffixes[i].c_str(), e.key.c_str() + delimiterPos + 1) == 0)
                    {
                        // The key's suffix is same as a suffix in keySuffixes. We add key (without suffix!) and value.
                        config->SetKeyValue(ePri, e.key.substr(0, delimiterPos).c_str(), e.value.c_str());
                        break;
                    }
                }
            }
        }
    }
}

const char* CfgFile::GetSectionName(int sectionIndex) const
{
    if ((sectionIndex < 0) || ((size_t)sectionIndex >= m_sections.size()))
    {
        return 0;
    }

    return(m_sections[sectionIndex].name.c_str());
}

int CfgFile::FindSection(const char* sectionName) const
{
    for (size_t i = 0, count = m_sections.size(); i < count; ++i)
    {
        if (azstricmp(m_sections[i].name.c_str(), sectionName) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

bool CfgFile::Entry::IsComment() const
{
    const char* pBegin = value.c_str();
    while (pBegin[0] == ' ' || pBegin[0] == '\t')
    {
        ++pBegin;
    }

    // "//" comment
    if (pBegin[0] == '/' && pBegin[1] == '/')
    {
        return true;
    }

    // ";" comment
    if (pBegin[0] == ';')
    {
        return true;
    }

    // empty line (treat it as comment)
    if (pBegin[0] == 0)
    {
        return true;
    }

    return false;
}
