/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Collection of Named data blocks.


#include "EditorDefs.h"

#include "NamedData.h"

// Editor
#include "Util/CryMemFile.h"    // for CryMemFile
#include "Util/PakFile.h"       // for CPakFile


//////////////////////////////////////////////////////////////////////////
CNamedData::CNamedData()
{
}

//////////////////////////////////////////////////////////////////////////
CNamedData::~CNamedData()
{
    Clear();
}

//////////////////////////////////////////////////////////////////////////
void CNamedData::AddDataBlock(const QString& blockName, void*   pData, int nSize, bool bCompress)
{
    assert(pData);
    assert(nSize > 0);

    DataBlock* pBlock = stl::find_in_map(m_blocks, blockName, (DataBlock*)nullptr);
    if (pBlock)
    {
        delete pBlock;
    }

    pBlock = new DataBlock();

    pBlock->bFastCompression = !bCompress;

    bCompress = false;

    if (bCompress)
    {
        pBlock->bCompressed = true;
        CMemoryBlock temp;
        temp.Attach(pData, nSize);
        temp.Compress(pBlock->compressedData);
    }
    else
    {
        pBlock->bCompressed = false;
        pBlock->data.Allocate(nSize);
        pBlock->data.Copy(pData, nSize);
    }
    m_blocks[blockName] = pBlock;
}

void CNamedData::AddDataBlock(const QString& blockName, CMemoryBlock& mem)
{
    DataBlock* pBlock = stl::find_in_map(m_blocks, blockName, (DataBlock*)nullptr);
    if (pBlock)
    {
        delete pBlock;
    }
    pBlock = new DataBlock();

    pBlock->bFastCompression = false;

    if (mem.GetUncompressedSize() != 0)
    {
        // This is compressed block.
        pBlock->bCompressed = true;
        pBlock->compressedData = mem;
    }
    else
    {
        pBlock->bCompressed = false;
        pBlock->data = mem;
    }
    m_blocks[blockName] = pBlock;
}

