/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_EVENTNODE_H
#define CRYINCLUDE_CRYMOVIE_EVENTNODE_H
#pragma once


#include "AnimNode.h"

class CAnimEventNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimEventNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimEventNode, "{F9F306E0-FF9C-4FF4-B1CC-5A96746364FE}", CAnimNode);

    CAnimEventNode();
    CAnimEventNode(const int id);

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CAnimNode
    //////////////////////////////////////////////////////////////////////////
    virtual void Animate(SAnimContext& ec);
    virtual void CreateDefaultTracks();
    virtual void OnReset();

    //////////////////////////////////////////////////////////////////////////
    // Supported tracks description.
    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

    static void Reflect(AZ::ReflectContext* context);

private:
    //! Last animated key in track.
    int m_lastEventKey;
};

#endif // CRYINCLUDE_CRYMOVIE_EVENTNODE_H
