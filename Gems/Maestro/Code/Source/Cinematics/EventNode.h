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

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    //! Last animated key in track.
    int m_lastEventKey;
};

#endif // CRYINCLUDE_CRYMOVIE_EVENTNODE_H
