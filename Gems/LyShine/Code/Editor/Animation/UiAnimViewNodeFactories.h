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

