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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWEVENTNODE_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWEVENTNODE_H
#pragma once

#include "IMovieSystem.h"
#include "TrackViewAnimNode.h"

////////////////////////////////////////////////////////////////////////////
// This class represents an IAnimNode deditcated to firing Track Events
////////////////////////////////////////////////////////////////////////////

class CTrackViewEventNode
    : public CTrackViewAnimNode
    , public ITrackEventListener
{
public:
    CTrackViewEventNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode);
    virtual ~CTrackViewEventNode();

    // overrides from ITrackEventListener
    void OnTrackEvent(IAnimSequence* pSequence, int reason, const char* event, void* pUserData) override;

protected:
    // updates existing keys using fromName events, changes them to use the toName events instead
    void RenameTrackEvent(const char* fromName, const char* toName);

    // updates existing keys using removedEventName events to use the empty string (representing no event)
    void RemoveTrackEvent(const char* removedEventName);
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWEVENTNODE_H
