/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "PakFile.h"

// AzFramework
#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/IArchive.h>

// Editor
#include "Util/CryMemFile.h"


//////////////////////////////////////////////////////////////////////////
CPakFile::CPakFile()
    : m_pArchive(nullptr)
    , m_pCryPak(nullptr)
{
}

//////////////////////////////////////////////////////////////////////////
CPakFile::CPakFile(AZ::IO::IArchive* pCryPak)
    : m_pArchive(nullptr)
    , m_pCryPak(pCryPak)
{
}

//////////////////////////////////////////////////////////////////////////
CPakFile::~CPakFile()
{
    Close();
}

//////////////////////////////////////////////////////////////////////////
CPakFile::CPakFile(const char* filename)
{
    m_pArchive = nullptr;
    Open(filename);
}

//////////////////////////////////////////////////////////////////////////
void CPakFile::Close()
{
    m_pArchive = nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CPakFile::Open(const char* filename, bool bAbsolutePath)
{
    if (m_pArchive)
    {
        Close();
    }

    auto pCryPak = m_pCryPak ? m_pCryPak : GetIEditor()->GetSystem()->GetIPak();
    if (pCryPak == nullptr)
    {
        return false;
    }

    if (bAbsolutePath)
    {
        m_pArchive = pCryPak->OpenArchive(filename, {}, AZ::IO::INestedArchive::FLAGS_ABSOLUTE_PATHS);
    }
    else
    {
        m_pArchive = pCryPak->OpenArchive(filename);
    }
    if (m_pArchive)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPakFile::OpenForRead(const char* filename)
{
    if (m_pArchive)
    {
        Close();
    }
    auto pCryPak = m_pCryPak ? m_pCryPak : GetIEditor()->GetSystem()->GetIPak();
    if (pCryPak == nullptr)
    {
        return false;
    }
    m_pArchive = pCryPak->OpenArchive(filename, {}, AZ::IO::INestedArchive::FLAGS_OPTIMIZED_READ_ONLY | AZ::IO::INestedArchive::FLAGS_ABSOLUTE_PATHS);
    if (m_pArchive)
    {
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPakFile::UpdateFile(const char* filename, CCryMemFile& file, bool bCompress)
{
    if (m_pArchive)
    {
        int nSize = static_cast<int>(file.GetLength());

        UpdateFile(filename, file.GetMemPtr(), nSize, bCompress);
        file.Close();
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPakFile::UpdateFile(const char* filename, CMemoryBlock& mem, bool bCompress, int nCompressLevel)
{
    if (m_pArchive)
    {
        return UpdateFile(filename, mem.GetBuffer(), mem.GetSize(), bCompress, nCompressLevel);
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////
bool CPakFile::UpdateFile(const char* filename, void* pBuffer, int nSize, bool bCompress, int nCompressLevel)
{
    if (m_pArchive)
    {
        if (bCompress)
        {
            return 0 == m_pArchive->UpdateFile(filename, pBuffer, nSize, AZ::IO::INestedArchive::METHOD_DEFLATE, nCompressLevel);
        }
        else
        {
            return 0 == m_pArchive->UpdateFile(filename, pBuffer, nSize);
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPakFile::RemoveFile(const char* filename)
{
    if (m_pArchive)
    {
        return m_pArchive->RemoveFile(filename);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPakFile::RemoveDir(const char* directory)
{
    if (m_pArchive)
    {
        return m_pArchive->RemoveDir(directory);
    }
    return false;
}