//////////////////////////////////////////////////////////////////////////
void CNamedData::Clear()
{
    for (TBlocks::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
    {
        delete it->second;
    }
    m_blocks.clear();
}

//////////////////////////////////////////////////////////////////////////
bool CNamedData::GetDataBlock(const QString& blockName, void*& pData, int& nSize)
{
    pData = nullptr;
    nSize = 0;

    bool bUncompressed = false;
    CMemoryBlock* mem = GetDataBlock(blockName, bUncompressed);
    if (mem)
    {
        pData = mem->GetBuffer();
        nSize = mem->GetSize();
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
CMemoryBlock* CNamedData::GetDataBlock(const QString& blockName, bool& bCompressed)
{
    DataBlock* pBlock = stl::find_in_map(m_blocks, blockName, (DataBlock*)nullptr);
    if (!pBlock)
    {
        return nullptr;
    }

    if (bCompressed)
    {
        // Return compressed data.
        if (!pBlock->compressedData.IsEmpty())
        {
            return &pBlock->compressedData;
        }
    }
    else
    {
        // Return normal data.
        if (!pBlock->data.IsEmpty())
        {
            return &pBlock->data;
        }
        else
        {
            // Uncompress compressed block.
            if (!pBlock->compressedData.IsEmpty())
            {
                pBlock->compressedData.Uncompress(pBlock->data);
                return &pBlock->data;
            }
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CNamedData::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        int iSize = static_cast<int>(m_blocks.size());
        ar << iSize;

        for (TBlocks::iterator it = m_blocks.begin(); it != m_blocks.end(); it++)
        {
            QString key = it->first;
            DataBlock* pBlock = it->second;

            unsigned int nOriginalSize;
            unsigned int nSizeFlags;
            unsigned int flags = 0;

            if (pBlock->bCompressed)
            {
                nOriginalSize = pBlock->compressedData.GetUncompressedSize();
                // Compressed data.
                unsigned int destSize = static_cast<unsigned int>(pBlock->compressedData.GetSize());
                void* dest = pBlock->compressedData.GetBuffer();
                nSizeFlags = destSize | (1u << 31);

                ar << key;
                ar << nSizeFlags; // Current size of data + 1 bit for compressed flag.
                ar << nOriginalSize;    // Size of uncompressed data.
                ar << flags;    // Some additional flags.
                ar.Write(dest, destSize);
            }
            else
            {
                nOriginalSize = pBlock->data.GetSize();
                void* dest = pBlock->data.GetBuffer();

                nSizeFlags = nOriginalSize;
                ar << key;
                ar << nSizeFlags;
                ar << nOriginalSize;    // Size of uncompressed data.
                ar << flags;    // Some additional flags.
                ar.Write(dest, nOriginalSize);
            }
        }
    }
    else
    {
        Clear();

        int iSize;
        ar >> iSize;

        for (int i = 0; i < iSize && ar.status() == QDataStream::Ok; i++)
        {
            QString key;
            unsigned int nSizeFlags = 0;
            unsigned int nSize = 0;
            unsigned int nOriginalSize = 0;
            unsigned int flags = 0;
            bool bCompressed = false;

            DataBlock* pBlock = new DataBlock();

            ar >> key;
            ar >> nSizeFlags;
            ar >> nOriginalSize;
            ar >> flags;

            nSize = nSizeFlags & (~(1 << 31));
            bCompressed = (nSizeFlags & (1 << 31)) != 0;

            if (nSize)
            {
                if (bCompressed)
                {
                    pBlock->compressedData.Allocate(nSize, nOriginalSize);
                    void* pSrcData = pBlock->compressedData.GetBuffer();
                    // Read compressed data.
                    ar.Read(pSrcData, nSize);
                }
                else
                {
                    pBlock->data.Allocate(nSize);
                    void* pSrcData = pBlock->data.GetBuffer();

                    // Read uncompressed data.
                    ar.Read(pSrcData, nSize);
                }
            }
            m_blocks[key] = pBlock;
        }
    }

    return ar.status() == QDataStream::Ok;
}

//////////////////////////////////////////////////////////////////////////
void CNamedData::Save(CPakFile& pakFile)
{
    for (TBlocks::iterator it = m_blocks.begin(); it != m_blocks.end(); it++)
    {
        QString key = it->first;
        DataBlock* pBlock = it->second;
        if (!pBlock->bCompressed)
        {
            QString filename = key + ".editor_data";
            pakFile.UpdateFile(filename.toUtf8().data(), pBlock->data, true, pBlock->bFastCompression ? AZ::IO::INestedArchive::LEVEL_FASTEST : AZ::IO::INestedArchive::LEVEL_BETTER);
        }
        else
        {
            int nOriginalSize = pBlock->compressedData.GetUncompressedSize();
            CCryMemFile memFile;
            // Write uncompressed data size.
            memFile.Write(&nOriginalSize, sizeof(nOriginalSize));
            // Write compressed data.
            memFile.Write(pBlock->compressedData.GetBuffer(), pBlock->compressedData.GetSize());
            pakFile.UpdateFile((key + ".editor_datac").toUtf8().data(), memFile, false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CNamedData::Load(const QString& levelPath, [[maybe_unused]] CPakFile& pakFile)
{
    int i;
    IFileUtil::FileArray files;
    CFileUtil::ScanDirectory(levelPath, "*.editor_data", files, false);
    for (i = 0; i < files.size(); i++)
    {
        QString filename = files[i].filename;
        CCryFile cfile;
        if (cfile.Open(Path::Make(levelPath, filename).toUtf8().data(), "rb"))
        {
            int fileSize = static_cast<int>(cfile.GetLength());
            if (fileSize > 0)
            {
                QString key = Path::GetFileName(filename);
                // Read data block.
                DataBlock* pBlock = new DataBlock();
                pBlock->data.Allocate(fileSize);
                cfile.ReadRaw(pBlock->data.GetBuffer(), fileSize);
                m_blocks[key] = pBlock;
            }
        }
    }
    files.clear();
    // Scan compressed data.
    CFileUtil::ScanDirectory(levelPath, "*.editor_datac", files, false);
    for (i = 0; i < files.size(); i++)
    {
        QString filename = files[i].filename;
        CCryFile cfile;
        if (cfile.Open(Path::Make(levelPath, filename).toUtf8().data(), "rb"))
        {
            int fileSize = static_cast<uint32>(cfile.GetLength());
            if (fileSize > 0)
            {
                // Read uncompressed data size.
                int nOriginalSize = 0;
                cfile.ReadType(&nOriginalSize);
                // Read uncompressed data.
                int nDataSize = fileSize - sizeof(nOriginalSize);

                QString key = Path::GetFileName(filename);
                // Read data block.
                DataBlock* pBlock = new DataBlock();
                pBlock->compressedData.Allocate(nDataSize, nOriginalSize);
                cfile.ReadRaw(pBlock->compressedData.GetBuffer(), nDataSize);
                m_blocks[key] = pBlock;
            }
        }
    }

    return true;
}

void CNamedData::SaveToFiles(const QString& rootPath)
{
    for (TBlocks::iterator it = m_blocks.begin(); it != m_blocks.end(); it++)
    {
        QString key = it->first;
        DataBlock* pBlock = it->second;
        QString filename = rootPath + key;

        if (pBlock->bCompressed)
        {
            filename += ".editor_datac";

            CCryFile file(filename.toUtf8().data(), "wb");

            int nOriginalSize = pBlock->compressedData.GetUncompressedSize();
            file.Write(&nOriginalSize, sizeof(nOriginalSize));
            file.Write(pBlock->compressedData.GetBuffer(), pBlock->compressedData.GetSize());
        }
        else
        {
            filename += ".editor_data";
            CCryFile file(filename.toUtf8().data(), "wb");
            file.Write(pBlock->data.GetBuffer(), pBlock->data.GetSize());
        }
    }
}

void CNamedData::LoadFromFiles(const QString& rootPath)
{
    // TODO: Unify code paths in a nicer way!
    CPakFile dummyPak;
    Load(rootPath, dummyPak);
}

CNamedData::DataBlock::DataBlock()
{
}
