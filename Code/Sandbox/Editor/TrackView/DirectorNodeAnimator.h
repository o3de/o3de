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

#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_DIRECTORNODEANIMATOR_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_DIRECTORNODEANIMATOR_H
#pragma once


#include "TrackViewAnimNode.h"

// This is used to bind/unbind sub sequences in director nodes
// when the sequence time changes. A sequence only gets bound if it was already
// referred in time before.
class CDirectorNodeAnimator
    : public IAnimNodeAnimator
{
public:
    CDirectorNodeAnimator(CTrackViewAnimNode* pDirectorNode);

    virtual void Animate(CTrackViewAnimNode* pNode, const SAnimContext& ac) override;
    virtual void Render(CTrackViewAnimNode* pNode, const SAnimContext& ac) override;
    virtual void UnBind(CTrackViewAnimNode* pNode) override;

    // Utility function to find a CTrackViewSequence* from an ISequenceKey
    static CTrackViewSequence* GetSequenceFromSequenceKey(const ISequenceKey& sequenceKey);

private:
    void ForEachActiveSequence(const SAnimContext& ac, CTrackViewTrack* pSequenceTrack,
        const bool bHandleOtherKeys, std::function<void(CTrackViewSequence*, const SAnimContext&)> animateFunction,
        std::function<void(CTrackViewSequence*, const SAnimContext&)> resetFunction);

    CTrackViewAnimNode* m_pDirectorNode;
};
#endif // CRYINCLUDE_EDITOR_TRACKVIEW_DIRECTORNODEANIMATOR_H
