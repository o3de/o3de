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

#ifndef __CREBEAM_H__
#define __CREBEAM_H__

#define BEAM_RE_CONE_SIDES 32
//=============================================================


class CREBeam
    : public CRendElementBase
{
private:

    CCryNameR m_eyePosInWSName;
    CCryNameR m_projMatrixName;
    CCryNameR m_invProjMatrixName;
    CCryNameR m_shadowCoordsName;
    CCryNameR m_lightParamsName;
    CCryNameR m_sphereParamsName;
    CCryNameR m_coneParamsName;
    CCryNameR m_lightPosName;
    CCryNameR m_miscOffsetsName;
    CCryNameR m_sampleOffsetsName;
    CCryNameR m_lightDiffuseName;
    CCryNameR m_screenScaleName;

public:
    CREBeam()
    {
        mfSetType(eDATA_Beam);

        m_eyePosInWSName        = CCryNameR("eyePosInWS");
        m_projMatrixName        = CCryNameR("projMatrix");
        m_invProjMatrixName = CCryNameR("invProjMatrix");
        m_shadowCoordsName  = CCryNameR("shadowCoords");
        m_lightParamsName       = CCryNameR("lightParams");
        m_sphereParamsName  = CCryNameR("sphereParams");
        m_coneParamsName        = CCryNameR("coneParams");
        m_lightPosName          = CCryNameR("lightPos");
        m_miscOffsetsName       = CCryNameR("MiscParams");
        m_sampleOffsetsName = CCryNameR("SampleOffsets");
        m_lightDiffuseName  = CCryNameR("lightDiffuse");
        m_screenScaleName       = CCryNameR("g_ScreenScale");
    }

    virtual ~CREBeam()
    {
    }

    virtual void mfPrepare(bool bCheckOverflow);
    virtual bool mfCompile(CParserBin& Parser, SParserFrame& Frame);
    virtual bool mfDraw(CShader* ef, SShaderPass* sl);
    virtual void mfExport([[maybe_unused]] struct SShaderSerializeContext& SC) {};
    virtual void mfImport([[maybe_unused]] struct SShaderSerializeContext& SC, [[maybe_unused]] uint32& offset) {};

    void SetupGeometry(SVF_P3F_C4B_T2F* pVertices, uint16* pIndices, float fAngleCoeff, float fNear, float fFar);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
};

#endif  // __CREBEAM_H__
