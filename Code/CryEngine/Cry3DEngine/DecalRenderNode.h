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

#ifndef CRYINCLUDE_CRY3DENGINE_DECALRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_DECALRENDERNODE_H
#pragma once


#include "DecalManager.h"


class CDecalRenderNode
    : public IDecalRenderNode
    , public Cry3DEngineBase
{
public:
    // implements IDecalRenderNode
    virtual void SetDecalProperties(const SDecalProperties& properties);
    virtual const SDecalProperties* GetDecalProperties() const;
    virtual void CleanUpOldDecals();

    // implements IRenderNode
    virtual IRenderNode* Clone() const;
    virtual void SetMatrix(const Matrix34& mat);
    virtual const Matrix34& GetMatrix() { return m_Matrix; }

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

    virtual uint8 GetSortPriority() { return m_decalProperties.m_sortPrio; }

    virtual void SetLayerId(uint16 nLayerId) { m_nLayerId = nLayerId; }
    virtual uint16 GetLayerId() { return m_nLayerId; }
    static void ResetDecalUpdatesCounter() { CDecalRenderNode::m_nFillBigDecalIndicesCounter = 0; }

    // SetMatrix only supports changing position, this will do the full transform
    void SetMatrixFull(const Matrix34& mat);
public:
    CDecalRenderNode();
    void RequestUpdate() { m_updateRequested = true; DeleteDecal(); }
    void DeleteDecal();

private:
    ~CDecalRenderNode();
    void CreateDecals();
    void ProcessUpdateRequest();
    void UpdateAABBFromRenderMeshes();
    bool CheckForceDeferred();

    void SetCommonProperties(CryEngineDecalInfo& decalInfo);
    void CreatePlanarDecal();
    void CreateDecalOnTerrain();
    void CreateDecal(const CryEngineDecalInfo& decalInfo);

private:
    Vec3 m_pos;
    AABB m_localBounds;
    _smart_ptr<IMaterial> m_pMaterial;
    bool m_updateRequested;
    SDecalProperties m_decalProperties;
    CDecal* m_decal;
    AABB m_WSBBox;
    Matrix34 m_Matrix;
    uint32 m_nLastRenderedFrameId;
    uint16 m_nLayerId;

public:
    static int m_nFillBigDecalIndicesCounter;
};

#endif // CRYINCLUDE_CRY3DENGINE_DECALRENDERNODE_H
