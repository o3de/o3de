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

// Description : Animations environment parameters


#ifndef CRYINCLUDE_CRYMOVIE_ANIMENVIRONMENTNODE_H
#define CRYINCLUDE_CRYMOVIE_ANIMENVIRONMENTNODE_H
#pragma once

#include "AnimNode.h"

class CAnimEnvironmentNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CAnimEnvironmentNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CAnimEnvironmentNode, "{8CB3E585-1A24-43E0-8124-9AE51EAE7F4C}", CAnimNode);

    CAnimEnvironmentNode();
    CAnimEnvironmentNode(const int id);
    static void Initialize();

    // Overrides from CAnimNode
    virtual void Animate(SAnimContext& ac);
    virtual void CreateDefaultTracks();
    virtual void Activate(bool bActivate);

    //! Overrides from IAnimNode
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;
    virtual void InitializeTrack(IAnimTrack* pTrack, const CAnimParamType& paramType);

    void StoreCelestialPositions();
    void RestoreCelestialPositions();

    float m_oldSunLongitude;
    float m_oldSunLatitude;
    float m_oldMoonLongitude;
    float m_oldMoonLatitude;
    bool  m_celestialPositionsStored;
};


#endif // CRYINCLUDE_CRYMOVIE_ANIMENVIRONMENTNODE_H

