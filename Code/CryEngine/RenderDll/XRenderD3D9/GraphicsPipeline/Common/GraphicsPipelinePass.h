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

#include <Range.h>
#include "GraphicsPipelineStateSet.h"
// forward declarations
typedef std::shared_ptr<CGraphicsPipelineStateLocalCache> CGraphicsPipelineStateLocalCachePtr;
class GraphicsPipelinePass;

struct SGraphicsPiplinePassContext
{
    SGraphicsPiplinePassContext(GraphicsPipelinePass* pass, EShaderTechniqueID technique, uint32 filter)
        : pPass(pass)
        , techniqueID(technique)
        , batchFilter(filter)
        , nFrameID(0)
        , renderListId(EFSLIST_INVALID)
        , sortGroupID(0)
        , passId(0)
        , passSubId(0)
        , pPipelineStats(0)
    {
    }

    GraphicsPipelinePass* pPass;
    EShaderTechniqueID techniqueID;
    uint32 batchFilter;

    ERenderListID renderListId;
    int           sortGroupID;
    threadID      nProcessThreadID;

    uint64        nFrameID;

    // One of ERenderableTechnique
    uint32        passId;
    // When pass have multiple sub-passes, specified here, selects a different PSO from compiled render object.
    uint32        passSubId;

    // Current pipeline stats.
    SPipeStat* pPipelineStats;

    // rend items
    TRange<int>    rendItems;
};

class GraphicsPipelinePass
{
public:
    virtual ~GraphicsPipelinePass() {}

    // Allocate resources needed by the pipeline pass
    virtual void Init() = 0;
    // Free resources used by the pipeline pass
    virtual void Shutdown() = 0;
    // Prepare pass before actual rendering starts (called every frame)
    virtual void Prepare() {};
    // Force pass to reset data
    virtual void Reset()  = 0;

    // initialize command list with pass specific data
    virtual void PrepareCommandList([[maybe_unused]] CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList) const {}
};
