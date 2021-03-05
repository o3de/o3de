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

#include "UiCanvasEditor_precompiled.h"
#include "UiAnimViewNodeFactories.h"
#include "UiAnimViewAnimNode.h"
#include "UiAnimViewEventNode.h"
//#include "UiAnimViewCameraNode.h"
#include "UiAnimViewTrack.h"

CUiAnimViewAnimNode* CUiAnimViewAnimNodeFactory::BuildAnimNode(IUiAnimSequence* pSequence, IUiAnimNode* pAnimNode, CUiAnimViewNode* pParentNode)
{
    CUiAnimViewAnimNode* retNode = nullptr;

    if (pAnimNode->GetType() == eUiAnimNodeType_Event)
    {
        retNode = new CUiAnimViewEventNode(pSequence, pAnimNode, pParentNode);
    }
    else
    {
        retNode = new CUiAnimViewAnimNode(pSequence, pAnimNode, pParentNode);
    }

    return retNode;
}

CUiAnimViewTrack* CUiAnimViewTrackFactory::BuildTrack(IUiAnimTrack* pTrack, CUiAnimViewAnimNode* pTrackAnimNode,
    CUiAnimViewNode* pParentNode, bool bIsSubTrack, unsigned int subTrackIndex)
{
#if UI_ANIMATION_REMOVED    // don't support geom cache
#if defined(USE_GEOM_CACHES)
    if (pTrack->GetParameterType() == eUiAnimParamType_TimeRanges && pTrackAnimNode->GetType() == eUiAnimNodeType_GeomCache)
    {
        return new CUiAnimViewGeomCacheUiAnimationTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
    }
#endif
#endif

    return new CUiAnimViewTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
}
