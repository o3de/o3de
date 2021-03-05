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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CFGFILE_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CFGFILE_H
#pragma once

#include "ICfgFile.h"                       // ICfgFile
#include <list>

enum EConfigPriority : uint32_t;
class Config;

/** Configuration file class.
        Uses format similar to windows .ini files.
*/
class CfgFile
    : public ICfgFile
{
public:
    CfgFile();
    virtual ~CfgFile();

    //////////////////////////////////////////////////////////////////////////
    // interface ICfgFile
    virtual void Release();
    virtual bool Load(const string& fileName);
    virtual bool Save(void);
    virtual void UpdateOrCreateEntry(const char* inszSection, const char* inszKey, const char* inszValue);
    virtual void RemoveEntry(const char* inszSection, const char* inszKey);
    virtual void CopySectionKeysToConfig(EConfigPriority ePri, int sectionIndex, const char* keySuffixes, IConfigSink* config) const;
    virtual const char* GetSectionName(int sectionIndex) const;
    virtual int FindSection(const char* sectionName) const;
    //////////////////////////////////////////////////////////////////////////

private:
    void LoadFromBuffer(const char* buf);

private:
    // Config file entry structure, filled by readSection method.
    struct Entry
    {
        string key;     //!< keys (for comments this is "")
        string value;   //!< values and comments (with leading ; or //)

        bool IsComment() const;
    };

    struct Section
    {
        string           name;           //!< Section name. The first one has the name "" and is used if no section was specified.
        std::list<Entry> entries;        //!< List of entries.
    };

    string               m_fileName;     //!< Configuration file name.
    int                  m_modified;     //!< Set to true if config file been modified.
    std::vector<Section> m_sections;     //!< List of sections in config file. (the first one has the name "" and is used if no section was specified)
};


#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CFGFILE_H
