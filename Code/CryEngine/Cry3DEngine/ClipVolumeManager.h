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

#ifndef __INCLUDE_CRY3DENGINE_CLIPVOLUMEMANAGER_H
#define __INCLUDE_CRY3DENGINE_CLIPVOLUMEMANAGER_H

class CClipVolume;

class CClipVolumeManager
    : public Cry3DEngineBase
{
    struct SClipVolumeInfo
    {
        CClipVolume* m_pVolume;
        bool m_bActive;

        SClipVolumeInfo()
            : m_bActive(false) {}
        SClipVolumeInfo(CClipVolume* pVolume)
            : m_pVolume(pVolume)
            , m_bActive(false)
        {}

        bool operator==(const SClipVolumeInfo& other) const { return m_pVolume == other.m_pVolume; }
    };

public:
    static const uint8 InactiveVolumeStencilRef    = 0xFD;
    static const uint8 AffectsEverythingStencilRef = 0xFE;

    virtual ~CClipVolumeManager();

    virtual IClipVolume* CreateClipVolume();
    virtual bool DeleteClipVolume(IClipVolume* pClipVolume);
    virtual bool UpdateClipVolume(IClipVolume* pClipVolume, _smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, bool bActive, uint32 flags, const char* szName);

    void PrepareVolumesForRendering(const SRenderingPassInfo& passInfo);

    void UpdateEntityClipVolume(const Vec3& pos, IRenderNode* pRenderNode);
    void UnregisterRenderNode(IRenderNode* pRenderNode);

    bool IsClipVolumeRequired(IRenderNode* pRenderNode) const;
    CClipVolume* GetClipVolumeByPos(const Vec3& pos, const IClipVolume* pIgnoreVolume = NULL) const;

    void GetMemoryUsage(class ICrySizer* pSizer) const;
    size_t GetClipVolumeCount() const { return m_ClipVolumes.size(); }

private:
    PodArray<SClipVolumeInfo> m_ClipVolumes;
};


#endif //__INCLUDE_CRY3DENGINE_CLIPVOLUMEMANAGER_H
