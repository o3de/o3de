/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWNODEFACTORIES_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWNODEFACTORIES_H
#pragma once


class CTrackViewTrack;
class CTrackViewAnimNode;
class CTrackViewNode;

class CTrackViewAnimNodeFactory
{
public:
    CTrackViewAnimNode* BuildAnimNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode);
};

class CTrackViewTrackFactory
{
public:
    CTrackViewTrack* BuildTrack(IAnimTrack* pTrack, CTrackViewAnimNode* pTrackAnimNode,
        CTrackViewNode* pParentNode, bool bIsSubTrack = false, unsigned int subTrackIndex = 0);
};
#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWNODEFACTORIES_H
