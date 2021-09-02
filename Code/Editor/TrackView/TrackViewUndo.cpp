/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TrackViewUndo.h"

// CryCommon
#include <CryCommon/Maestro/Types/AnimNodeType.h>

// Editor
#include "TrackView/TrackViewAnimNode.h"
#include "TrackView/TrackViewSequence.h"


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CUndoComponentEntityTrackObject::CUndoComponentEntityTrackObject(CTrackViewTrack* track)
{
    AZ_Assert(track, "Expected a valid track");
    if (track)
    {
        m_trackName = track->GetName();
        AZ_Assert(!m_trackName.empty(), "Expected a valid track name");

        CTrackViewAnimNode* animNode = track->GetAnimNode();
        AZ_Assert(animNode, "Expected a valid anim node");
        if (animNode)
        {
            m_trackComponentId = animNode->GetComponentId();
            AZ_Assert(m_trackComponentId != AZ::InvalidComponentId, "Expected a valid track component id");

            CTrackViewSequence* sequence = track->GetSequence();
            AZ_Assert(sequence, "Expected to find the sequence");
            if (sequence)
            {
                m_sequenceId = sequence->GetSequenceComponentEntityId();
                AZ_Assert(m_sequenceId.IsValid(), "Expected a valid sequence id");

                AnimNodeType nodeType = animNode->GetType();
                AZ_Assert(nodeType == AnimNodeType::Component, "Expected a this node to be a AnimNodeType::Component type");
                if (nodeType == AnimNodeType::Component)
                {
                    CTrackViewAnimNode* parentAnimNode = static_cast<CTrackViewAnimNode*>(animNode->GetParentNode());
                    AZ_Assert(parentAnimNode, "Expected a valid parent node");
                    if (parentAnimNode)
                    {
                        m_entityId = parentAnimNode->GetAzEntityId();
                        AZ_Assert(m_entityId.IsValid(), "Expected a valid sequence id");

                        // Store undo info.
                        m_undo = track->GetMemento();
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack* CUndoComponentEntityTrackObject::FindTrack(CTrackViewSequence* sequence)
{
    AZ_Assert(sequence, "Expected to find the sequence");

    CTrackViewTrack* track = nullptr;
    CTrackViewTrackBundle allTracks = sequence->GetAllTracks();
    for (unsigned int trackIndex = 0; trackIndex < allTracks.GetCount(); trackIndex++)
    {
        CTrackViewTrack* curTrack = allTracks.GetTrack(trackIndex);
        if (curTrack->GetAnimNode() && curTrack->GetAnimNode()->GetComponentId() == m_trackComponentId)
        {
            if (curTrack->GetName() == m_trackName)
            {
                CTrackViewAnimNode* parentAnimNode = static_cast<CTrackViewAnimNode*>(curTrack->GetAnimNode()->GetParentNode());
                if (parentAnimNode && parentAnimNode->GetAzEntityId() == m_entityId)
                {
                    track = curTrack;
                    break;
                }
            }
        }
    }
    return track;
}

//////////////////////////////////////////////////////////////////////////
void CUndoComponentEntityTrackObject::Undo(bool bUndo)
{
    CTrackViewSequence* sequence = CTrackViewSequence::LookUpSequenceByEntityId(m_sequenceId);
    AZ_Assert(sequence, "Expected to find the sequence");
    if (sequence)
    {
        CTrackViewTrack* track = FindTrack(sequence);
        AZ_Assert(track, "Expected to find track");
        {
            CTrackViewSequenceNoNotificationContext context(sequence);

            if (bUndo)
            {
                m_redo = track->GetMemento();
            }

            // Undo track state.
            track->RestoreFromMemento(m_undo);
        }

        if (bUndo)
        {
            sequence->OnKeysChanged();
        }
        else
        {
            sequence->ForceAnimation();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CUndoComponentEntityTrackObject::Redo()
{
    CTrackViewSequence* sequence = CTrackViewSequence::LookUpSequenceByEntityId(m_sequenceId);
    AZ_Assert(sequence, "Expected to find the sequence");
    if (sequence)
    {
        CTrackViewTrack* track = FindTrack(sequence);
        AZ_Assert(track, "Expected to find track");

        // Redo track state.
        track->RestoreFromMemento(m_redo);

        sequence->OnKeysChanged();
    }
}
