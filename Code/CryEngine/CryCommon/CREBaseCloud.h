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

#ifndef __CREBASECLOUD_H__
#define __CREBASECLOUD_H__

//================================================================================

class SCloudParticle
{
public:
    inline SCloudParticle();
    inline SCloudParticle(const Vec3& vPos, float fRadius, const ColorF& baseColor, float fTransparency = 0);
    inline SCloudParticle(const Vec3& vPos, float fRadiusX, float fRadiusY, float fRotMin, float fRotMax, Vec2 vUV[]);
    inline ~SCloudParticle();

    float               GetRadiusX() const               { return m_fSize[0];      }
    float               GetRadiusY() const               { return m_fSize[1];      }
    float               GetTransparency() const         { return m_fTransparency;  }
    const Vec3&         GetPosition() const             { return m_vPosition;  }
    const ColorF&       GetBaseColor() const            { return m_vBaseColor; }
    uint32                GetNumLitColors() const         { return m_vLitColors.size();  }
    inline const ColorF GetLitColor(unsigned int index) const;
    float               GetSquareSortDistance() const   { return m_fSquareSortDistance; }

    //! Sets the radius of the particle.
    void                SetRadiusX(float rad)            { m_fSize[0]     = rad;    }
    void                SetRadiusY(float rad)            { m_fSize[1]     = rad;    }
    void                SetTransparency(float trans)    { m_fTransparency = trans; }
    void                SetPosition(const Vec3& pos)    { m_vPosition  = pos;  }
    void                SetBaseColor(const ColorF& col) { m_vBaseColor = col;  }
    void                AddLitColor(const ColorF& col)  { m_vLitColors.push_back(col); }
    void                ClearLitColors()                { m_vLitColors.clear();        }
    void                SetSquareSortDistance(float fSquareDistance) { m_fSquareSortDistance = fSquareDistance; }

    bool operator<(const SCloudParticle& p) const
    {
        return (m_fSquareSortDistance < p.m_fSquareSortDistance);
    }

    bool operator>(const SCloudParticle& p) const
    {
        return (m_fSquareSortDistance > p.m_fSquareSortDistance);
    }

protected:
    float               m_fTransparency;
    Vec3                m_vPosition;
    float               m_fSize[2];
    float               m_fRotMin;
    float               m_fRotMax;
    ColorF              m_vBaseColor;
    TArray<ColorF>      m_vLitColors;
    Vec3                m_vEye;

    // for sorting particles during shading
    float               m_fSquareSortDistance;
public:
    Vec2                m_vUV[2];
};

inline SCloudParticle::SCloudParticle()
{
    m_fSize[0] = 0;
    m_fTransparency = 0;
    m_vPosition = Vec3(0, 0, 0);
    m_vBaseColor = Col_Black;
    m_vEye = Vec3(0, 0, 0);
    m_fSquareSortDistance = 0;

    m_vLitColors.clear();
}

inline SCloudParticle::SCloudParticle(const Vec3& pos, float fRadius, const ColorF& baseColor, float fTransparency)
{
    m_fSize[0] = fRadius;
    m_fSize[1] = fRadius;
    m_fTransparency = fTransparency;
    m_vPosition = pos;
    m_vBaseColor = baseColor;
    m_vUV[0] = Vec2(0, 0);
    m_vUV[1] = Vec2(1, 1);

    m_fRotMin = 0;
    m_fRotMax = 0;
    m_vEye = Vec3(0, 0, 0);
    m_fSquareSortDistance = 0;

    m_vLitColors.clear();
}

inline SCloudParticle::SCloudParticle(const Vec3& vPos, float fRadiusX, float fRadiusY, float fRotMin, float fRotMax, Vec2 vUV[2])
{
    m_fSize[0] = fRadiusX;
    m_fSize[1] = fRadiusY;
    m_vPosition = vPos;
    m_vBaseColor = Col_White;
    m_vUV[0] = vUV[0];
    m_vUV[1] = vUV[1];
    m_fRotMin = fRotMin;
    m_fRotMax = fRotMax;

    m_fTransparency = 1.0f;
    m_vEye = Vec3(0, 0, 0);
    m_fSquareSortDistance = 0;

    m_vLitColors.clear();
}

inline SCloudParticle::~SCloudParticle()
{
    m_vLitColors.clear();
}

inline const ColorF SCloudParticle::GetLitColor(unsigned int index) const
{
    if (index <= m_vLitColors.size())
    {
        return m_vLitColors[index];
    }
    else
    {
        return Col_Black;
    }
}

//===========================================================================
class CREBaseCloud
    : public CRendElementBase
{
    friend class CRECloud;

public:
    CREBaseCloud()
        : CRendElementBase()
    {
        mfSetType(eDATA_Cloud);
        mfUpdateFlags(FCEF_TRANSFORM);
    }
    virtual void SetParticles(SCloudParticle* pParticles, int nNumParticles) = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

#endif  // __CREBASECLOUD_H__
