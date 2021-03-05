
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

#ifndef __CRECLOUD_H__
#define __CRECLOUD_H__

//=============================================================

#define FCEF_OLD    0x1000

class CRECloud
    : public CREBaseCloud
{
    friend class CREImposter;

protected: // datatypes
    typedef std::vector<SCloudParticle*>     ParticleArray;
    typedef ParticleArray::iterator         ParticleIterator;
    typedef ParticleArray::const_iterator   ParticleConstIterator;

    typedef std::vector<Vec3>              DirectionArray;
    typedef DirectionArray::iterator       DirectionIterator;

    class ParticleAwayComparator
    {
    public:
        bool operator()(SCloudParticle* pA, SCloudParticle* pB)
        {
            return ((*pA) < (*pB));
        }
    };

    class ParticleTowardComparator
    {
    public:
        bool operator()(SCloudParticle* pA, SCloudParticle* pB)
        {
            return ((*pA) > (*pB));
        }
    };

protected: // data
    enum ESortDirection
    {
        eSort_TOWARD,
        eSort_AWAY
    };

    ParticleArray               m_particles;     // cloud particles
    // particle sorting functors for STL sort.
    ParticleTowardComparator    m_towardComparator;
    ParticleAwayComparator      m_awayComparator;

    DirectionArray              m_lightDirections;// light directions in cloud space (cached)

    SMinMaxBox                  m_boundingBox;   // bounds

    bool                        m_bUseAnisoLighting;

    Vec3                        m_vLastSortViewDir;
    Vec3                        m_vLastSortCamPos;
    Vec3                        m_vSortPos;

    float                       m_fSplitDistance;
    bool                        m_bReshadeCloud;
    bool                        m_bEnabled;
    float                       m_fScale;

    CTexture*                   m_pTexParticle;
    uint32                      m_nNumPlanes;
    uint32                      m_nNumColorGradients;

    ColorF                      m_CurSpecColor;
    ColorF                      m_CurDiffColor;
    float                                               m_fCloudColorScale;     // needed for HDR (>=1)

    static uint32                 m_siShadeResolution;// the resolution of the viewport used for shading
    static float                m_sfAlbedo;      // the cloud albedo
    static float                m_sfExtinction;  // the extinction of the clouds
    static float                m_sfTransparency;// the transparency of the clouds
    static float                m_sfScatterFactor;// How much the clouds scatter
    static float                m_sfSortAngleErrorTolerance;// how far the view must turn to cause a resort.
    static float                m_sfSortSquareDistanceTolerance;// how far the view must move to cause a resort.


protected:
    void SortParticles(const Vec3& vViewDir, const Vec3& vSortPoint, ESortDirection eDir);
    void GetIllumParams(ColorF& specColor, ColorF& diffColor);
    void ShadeCloud(Vec3 vPos);
    void IlluminateCloud(Vec3 vLightPos, Vec3 vObjPos, ColorF cLightColor, ColorF cAmbColor, bool bReset);
    void DisplayWithoutImpostor(const CameraViewParameters& camera);
    bool mfDisplay(bool bDisplayFrontOfSplit);
    void UpdateWorldSpaceBounds(CRenderObject* pObj);
    inline float GetScale() { return m_fScale; }
    bool UpdateImposter(CRenderObject* pObj);
    bool mfLoadCloud(const string& name, float fScale, bool bLocal);

    void ClearParticles()
    {
        size_t size = m_particles.size();
        for (size_t i(0); i < size; ++i)
        {
            delete m_particles[i];
        }
        m_particles.resize(0);
    }

public:
    CRECloud()
        : CREBaseCloud()
        , m_bUseAnisoLighting(true)
        , m_bReshadeCloud(true)
        , m_bEnabled(true)
        , m_fScale(1.0f)
        , m_vLastSortViewDir(Vec3(0, 0, 0))
        , m_vLastSortCamPos(Vec3(0, 0, 0))
        , m_CurSpecColor(Col_White)
        , m_CurDiffColor(Col_White)
        , m_pTexParticle(NULL)
        , m_nNumPlanes(0)
        , m_nNumColorGradients(0)
        , m_fCloudColorScale(1)
    {
        mfSetType(eDATA_Cloud);
        mfSetFlags(FCEF_TRANSFORM);
    }

    virtual ~CRECloud()
    {
        ClearParticles();
    }

    virtual bool mfCompile(CParserBin& Parser, SParserFrame& Frame);
    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfDraw(CShader* ef, SShaderPass* sl);

    virtual void SetParticles(SCloudParticle* pParticles, int nNumParticles)
    {
        m_bReshadeCloud = true;

        m_boundingBox.Clear();

        ClearParticles();
        m_particles.reserve(nNumParticles);

        for (int i = 0; i < nNumParticles; i++)
        {
            SCloudParticle* pPart = new SCloudParticle;
            *pPart = pParticles[i];
            float rx = pPart->GetRadiusX();
            Vec3 vMin = pPart->GetPosition() - Vec3(rx, rx, rx);
            Vec3 vMax = pPart->GetPosition() + Vec3(rx, rx, rx);
            m_boundingBox.AddPoint(vMin);
            m_boundingBox.AddPoint(vMax);
            m_particles.push_back(pPart);
        }
    }

    bool GenerateCloudImposter(CShader* pShader, CShaderResources* pRes, CRenderObject* pObject);
};

#endif  // __CRECLOUD_H__
