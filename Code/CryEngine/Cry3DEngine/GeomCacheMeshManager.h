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

// Description : Manages static meshes for geometry caches


#ifndef CRYINCLUDE_CRY3DENGINE_GEOMCACHEMESHMANAGER_H
#define CRYINCLUDE_CRY3DENGINE_GEOMCACHEMESHMANAGER_H
#pragma once

#if defined(USE_GEOM_CACHES)

#include "IRenderMesh.h"
#include "GeomCacheFileFormat.h"
#include "GeomCache.h"
#include <StlUtils.h>

class CGeomCacheMeshManager
{
public:
    void Reset();

    bool ReadMeshStaticData(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, SGeomCacheStaticMeshData& staticMeshData) const;
    _smart_ptr<IRenderMesh> ConstructStaticRenderMesh(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo,
        SGeomCacheStaticMeshData& staticMeshData, const char* pFileName);
    _smart_ptr<IRenderMesh> GetStaticRenderMesh(const uint64 hash) const;
    void RemoveReference(SGeomCacheStaticMeshData& staticMeshData);
private:
    bool ReadMeshIndices(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo,
        SGeomCacheStaticMeshData& staticMeshData, std::vector<vtx_idx>& indices) const;
    bool ReadMeshPositions(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<Vec3> positions) const;
    bool ReadMeshTexcoords(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<Vec2> texcoords) const;
    bool ReadMeshQTangents(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<SPipTangents> tangents) const;
    bool ReadMeshColors(CGeomCacheStreamReader& reader, const GeomCacheFile::SMeshInfo& meshInfo, strided_pointer<UCol> colors) const;

    struct SMeshMapInfo
    {
        _smart_ptr<IRenderMesh> m_pRenderMesh;
        uint m_refCount;
    };

    // Map from mesh hash to render mesh
    typedef AZStd::unordered_map<uint64, SMeshMapInfo> TMeshMap;
    TMeshMap m_meshMap;
};

#endif
#endif // CRYINCLUDE_CRY3DENGINE_GEOMCACHEMESHMANAGER_H
