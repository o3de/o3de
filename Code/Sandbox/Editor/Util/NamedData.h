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

// Description : Collection of Named data blocks


#ifndef CRYINCLUDE_EDITOR_UTIL_NAMEDDATA_H
#define CRYINCLUDE_EDITOR_UTIL_NAMEDDATA_H
#pragma once
#include "MemoryBlock.h"
#include "QtUtil.h"

class CPakFile;

class CNamedData
{
public:
    CNamedData();
    virtual ~CNamedData();
    void AddDataBlock(const QString& blockName, void*   pData, int nSize, bool bCompress = true);
    void AddDataBlock(const QString& blockName, CMemoryBlock& block);
    //! Returns uncompressed block data.
    bool GetDataBlock(const QString& blockName, void*& pData, int& nSize);
    //! Returns raw data block in original form (Compressed or Uncompressed).
    CMemoryBlock* GetDataBlock(const QString& blockName, bool& bCompressed);

    void Clear();

public:
    virtual bool Serialize(CArchive& ar);

    //! Save named data to pak file.
    void Save(CPakFile& pakFile);
    //! Load named data from pak file.
    bool Load(const QString& levelPath, CPakFile& pakFile);

    void SaveToFiles(const QString& rootPath);
    void LoadFromFiles(const QString& rootPath);

private:
    struct DataBlock
    {
        DataBlock();
        QString blockName;
        CMemoryBlock data;
        CMemoryBlock compressedData;
        //! This block is compressed.
        bool bCompressed;
        bool bFastCompression;
    };
    typedef std::map<QString, DataBlock*, stl::less_stricmp<QString> > TBlocks;
    TBlocks m_blocks;
};
#endif // CRYINCLUDE_EDITOR_UTIL_NAMEDDATA_H
