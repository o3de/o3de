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

#ifndef CRYINCLUDE_CRY3DENGINE_CLOUDRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_CLOUDRENDERNODE_H
#pragma once

struct SCloudDescription;

//////////////////////////////////////////////////////////////////////////
// RenderNode for rendering single cloud object.
//////////////////////////////////////////////////////////////////////////
class CCloudRenderNode
    : public ICloudRenderNode
    , public Cry3DEngineBase
{
public:

    CCloudRenderNode();

    //////////////////////////////////////////////////////////////////////////
    // Implements ICloudRenderNode
    //////////////////////////////////////////////////////////////////////////
    virtual bool LoadCloud(const char* sCloudFilename);
    virtual bool LoadCloudFromXml(XmlNodeRef cloudNode);
    virtual void SetMovementProperties(const SCloudMovementProperties& properties);

    //////////////////////////////////////////////////////////////////////////
    // Implements IRenderNode
    //////////////////////////////////////////////////////////////////////////
    virtual void GetLocalBounds(AABB& bbox) { bbox = m_bounds; };
    virtual void SetMatrix(const Matrix34& mat);

    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName() const { return "Cloud"; }
    virtual const char* GetName() const { return "Cloud"; }
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual void Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
    void SetMaterial(_smart_ptr<IMaterial> pMat) override { m_pMaterial = pMat; }
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);
    //////////////////////////////////////////////////////////////////////////

    bool CheckIntersection(const Vec3& p1, const Vec3& p2);
    void MoveCloud();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
private:
    void SetCloudDesc(SCloudDescription* pCloud);
    ~CCloudRenderNode();
    virtual void SetMatrixInternal(const Matrix34& mat, bool updateOrigin);

private:
    Vec3 m_pos;
    float m_fScale;
    _smart_ptr<IMaterial> m_pMaterial;
    _smart_ptr<SCloudDescription> m_pCloudDesc;
    Matrix34 m_matrix;
    Matrix34 m_offsetedMatrix;
    Vec3 m_vOffset;
    AABB m_bounds;

    CREBaseCloud* m_pCloudRenderElement;
    CREImposter* m_pREImposter;
    float m_alpha;

    Vec3 m_origin;
    SCloudMovementProperties m_moveProps;

    AABB m_WSBBox;
};

#endif // CRYINCLUDE_CRY3DENGINE_CLOUDRENDERNODE_H
