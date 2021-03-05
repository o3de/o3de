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

// Description : Export geometry OCM file format


#ifndef CRYINCLUDE_EDITOR_EXPORT_OCMEXPORTER_H
#define CRYINCLUDE_EDITOR_EXPORT_OCMEXPORTER_H
#pragma once

#include <IExportManager.h>

class CFileEndianWriter;

struct SOCMeshInfo
{
    Matrix44    m_OBBMat;
    uint32      m_Offset;
    size_t      m_MeshHash;
    bool    operator==(const SOCMeshInfo& rOther) const{return m_MeshHash == rOther.m_MeshHash; }
};
typedef std::vector<SOCMeshInfo>    tdMeshOffset;

class COCMExporter
    : public IExporter
{
public:
    virtual const char* GetExtension() const;
    virtual const char* GetShortDescription() const;
    virtual bool ExportToFile(const char* filename, const Export::IData* pExportData);
    virtual bool ImportFromFile([[maybe_unused]] const char* filename, [[maybe_unused]] Export::IData* pData){return false; };
    virtual void Release();

private:
    const char* TrimFloat(float fValue) const;
    void SaveInstance(CFileEndianWriter& rWriter, const Export::Object* pInstance, const SOCMeshInfo& rMeshInfo);
    size_t SaveMesh(CFileEndianWriter& rWriter, const Export::Object* pMesh, Matrix44& rOBBMat);

    void Extends(const Matrix44& rTransform, const Export::Object* pMesh, f32& rMinX, f32& rMaxX, f32& rMinY, f32& rMaxY, f32& rMinZ, f32& rMaxZ)  const;
    Matrix44 CalcOBB(const Export::Object* pMesh);
};


#endif // CRYINCLUDE_EDITOR_EXPORT_OCMEXPORTER_H
