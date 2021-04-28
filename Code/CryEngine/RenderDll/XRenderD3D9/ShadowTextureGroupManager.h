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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_SHADOWTEXTUREGROUPMANAGER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_SHADOWTEXTUREGROUPMANAGER_H
#pragma once

#include <vector>           // STL vector<>

// to combine multiple shadowmaps into one texture (e.g. GSM levels or Cubemap sides)
class CShadowTextureGroupManager
{
public:

    // Arguments
    //   pLightOwner - must not be 0
    // Returns
    //   pointer to the texture pointer, don't store this pointer
    SDynTexture_Shadow** FindOrCreateShadowTextureGroup(const IRenderNode* pLightOwner)
    {
        std::vector<SShadowTextureGroup>::iterator it, end = m_GSMGroups.end();

        for (it = m_GSMGroups.begin(); it != end; ++it)
        {
            SShadowTextureGroup& ref = *it;

            if (ref.m_pLightOwner == pLightOwner)
            {
                return &(ref.m_pTextureGroupItem);
            }
        }

        m_GSMGroups.push_back(SShadowTextureGroup(pLightOwner, 0));

        return &(m_GSMGroups.back().m_pTextureGroupItem);
    }

    void RemoveTextureGroupEntry(const IRenderNode* pLightOwner)
    {
        std::vector<SShadowTextureGroup>::iterator it, end = m_GSMGroups.end();

        for (it = m_GSMGroups.begin(); it != end; ++it)
        {
            SShadowTextureGroup& ref = *it;

            if (ref.m_pLightOwner == pLightOwner)
            {
                m_GSMGroups.erase(it);
                break;
            }
        }
    }

    //  TODO:  remove old entries !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

private: // ----------------------------------------------------------------------------------

    struct SShadowTextureGroup
    {
        SShadowTextureGroup(const IRenderNode* pLightOwner, SDynTexture_Shadow* pTextureGroupItem)
            : m_pLightOwner(pLightOwner)
            , m_pTextureGroupItem(pTextureGroupItem)
        {
        }

        const IRenderNode*                 m_pLightOwner;                                           // key
        SDynTexture_Shadow*                m_pTextureGroupItem;                                 // can be extended to combine 6 cubemap sides
    };

    std::vector<SShadowTextureGroup>            m_GSMGroups;        // could be a map<emitterid,m_pTextureGroupItem[]> but vector is faster for small containers
};



#endif // SHADOW_TEXTURE_GROUP_MANAGER_H
