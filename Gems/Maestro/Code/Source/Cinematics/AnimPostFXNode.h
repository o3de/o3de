/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_ANIMPOSTFXNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMPOSTFXNODE_H
#pragma once


#include "AnimNode.h"

class CFXNodeDescription;

//////////////////////////////////////////////////////////////////////////
class CAnimPostFXNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimPostFXNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimPostFXNode, "{41FCA8BB-46A8-4F37-87C2-C1D10994854B}", CAnimNode);

    //-----------------------------------------------------------------------------
    //!
    static void Initialize();
    static CFXNodeDescription* GetFXNodeDescription(AnimNodeType nodeType);
    static CAnimNode* CreateNode(const int id, AnimNodeType nodeType);

    //-----------------------------------------------------------------------------
    //!
    CAnimPostFXNode();
    CAnimPostFXNode(const int id, AnimNodeType nodeType, CFXNodeDescription* pDesc);

    //-----------------------------------------------------------------------------
    //!
    void SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

    //-----------------------------------------------------------------------------
    //!
    unsigned int GetParamCount() const override;
    CAnimParamType GetParamType(unsigned int nIndex) const override;

    //-----------------------------------------------------------------------------
    //!

    //-----------------------------------------------------------------------------
    //!
    void CreateDefaultTracks() override;

    void OnReset() override;

    //-----------------------------------------------------------------------------
    //!
    void Animate(SAnimContext& ac) override;

    void InitPostLoad(IAnimSequence* sequence) override;

    static void Reflect(AZ::ReflectContext* context);

protected:
    bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

    typedef std::map< AnimNodeType, _smart_ptr<CFXNodeDescription> > FxNodeDescriptionMap;
    static StaticInstance<FxNodeDescriptionMap> s_fxNodeDescriptions;
    static bool s_initialized;

    CFXNodeDescription* m_pDescription;
};

#endif // CRYINCLUDE_CRYMOVIE_ANIMPOSTFXNODE_H
