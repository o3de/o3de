/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewNodeFactories.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>

// Editor
#include "TrackViewEventNode.h"


CTrackViewAnimNode* CTrackViewAnimNodeFactory::BuildAnimNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode)
{
    CTrackViewAnimNode* retNode = nullptr;

    if (pAnimNode->GetType() == AnimNodeType::Event)
    {
        retNode = new CTrackViewEventNode(pSequence, pAnimNode, pParentNode);
    }
    else
    {
        retNode = new CTrackViewAnimNode(pSequence, pAnimNode, pParentNode);
    }

    return retNode;
}

CTrackViewTrack* CTrackViewTrackFactory::BuildTrack(IAnimTrack* pTrack, CTrackViewAnimNode* pTrackAnimNode,
    CTrackViewNode* pParentNode, bool bIsSubTrack, unsigned int subTrackIndex)
{
    return new CTrackViewTrack(pTrack, pTrackAnimNode, pParentNode, bIsSubTrack, subTrackIndex);
}
