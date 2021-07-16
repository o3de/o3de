/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWUNDO_H
#define CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWUNDO_H
#pragma once

#include "TrackViewTrack.h"

#include "Undo/IUndoObject.h"

class CTrackViewSequence;

/** Undo object stored when track is modified for component entity.
* Stores ids, not raw pointers.
*/
class CUndoComponentEntityTrackObject
    : public IUndoObject
{
public:
    CUndoComponentEntityTrackObject(CTrackViewTrack* track);

protected:
    virtual int GetSize() override { return sizeof(*this); }
    virtual QString GetDescription() override { return "Undo Component Entity Track Modify"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:

    // Helper function to get a pointer to the track based.
    CTrackViewTrack* FindTrack(CTrackViewSequence* sequence);

    // Internal state are id's used to uniquely identify
    // a track. This does not store pointers to tracks because
    // those can change when an AZ::Undo event happens and the entity
    // is reloaded.
    AZ::EntityId m_sequenceId;
    AZ::EntityId m_entityId;
    AZStd::string m_trackName;
    AZ::ComponentId m_trackComponentId;

    CTrackViewTrackMemento m_undo;
    CTrackViewTrackMemento m_redo;
};

#endif // CRYINCLUDE_EDITOR_TRACKVIEW_TRACKVIEWUNDO_H
