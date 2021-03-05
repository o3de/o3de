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
