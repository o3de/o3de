/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYMOVIE_COMMENTNODE_H
#define CRYINCLUDE_CRYMOVIE_COMMENTNODE_H
#pragma once


#include "AnimNode.h"

class CCommentContext;

class CCommentNode
    : public CAnimNode
{
public:
    AZ_CLASS_ALLOCATOR(CCommentNode, AZ::SystemAllocator, 0);
    AZ_RTTI(CCommentNode, "{9FCBF56F-B7B3-4519-B3D2-9B7E5F7E6210}", CAnimNode);

    CCommentNode(const int id);
    CCommentNode();

    static void Initialize();

    //-----------------------------------------------------------------------------
    //! Overrides from CAnimNode
    virtual void Animate(SAnimContext& ac);

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
};

#endif // CRYINCLUDE_CRYMOVIE_COMMENTNODE_H
