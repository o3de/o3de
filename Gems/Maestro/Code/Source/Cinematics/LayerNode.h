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

// Description : Header of layer node to control entities properties in the
//               specific layer.


#ifndef CRYINCLUDE_CRYMOVIE_LAYERNODE_H
#define CRYINCLUDE_CRYMOVIE_LAYERNODE_H
#pragma once


#include "AnimNode.h"

class CLayerNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CLayerNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CLayerNode, "{C2E65C31-D469-4DE0-8F67-B5B00DE96E52}", CAnimNode);

    //-----------------------------------------------------------------------------
    //!
    CLayerNode();
    CLayerNode(const int id);
    static void Initialize();

    //-----------------------------------------------------------------------------
    //! Overrides from CAnimNode
    virtual void Animate(SAnimContext& ec);

    virtual void CreateDefaultTracks();

    virtual void OnReset();

    virtual void Activate(bool bActivate);

    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);

    //-----------------------------------------------------------------------------
    //! Overrides from IAnimNode
    virtual unsigned int GetParamCount() const;
    virtual CAnimParamType GetParamType(unsigned int nIndex) const;

    static void Reflect(AZ::SerializeContext* serializeContext);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

private:
    bool m_bInit;
    bool m_bPreVisibility;
};

#endif // CRYINCLUDE_CRYMOVIE_LAYERNODE_H
