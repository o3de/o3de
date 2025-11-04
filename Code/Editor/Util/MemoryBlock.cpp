/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "MemoryBlock.h"
#include <zlib.h>

#include <QMessageBox>
#include <QApplication>

//////////////////////////////////////////////////////////////////////////
CMemoryBlock::CMemoryBlock()
    : m_buffer(nullptr)
    , m_size(0)
    , m_uncompressedSize(0)
    , m_owns(false)
{
}

//////////////////////////////////////////////////////////////////////////
CMemoryBlock::CMemoryBlock(const CMemoryBlock& mem)
{
    *this = mem;
}

//////////////////////////////////////////////////////////////////////////
CMemoryBlock::~CMemoryBlock()
{
    Free();
}

//////////////////////////////////////////////////////////////////////////
CMemoryBlock& CMemoryBlock::operator=(const CMemoryBlock& mem)
{
    if (mem.GetSize() > 0)
    {
        // Do not reallocate.
        if (mem.GetSize() > GetSize())
        {
            if (!Allocate(mem.GetSize()))
            {
                return *this;
            }
        }
        Copy(mem.GetBuffer(), mem.GetSize());
    }
    else
    {
        m_buffer = nullptr;
        m_size = 0;
        m_owns = false;
    }
    m_uncompressedSize = mem.m_uncompressedSize;
    return *this;
}

//////////////////////////////////////////////////////////////////////////
bool CMemoryBlock::Allocate(int size, int uncompressedSize)
{
    assert(size > 0);
    if (m_buffer)
    {
        m_buffer = realloc(m_buffer, size);
    }
    else
    {
        m_buffer = malloc(size);
    }
    if (!m_buffer)
    {
        QString str;
        str = QStringLiteral("CMemoryBlock::Allocate failed to allocate %1Mb of Memory").arg(size / (1024 * 1024));
        CryLogAlways("%s", str.toUtf8().data());

        QMessageBox::critical(QApplication::activeWindow(), QString(), str + QString("\r\nSandbox will try to reduce its working memory set to free memory for this allocation."));
        GetIEditor()->ReduceMemory();
        if (m_buffer)
        {
            m_buffer = realloc(m_buffer, size);
        }
        else
        {
            m_buffer = malloc(size);
        }
        if (!m_buffer)
        {
            AZ_Warning("CMemoryBlock", false, "Reducing working memory set failed, Sandbox must quit");
        }
        else
        {
            AZ_Warning("CMemoryBlock", false, "Reducing working memory set succeeded\r\nSandbox may become unstable, it is advised to save the level and restart editor.");
        }
    }

    m_owns = true;
    m_size = size;
    m_uncompressedSize = uncompressedSize;
    // Check if allocation failed.
    if (m_buffer == nullptr)
    {
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Free()
{
    if (m_buffer && m_owns)
    {
        free(m_buffer);
    }
    m_buffer = nullptr;
    m_owns = false;
    m_size = 0;
    m_uncompressedSize = 0;
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Copy(void* src, int size)
{
    assert(size <= m_size);
    memcpy(m_buffer, src, size);
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Attach(void* buffer, int size, int uncompressedSize)
{
    Free();
    m_owns = false;
    m_buffer = buffer;
    m_size = size;
    m_uncompressedSize = uncompressedSize;
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Detach()
{
    Free();
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Compress(CMemoryBlock& toBlock) const
{
    // Cannot compress to itself.
    assert(this != &toBlock);
    unsigned long destSize = m_size * 2 + 128;
    CMemoryBlock temp;
    temp.Allocate(static_cast<int>(destSize));

    compress((unsigned char*)temp.GetBuffer(), &destSize, (unsigned char*)GetBuffer(), m_size);

    toBlock.Allocate(static_cast<int>(destSize));
    toBlock.Copy(temp.GetBuffer(), static_cast<int>(destSize));
    toBlock.m_uncompressedSize = GetSize();
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Uncompress(CMemoryBlock& toBlock) const
{
    assert(this != &toBlock);
    toBlock.Allocate(m_uncompressedSize);
    toBlock.m_uncompressedSize = 0;
    unsigned long destSize = m_uncompressedSize;
    [[maybe_unused]] int result = uncompress((unsigned char*)toBlock.GetBuffer(), &destSize, (unsigned char*)GetBuffer(), GetSize());
    assert(result == Z_OK);
    assert(destSize == static_cast<unsigned long>(m_uncompressedSize));
}

//////////////////////////////////////////////////////////////////////////
void CMemoryBlock::Serialize(CArchive& ar)
{
    if (ar.IsLoading())
    {
        int size;
        // Loading.
        ar >> size;
        if (size != m_size)
        {
            Allocate(size);
        }
        m_size = size;
        ar >> m_uncompressedSize;
        ar.Read(m_buffer, m_size);
    }
    else
    {
        // Saving.
        ar << m_size;
        ar << m_uncompressedSize;
        ar.Write(m_buffer, m_size);
    }
}
