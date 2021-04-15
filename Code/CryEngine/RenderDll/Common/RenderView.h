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

#include <CryThreadSafeRendererContainer.h>
#include <CryThreadSafeWorkerContainer.h>

#include "RenderPipeline.h"

// This class encapsulate all renderbale information to render a camera view.
// It stores list of render items added by 3D engine
class CRenderView
{
public:
    CRenderView();
    ~CRenderView();

    void FreeRenderItems();
    void ClearRenderItems();
    void InitRenderItems();

    CThreadSafeWorkerContainer<SRendItem>& GetRenderItems(int nAfterWater, int nRenderList);
    uint32 GetBatchFlags(int recusrion, int nAfterWater, int nRenderList) const;

    void AddRenderItem(IRenderElement* pElem, CRenderObject* RESTRICT_POINTER pObj, const SShaderItem& pSH,
        uint32 nList, int nAafterWater, uint32 nBatchFlags, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    void PrepareForRendering();

    void PrepareForWriting();

    static CRenderView* CurrentRenderView() { return gRenDev->m_RP.m_pCurrentRenderView; }
    static CRenderView* CurrentFillView()    { return gRenDev->m_RP.m_pCurrentFillView; }
    static CRenderView* GetRenderViewForThread(int thread) { return gRenDev->GetRenderViewForThread(thread); }

private:
    CThreadSafeWorkerContainer<SRendItem> m_renderItems[MAX_LIST_ORDER][EFSLIST_NUM];
    
    uint32 m_BatchFlags[MAX_REND_RECURSION_LEVELS][MAX_LIST_ORDER][EFSLIST_NUM];

public:
    SRenderListDesc m_RenderListDesc[MAX_REND_RECURSION_LEVELS];

private:
    CCamera m_camera;                       // Current camera
    CameraViewParameters m_viewParameters;

};

typedef std::shared_ptr<CRenderView> CRenderViewPtr;
