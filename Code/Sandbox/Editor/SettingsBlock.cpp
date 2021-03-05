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

#include "SettingsBlock.h"

// CryCommon
#include <CryCommon/Serialization/ITextInputArchive.h>
#include <CryCommon/Serialization/ITextOutputArchive.h>

// Editor
#include "Serialization.h"

using std::vector;

SProjectSettingsBlock* SProjectSettingsBlock::s_pLastBlock;

SProjectSettingsBlock::SProjectSettingsBlock(const char* name, const char* label)
    : m_name(name)
    , m_label(label)
{
    m_pPrevious = s_pLastBlock;
    s_pLastBlock = this;
}


struct SAllSettingsSerializer
{
    void Serialize(Serialization::IArchive& ar)
    {
        SProjectSettingsBlock* pCurrent = SProjectSettingsBlock::s_pLastBlock;
        while (pCurrent != 0)
        {
            ar(*pCurrent, pCurrent->GetName(), pCurrent->GetLabel());
            pCurrent = pCurrent->m_pPrevious;
        }
    }
} static gAllSettingsSerializer;

void SProjectSettingsBlock::GetAllSettingsSerializer(Serialization::SStruct* pSerializer)
{
    *pSerializer = Serialization::SStruct(gAllSettingsSerializer);
}

SProjectSettingsBlock* SProjectSettingsBlock::Find(const char* blockName)
{
    SProjectSettingsBlock* pCurrent = SProjectSettingsBlock::s_pLastBlock;
    while (pCurrent != 0)
    {
        if (_stricmp(pCurrent->GetName(), blockName) == 0)
        {
            return pCurrent;
        }
    }
    return 0;
}

static bool ReadFileContent(vector<char>* pBuffer, const char* filename)
{
    AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, "rb");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    size_t size = gEnv->pCryPak->FGetSize(fileHandle);
    pBuffer->resize(size);

    bool result = true;
    if (gEnv->pCryPak->FRead(&(*pBuffer)[0], size, fileHandle) != size)
    {
        result = false;
    }
    gEnv->pCryPak->FClose(fileHandle);
    return result;
}

static bool SaveFileContent(const char* filename, const char* pBuffer, size_t length)
{
    string fullpath = Path::GamePathToFullPath(filename).toUtf8().data();

    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    if (!gEnv->pFileIO->Open(fullpath.c_str(), AZ::IO::GetOpenModeFromStringMode("wb"), fileHandle))
    {
        return false;
    }

    bool result = true;
    if (!gEnv->pFileIO->Write(fileHandle, pBuffer, length))
    {
        result = false;
    }

    gEnv->pFileIO->Close(fileHandle);
    return result;
}

static bool SaveFileContentIfDiffers(const char* filename, const char* pBuffer, size_t length)
{
    vector<char> content;
    ReadFileContent(&content, filename);

    bool needToWrite = true;
    if (!content.empty() && content.size() == length)
    {
        needToWrite = memcmp(&content[0], pBuffer, length) != 0;
    }

    if (needToWrite)
    {
        return SaveFileContent(filename, pBuffer, length);
    }
    else
    {
        return true;
    }
}

bool SProjectSettingsBlock::Load()
{
    const char* filename = GetFilename();

    vector<char> content;
    if (!ReadFileContent(&content, filename))
    {
        return false;
    }

    auto pArchive(Serialization::CreateTextInputArchive());
    if (!pArchive)
    {
        return false;
    }

    if (!pArchive->AttachMemory(&content[0], content.size()))
    {
        return false;
    }

    Serialization::SStruct serializer;
    GetAllSettingsSerializer(&serializer);
    serializer(*pArchive);
    return true;
}

bool SProjectSettingsBlock::Save()
{
    const char* filename = GetFilename();
    auto pArchive(Serialization::CreateTextOutputArchive());
    if (!pArchive)
    {
        return false;
    }

    Serialization::SStruct serializer;
    GetAllSettingsSerializer(&serializer);
    serializer(*pArchive);

    return SaveFileContentIfDiffers(filename, pArchive->GetBuffer(), pArchive->GetBufferLength());
}

const char* SProjectSettingsBlock::GetFilename()
{
    return "SandboxSettings.json";
}

