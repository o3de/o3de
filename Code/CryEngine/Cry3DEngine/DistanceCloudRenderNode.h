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

#ifndef CRYINCLUDE_CRY3DENGINE_DISTANCECLOUDRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_DISTANCECLOUDRENDERNODE_H
#pragma once


class CDistanceCloudRenderNode
    : public IDistanceCloudRenderNode
    , public Cry3DEngineBase
{
public:
    // implements IDistanceCloudRenderNode
    virtual void SetProperties(const SDistanceCloudProperties& properties);

    // implements IRenderNode
    virtual void SetMatrix(const Matrix34& mat);

    virtual EERType GetRenderNodeType();
    virtual const char* GetEntityClassName() const;
    virtual const char* GetName() const;
    virtual Vec3 GetPos(bool bWorldOnly = true) const;
    virtual void Render(const SRendParams& rParam, const SRenderingPassInfo& passInfo);
    void SetMaterial(_smart_ptr<IMaterial> pMat) override;
    virtual _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = 0);
    virtual _smart_ptr<IMaterial> GetMaterialOverride() { return m_pMaterial; }
    virtual float GetMaxViewDist();
    virtual void Precache();
    virtual void GetMemoryUsage(ICrySizer* pSizer) const;
    virtual const AABB GetBBox() const { return m_WSBBox; }
    virtual void SetBBox(const AABB& WSBBox) { m_WSBBox = WSBBox; }
    virtual void FillBBox(AABB& aabb);
    virtual void OffsetPosition(const Vec3& delta);
    virtual void SetLayerId(uint16 nLayerId) { m_nLayerId = nLayerId; }
    virtual uint16 GetLayerId() { return m_nLayerId; }

public:
    CDistanceCloudRenderNode();
    SDistanceCloudProperties GetProperties() const;

private:
    ~CDistanceCloudRenderNode();

private:
    Vec3 m_pos;
    float m_sizeX;
    float m_sizeY;
    float m_rotationZ;
    _smart_ptr< IMaterial > m_pMaterial;
    AABB m_WSBBox;
    uint16 m_nLayerId;
};


#endif // CRYINCLUDE_CRY3DENGINE_DISTANCECLOUDRENDERNODE_H
