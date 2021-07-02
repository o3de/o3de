/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>

class CUiAnimViewTrack;
class CUiAnimViewAnimNode;
class CUiAnimViewNode;

class CUiAnimViewAnimNodeFactory
{
public:
    CUiAnimViewAnimNode* BuildAnimNode(IUiAnimSequence* pSequence, IUiAnimNode* pAnimNode, CUiAnimViewNode* pParentNode);
};

class CUiAnimViewTrackFactory
{
public:
    CUiAnimViewTrack* BuildTrack(IUiAnimTrack* pTrack, CUiAnimViewAnimNode* pTrackAnimNode,
        CUiAnimViewNode* pParentNode, bool bIsSubTrack = false, unsigned int subTrackIndex = 0);
};

