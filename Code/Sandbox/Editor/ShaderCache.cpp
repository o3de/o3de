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

#include "EditorDefs.h"

#include "ShaderCache.h"

// Editor
#include "GameEngine.h"


//////////////////////////////////////////////////////////////////////////
bool CLevelShaderCache::Reload()
{
    return Load(m_filename.toUtf8().data());
}

//////////////////////////////////////////////////////////////////////////
bool CLevelShaderCache::Load(const char* filename)
{
    FILE* f = nullptr;
    azfopen(&f, filename, "rt");
    if (!f)
    {
        return false;
    }

    int nNumLines = 0;
    m_entries.clear();
    m_filename = filename;
    char str[65535];
    while (fgets(str, sizeof(str), f) != NULL)
    {
        if (str[0] == '<')
        {
            m_entries.insert(str);
            nNumLines++;
        }
    }
    fclose(f);
    if (nNumLines == m_entries.size())
    {
        m_bModified = false;
    }
    else
    {
        m_bModified = true;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLevelShaderCache::LoadBuffer(const QString& textBuffer, bool bClearOld)
{
    const char* separators = "\r\n,";

    int nNumLines = 0;
    if (bClearOld)
    {
        m_entries.clear();
    }
    m_filename = "";

    for (auto resToken : textBuffer.split(QRegularExpression(QStringLiteral("[%1]").arg(separators)), Qt::SkipEmptyParts))
    {
        if (!resToken.isEmpty() && resToken[0] == '<')
        {
            m_entries.insert(resToken);
            nNumLines++;
        }
    }
    if (nNumLines == m_entries.size() && !bClearOld)
    {
        m_bModified = false;
    }
    else
    {
        m_bModified = true;
    }

    int numShaders = m_entries.size();
    CLogFile::FormatLine("%d shader combination loaded for level %s", numShaders, GetIEditor()->GetGameEngine()->GetLevelPath().toUtf8().data());

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLevelShaderCache::Save()
{
    if (m_filename.isEmpty())
    {
        return false;
    }

    Update();

    FILE* f = nullptr;
    azfopen(&f, m_filename.toUtf8().data(), "wt");
    if (f)
    {
        for (Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            fputs(it->toLatin1().data(), f);
        }
        fclose(f);
    }
    m_bModified = false;
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLevelShaderCache::SaveBuffer(QString& textBuffer)
{
    Update();

    textBuffer.reserve(m_entries.size() * 1024);
    for (Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        textBuffer += (*it);
        textBuffer += "\n";
    }
    m_bModified = false;

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CLevelShaderCache::Update()
{
    IRenderer* pRenderer = gEnv->pRenderer;
    {
        QString buf;
        char* str = NULL;
        pRenderer->EF_Query(EFQ_GetShaderCombinations, str);
        if (str)
        {
            buf = str;
            pRenderer->EF_Query(EFQ_DeleteMemoryArrayPtr, str);
        }
        LoadBuffer(buf, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLevelShaderCache::Clear()
{
    m_entries.clear();
    m_bModified = true;
}

//////////////////////////////////////////////////////////////////////////
void CLevelShaderCache::ActivateShaders()
{
    bool bPreload = false;
    ICVar* pSysPreload = gEnv->pConsole->GetCVar("sys_preload");
    if (pSysPreload && pSysPreload->GetIVal() != 0)
    {
        bPreload = true;
    }

    if (bPreload)
    {
        QString textBuffer;
        textBuffer.reserve(m_entries.size() * 1024);
        for (Entries::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            textBuffer += (*it);
            textBuffer += "\n";
        }
        gEnv->pRenderer->EF_Query(EFQ_SetShaderCombinations, textBuffer);
    }
}
