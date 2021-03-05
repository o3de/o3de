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

#ifndef __INCLUDE_CRY3DENGINE_CLIPVOLUME_H
#define __INCLUDE_CRY3DENGINE_CLIPVOLUME_H

struct IBSPTree3D;

class CClipVolume
    : public IClipVolume
{
public:
    CClipVolume();
    virtual ~CClipVolume();

    ////////////// IClipVolume implementation //////////////
    virtual void GetClipVolumeMesh(_smart_ptr<IRenderMesh>& renderMesh, Matrix34& worldTM) const;
    virtual AABB GetClipVolumeBBox() const;

    virtual uint8 GetStencilRef() const { return m_nStencilRef; }
    virtual uint GetClipVolumeFlags() const { return m_nFlags; }
    virtual bool IsPointInsideClipVolume(const Vec3& vPos) const;
    ////////////////////////////

    void SetName(const char* szName);
    void SetStencilRef(int nStencilRef) { m_nStencilRef = nStencilRef; }

    void Update(_smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, uint32 flags);

    void RegisterRenderNode(IRenderNode* pRenderNode);
    void UnregisterRenderNode(IRenderNode* pRenderNode);

    void GetMemoryUsage(class ICrySizer* pSizer) const;

private:
    uint8 m_nStencilRef;
    uint32 m_nFlags;
    Matrix34 m_WorldTM;
    Matrix34 m_InverseWorldTM;
    AABB m_BBoxWS;
    AABB m_BBoxLS;

    _smart_ptr<IRenderMesh> m_pRenderMesh;
    IBSPTree3D* m_pBspTree;

    PodArray<IRenderNode*> m_lstRenderNodes;
    char m_sName[64];
};

#endif //__INCLUDE_CRY3DENGINE_CLIPVOLUME_H
