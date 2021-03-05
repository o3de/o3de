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
#ifndef _EFFECTS_RENDERNODES_IGAMERENDERNODE_H_
#define _EFFECTS_RENDERNODES_IGAMERENDERNODE_H_
#pragma once

#include <IEntityRenderState.h>
#include <TypeLibrary.h>

// Forward declares
struct IGameRenderNodeParams;

//==================================================================================================
// Name: IGameRenderNode
// Desc: Base interface for all game render nodes
// Author: James Chilvers
//==================================================================================================
struct IGameRenderNode
    : public IRenderNode
    , public _i_reference_target_t
{
    DECLARE_TYPELIB(IGameRenderNode); // Allow soft coding on this interface

    virtual ~IGameRenderNode() {}

    virtual bool InitialiseGameRenderNode() = 0;
    virtual void ReleaseGameRenderNode() = 0;

    virtual void SetParams(const IGameRenderNodeParams* pParams = NULL) = 0;
}; //-----------------------------------------------------------------------------------------------

//==================================================================================================
// Name: IGameRenderNodeParams
// Desc: Game render node params
// Author: James Chilvers
//==================================================================================================
struct IGameRenderNodeParams
{
    virtual ~IGameRenderNodeParams() {}
}; //------------------------------------------------------------------------------------------------

#endif//_EFFECTS_RENDERNODES_IGAMERENDERNODE_H_
