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

#ifndef CRYINCLUDE_CRY3DENGINE_VOLUMEOBJECTDATACREATE_H
#define CRYINCLUDE_CRY3DENGINE_VOLUMEOBJECTDATACREATE_H
#pragma once

const int VOLUME_SIZE(64);
const int VOLUME_SHADOW_SIZE(32);


template <typename T>
struct SVolumeDataSrc
{
    typedef T VoxelElementType;

    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_depth;
    unsigned int m_slice;
    VoxelElementType* m_pData;

    SVolumeDataSrc(unsigned int width, unsigned int height, unsigned int depth)
        : m_width(width)
        , m_height(height)
        , m_depth(depth)
        , m_slice(width * depth)
        , m_pData(0)
    {
        m_pData = new VoxelElementType[m_width * m_height * m_depth];
    }

    ~SVolumeDataSrc()
    {
        SAFE_DELETE_ARRAY(m_pData)
    }

    size_t size() const
    {
        return m_width * m_height * m_depth;
    }

    size_t Idx(unsigned int x, unsigned int y, unsigned int z) const
    {
        //return z * m_width * m_height + y * m_width + x;
        return (z * m_height + y) * m_width + x;
    }

    T& operator[](size_t idx)
    {
        return m_pData[idx];
    }

    const T& operator[](size_t idx) const
    {
        return m_pData[idx];
    }
};


typedef SVolumeDataSrc<uint8> SVolumeDataSrcB;
typedef SVolumeDataSrc<float> SVolumeDataSrcF;


bool CreateVolumeObject(const char* filePath, SVolumeDataSrcB& trg, AABB& tightBounds, float& scale);
bool CreateVolumeShadow(const Vec3& lightDir, float shadowStrength, const SVolumeDataSrcB& density, SVolumeDataSrcB& shadows);
bool CreateDownscaledVolumeObject(const SVolumeDataSrcB& src, SVolumeDataSrcB& trg);


struct SVolumeDataHull
{
    SVolumeDataHull()
        : m_numPts(0)
        , m_numTris(0)
        , m_pPts(0)
        , m_pIdx(0)
    {
    }

    ~SVolumeDataHull()
    {
        SAFE_DELETE_ARRAY(m_pPts);
        SAFE_DELETE_ARRAY(m_pIdx);
    }

    size_t m_numPts;
    size_t m_numTris;
    SVF_P3F* m_pPts;
    vtx_idx* m_pIdx;
};


bool CreateVolumeDataHull(const SVolumeDataSrcB& src, SVolumeDataHull& hull);

#endif // CRYINCLUDE_CRY3DENGINE_VOLUMEOBJECTDATACREATE_H
