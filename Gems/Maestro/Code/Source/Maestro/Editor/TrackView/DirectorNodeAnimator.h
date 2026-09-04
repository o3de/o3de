/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "TrackViewAnimNode.h"

#include <AzCore/std/functional.h>

// This is used to bind/unbind sub sequences in director nodes
// when the sequence time changes. A sequence only gets bound if it was already
// referred in time before.
class CDirectorNodeAnimator
    : public IAnimNodeAnimator
{
public:
    CDirectorNodeAnimator(CTrackViewAnimNode* pDirectorNode);

    void Animate(CTrackViewAnimNode* pNode, const SAnimContext& ac) override;
    void Render(CTrackViewAnimNode* pNode, const SAnimContext& ac) override;
    void UnBind(CTrackViewAnimNode* pNode) override;

    // Utility function to find a CTrackViewSequence* from an ISequenceKey
    static CTrackViewSequence* GetSequenceFromSequenceKey(const ISequenceKey& sequenceKey);

private:
    void ForEachActiveSequence(const SAnimContext& ac, CTrackViewTrack* pSequenceTrack,
        const bool bHandleOtherKeys, std::function<void(CTrackViewSequence*, const SAnimContext&)> animateFunction,
        std::function<void(CTrackViewSequence*, const SAnimContext&)> resetFunction);
};
