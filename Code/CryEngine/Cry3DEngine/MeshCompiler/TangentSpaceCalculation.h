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

// Description   : calculated the tangent space base vector for a given mesh
// Dependencies  : none
// Documentation : "How to calculate tangent base vectors.doc"


#ifndef CRYINCLUDE_CRY3DENGINE_MESHCOMPILER_TANGENTSPACECALCULATION_H
#define CRYINCLUDE_CRY3DENGINE_MESHCOMPILER_TANGENTSPACECALCULATION_H
#pragma once


enum eCalculateTangentSpaceErrorCode
{
    CALCULATE_TANGENT_SPACE_NO_ERRORS,
    BROKEN_TEXTURE_COORDINATES,
    VERTICES_SHARING_COORDINATES,
    ALL_VERTICES_ON_THE_SAME_VECTOR,
    MEMORY_ALLOCATION_FAILED
};

class ITriangleInputProxy
{
public:
    virtual ~ITriangleInputProxy(){}

    virtual uint32  GetTriangleCount() const = 0;
    virtual void    GetTriangleIndices(const uint32 indwTriNo, uint32 outdwPos[3], uint32 outdwNorm[3], uint32 outdwUV[3]) const = 0;
    virtual void    GetPos(const uint32 indwPos, Vec3& outfPos) const = 0;
    virtual void    GetUV(const uint32 indwPos, Vec2& outfUV) const = 0;
    virtual void    GetNorm(const uint32 indwTriNo, const uint32 indwVertNo, Vec3& outfNorm) const = 0;
};

class CTangentSpaceCalculation
{
public:
    //! /param inInput - the normals are only used as smoothing input - we calculate the normals ourself
    eCalculateTangentSpaceErrorCode CalculateTangentSpace(const ITriangleInputProxy& inInput, const bool bUseCustomNormals, string& errorMessage);

    size_t  GetBaseCount();
    void    GetTriangleBaseIndices(const uint32 indwTriNo, uint32 outdwBase[3]);

    //! returns a orthogonal base (perpendicular and normalized)
    void    GetBase(const uint32 indwPos, float* outU, float* outV, float* outN);

private:

    struct CBase33
    {
        CBase33();
        CBase33(const Vec3& Uval, const Vec3& Vval, const Vec3& Nval);

        Vec3 u;
        Vec3 v;
        Vec3 n; // part of the tangent base but can be used also as vertex normal
    };

    struct CVec3PredicateLess
    {
        bool operator() (const Vec3& first, const Vec3& second) const;
    };

    struct CBase33PredicateLess
    {
        bool operator() (const CBase33& first, const CBase33& second) const;
    };

    struct CBaseIndex
    {
        // position index in the positions stream
        uint32 m_posIndex;
        // normal index in the vertex normals stream
        uint32 m_normIndex;
    };

    struct CBaseIndexOrder
    {
        bool operator() (const CBaseIndex& a, const CBaseIndex& b) const;
    };

    struct CTriBaseIndex
    {
        uint32 p[3]; //!< index in m_BaseVectors
    };

    // [dwTriangleCount]
    std::vector<CTriBaseIndex>  m_trianglesBaseAssigment;
    // [0..] generated output data
    std::vector<CBase33>        m_baseVectors;

    eCalculateTangentSpaceErrorCode CalculateTangentSpaceMikk(const ITriangleInputProxy& inInput, string& errorMessage);

    CBase33& GetBase(std::multimap<CBaseIndex, uint32, CBaseIndexOrder>& inMap, const uint32 indwPosNo, const uint32 indwNormNo);
    uint32  AddUV2Base(std::multimap<CBaseIndex, uint32, CBaseIndexOrder>& inMap, const uint32 indwPosNo, const uint32 indwNormNo, const Vec3& inU, const Vec3& inV, const Vec3& inNormN);
    void    AddNormal2Base(std::multimap<CBaseIndex, uint32, CBaseIndexOrder>& inMap, const uint32 indwPosNo, const uint32 indwNormNo, const Vec3& inNormal);
    Vec3    Rotate(const Vec3& vFrom, const Vec3& vTo, const Vec3& vInput);
    void    DebugMesh(const ITriangleInputProxy& inInput) const;
    float   CalcAngleBetween(const Vec3& invA, const Vec3& invB);
};

#endif // CRYINCLUDE_CRY3DENGINE_MESHCOMPILER_TANGENTSPACECALCULATION_H
