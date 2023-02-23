/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "AnimNode.h"

class CUiAnimEventNode
    : public CUiAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CUiAnimEventNode, AZ::SystemAllocator);
    AZ_RTTI(CUiAnimEventNode, "{51C82937-293D-4E20-8966-5288D1580615}", CUiAnimNode);

    CUiAnimEventNode();
    CUiAnimEventNode(const int id);

    //////////////////////////////////////////////////////////////////////////
    // Overrides from CUiAnimNode
    //////////////////////////////////////////////////////////////////////////
    virtual void Animate(SUiAnimContext& ec);
    virtual void CreateDefaultTracks();
    virtual void OnReset();

    //////////////////////////////////////////////////////////////////////////
    // Supported tracks description.
    //////////////////////////////////////////////////////////////////////////
    virtual unsigned int GetParamCount() const;
    virtual CUiAnimParamType GetParamType(unsigned int nIndex) const;
    virtual bool GetParamInfoFromType(const CUiAnimParamType& paramId, SParamInfo& info) const;

    static void Reflect(AZ::SerializeContext* serializeContext);

private:
    void SetScriptValue();

private:
    //! Last animated key in track.
    int m_lastEventKey;
};
