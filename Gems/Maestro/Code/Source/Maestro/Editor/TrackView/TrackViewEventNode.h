/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMovieSystem.h>
#include "TrackViewAnimNode.h"

//////////////////////////////////////////////////////////////////////////
// This class represents an IAnimNode dedicated to firing Track Events
//////////////////////////////////////////////////////////////////////////

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
