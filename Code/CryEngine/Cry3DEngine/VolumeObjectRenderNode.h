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

#ifndef CRYINCLUDE_CRY3DENGINE_VOLUMEOBJECTRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_VOLUMEOBJECTRENDERNODE_H
#pragma once

class CREVolumeObject;
class CVolumeDataItem;
class CVolumeShadowCreator;


class CVolumeObjectRenderNode
    : public IVolumeObjectRenderNode
    , public Cry3DEngineBase
{
public:
    static void MoveVolumeObjects();

public:
    CVolumeObjectRenderNode();

    // implements IVolumeObjectRenderNode
    virtual void LoadVolumeData(const char* filePath);
    virtual void SetProperties(const SVolumeObjectProperties& properties);
    virtual void SetMovementProperties(const SVolumeObjectMovementProperties& properties);

    // implements IRenderNode
    virtual void SetMatrix(const Matrix34& mat);

    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName() const;
    virtual const char* GetName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual void Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
    virtual void SetMaterial(_smart_ptr<IMaterial> pMat);
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = 0);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual void Precache();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);


private:
    typedef std::set<CVolumeObjectRenderNode*> VolumeObjectSet;

private:
    static void RegisterVolumeObject(CVolumeObjectRenderNode* p);
    static void UnregisterVolumeObject(CVolumeObjectRenderNode* p);

private:
    ~CVolumeObjectRenderNode();
    bool IsViewerInsideVolume(const SRenderingPassInfo& passInfo) const;
    bool NearPlaneIntersectsVolume(const SRenderingPassInfo& passInfo) const;
    void InvalidateLastCachedLightDir();
    void UpdateShadows();
    Plane GetVolumeTraceStartPlane(bool viewerInsideVolume, const SRenderingPassInfo& passInfo) const;
    float GetDistanceToCamera(const SRenderingPassInfo& passInfo) const;
    void SetMatrixInternal(const Matrix34& mat, bool updateOrigin);
    void Move();

private:
    static CVolumeShadowCreator* ms_pVolShadowCreator;
    static StaticInstance<VolumeObjectSet> ms_volumeObjects;

    static ICVar* ms_CV_volobj_stats;
    static int e_volobj_stats;

private:
    AABB m_WSBBox;

    Vec3 m_pos;
    Vec3 m_origin;

    Matrix34 m_matOrig;
    Matrix34 m_mat;
    Matrix34 m_matInv;

    Vec3 m_lastCachedLightDir;

    AABB m_tightBoundsOS;

    SVolumeObjectMovementProperties m_moveProps;

    float m_alpha;
    float m_scale;

    float m_shadowStrength;

    _smart_ptr< IMaterial > m_pMaterial;

    CREVolumeObject* m_pRE[RT_COMMAND_BUF_COUNT];

    CVolumeDataItem* m_pVolDataItem;
    IVolumeTexture* m_pVolShadTex;
};

#endif // CRYINCLUDE_CRY3DENGINE_VOLUMEOBJECTRENDERNODE_H
