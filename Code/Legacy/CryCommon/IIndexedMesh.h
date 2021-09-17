/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "Cry_Color.h"
#include <VertexFormats.h>
#include <Vertex.h>

// Description:
//    2D Texture coordinates used by CMesh.
struct SMeshTexCoord
{
    SMeshTexCoord() {}

private:
    float s, t;

public:
    bool IsEquivalent(const SMeshTexCoord& other, float epsilon = 0.00005f) const
    {
        return
            (fabs_tpl(s - other.s) <= epsilon) &&
            (fabs_tpl(t - other.t) <= epsilon);
    }
    ILINE Vec2 GetUV() const
    {
        return Vec2(s, t);
    }
};

// Description:
//    RGBA Color description structure used by CMesh.
struct SMeshColor
{
    SMeshColor() {}

private:
    uint8 r, g, b, a;

public:
    explicit SMeshColor(uint8 otherr, uint8 otherg, uint8 otherb, uint8 othera)
    {
        r = otherr;
        g = otherg;
        b = otherb;
        a = othera;
    }
};


// Description:
//    Defines a single triangle face in the CMesh topology.
struct SMeshFace
{
    int v[3]; // indices to vertex, normals and optionally tangent basis arrays
    unsigned char nSubset; // index to mesh subsets array.
};

// Description:
//    3D Normal Vector used by CMesh.
struct SMeshNormal
{
    SMeshNormal() {}

private:
    Vec3 Normal;

public:
    explicit SMeshNormal(const Vec3& othern)
    {
        Normal = othern;
    }

    Vec3 GetN() const { return Normal; }

};

// Subset of mesh is a continuous range of vertices and indices that share same material.
struct SMeshSubset
{
    Vec3 vCenter;
    float fRadius;
    float fTexelDensity;

    int nFirstIndexId;
    int nNumIndices;

    int nFirstVertId;
    int nNumVerts;

    int nMatID; // Material Sub-object id.
    int nMatFlags; // Special Material flags.
    int nPhysicalizeType; // Type of physicalization for this subset.

    AZ::Vertex::Format vertexFormat;

    SMeshSubset()
        : vCenter(0, 0, 0)
        , fRadius(0)
        , fTexelDensity(0)
        , nFirstIndexId(0)
        , nNumIndices(0)
        , nFirstVertId(0)
        , nNumVerts(0)
        , nMatID(0)
        , nMatFlags(0)
        , nPhysicalizeType(0x1000)
        , vertexFormat(eVF_P3S_C4B_T2S)
    {
    }

};

// Description:
//    Editable mesh interface.
//    IndexedMesh can be created directly or loaded from CGF file, before rendering it is converted into IRenderMesh.
//    IStatObj is used to host IIndexedMesh, and corresponding IRenderMesh.
struct IIndexedMesh
{
    /*! Structure used for read-only access to mesh data. Used by GetMesh() function */
    struct SMeshDescription
    {
        const SMeshFace*     m_pFaces;    // pointer to array of faces
        const Vec3*          m_pVerts;    // pointer to array of vertices in f32 format
        const Vec3f16*       m_pVertsF16; // pointer to array of vertices in f16 format
        const SMeshNormal*   m_pNorms;    // pointer to array of normals
        const SMeshColor*    m_pColor;    // pointer to array of vertex colors
        const SMeshTexCoord* m_pTexCoord; // pointer to array of texture coordinates
        const vtx_idx*   m_pIndices;  // pointer to array of indices
        int m_nFaceCount;  // number of elements m_pFaces array
        int m_nVertCount;  // number of elements in m_pVerts, m_pNorms and m_pColor arrays
        int m_nCoorCount;  // number of elements in m_pTexCoord array
        int m_nIndexCount; // number of elements in m_pIndices array
    };

    virtual ~IIndexedMesh() {}

    // Release indexed mesh.
    virtual void Release() = 0;

    //! Gives read-only access to mesh data
    virtual void GetMeshDescription(SMeshDescription& meshDesc) const = 0;

    //! Return number of allocated faces
    virtual int GetFaceCount() const = 0;

    //! Return number of allocated vertices, normals and colors
    virtual int GetVertexCount() const = 0;

    /*! Reallocates vertices, normals and colors. Calling this function invalidates SMeshDescription pointers */
    virtual void SetVertexCount(int nNewCount) = 0;

    //! Return number of allocated texture coordinates
    virtual int GetTexCoordCount() const = 0;

    // Get number of indices in the mesh.
    virtual int GetIndexCount() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Subset access.
    //////////////////////////////////////////////////////////////////////////
    virtual int GetSubSetCount() const = 0;
    virtual const SMeshSubset& GetSubSet(int nIndex) const = 0;
};
