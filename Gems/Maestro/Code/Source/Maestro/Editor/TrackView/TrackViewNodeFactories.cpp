/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Maestro/Editor/TrackView/TrackViewNodeFactories.h>

#include "EditorDefs.h"

#include <Maestro/Editor/TrackView/TrackViewEventNode.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimParamType.h>


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
