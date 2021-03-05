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

#ifndef CRYINCLUDE_EDITOR_SHADERCACHE_H
#define CRYINCLUDE_EDITOR_SHADERCACHE_H
#pragma once

//////////////////////////////////////////////////////////////////////////
class CLevelShaderCache
{
public:
    CLevelShaderCache()
    {
        m_bModified = false;
    }
    bool Load(const char* filename);
    bool LoadBuffer(const QString& textBuffer, bool bClearOld = true);
    bool SaveBuffer(QString& textBuffer);
    bool Save();
    bool Reload();
    void Clear();

    void Update();
    void ActivateShaders();

private:
    //////////////////////////////////////////////////////////////////////////
    bool m_bModified;
    QString m_filename;
    typedef std::set<QString> Entries;
    Entries m_entries;
};


#endif // CRYINCLUDE_EDITOR_SHADERCACHE_H
