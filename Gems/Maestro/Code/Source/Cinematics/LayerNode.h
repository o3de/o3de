/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

    static void Reflect(AZ::ReflectContext* context);

protected:
    virtual bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const;

private:
    bool m_bInit;
    bool m_bPreVisibility;
};

#endif // CRYINCLUDE_CRYMOVIE_LAYERNODE_H
