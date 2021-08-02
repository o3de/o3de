/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>
#include "UiAnimViewAnimNode.h"

////////////////////////////////////////////////////////////////////////////
// This class represents an IUiAnimNode deditcated to firing Track Events
////////////////////////////////////////////////////////////////////////////

class CUiAnimViewEventNode
    : public CUiAnimViewAnimNode
    , public IUiTrackEventListener
{
public:
    CUiAnimViewEventNode(IUiAnimSequence* pSequence, IUiAnimNode* pAnimNode, CUiAnimViewNode* pParentNode);
    virtual ~CUiAnimViewEventNode();

    // overrides from IUiTrackEventListener
    void OnTrackEvent(IUiAnimSequence* pSequence, int reason, const char* event, void* pUserData) override;

    // gets all event keys that have eventName set as their Track Event
    static CUiAnimViewKeyBundle GetTrackEventKeys(CUiAnimViewAnimNode* pAnimNode, const char* eventName);

protected:
    // updates existing keys using fromName events, changes them to use the toName events instead
    void RenameTrackEvent(const char* fromName, const char* toName);

    // updates existing keys using removedEventName events to use the empty string (representing no event)
    void RemoveTrackEvent(const char* removedEventName);
};
