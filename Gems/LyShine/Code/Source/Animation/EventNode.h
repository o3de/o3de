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

#pragma once

#include "AnimNode.h"

class CUiAnimEventNode
    : public CUiAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CUiAnimEventNode, AZ::SystemAllocator, 0);
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
