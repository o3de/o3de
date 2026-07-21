/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
    return new CUiAnimViewTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
}
