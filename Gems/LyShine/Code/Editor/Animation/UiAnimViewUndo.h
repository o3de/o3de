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


#include "UiAnimViewTrack.h"

#include "UiAnimUndoObject.h"
#include "UiAnimUndo.h"

class CUiAnimViewNode;
class CUiAnimViewSequence;
class CUiAnimViewAnimNode;

/** Undo object for sequence settings
*/
class CUndoSequenceSettings
    : public UiAnimUndoObject
{
public:
    CUndoSequenceSettings(CUiAnimViewSequence* pSequence);

protected:
    virtual int GetSize() override { return sizeof(*this); }
    virtual const char* GetDescription() override { return "Undo Sequence Settings"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    CUiAnimViewSequence* m_pSequence;

    Range m_oldTimeRange;
    Range m_newTimeRange;
    IUiAnimSequence::EUiAnimSequenceFlags m_newFlags;
    IUiAnimSequence::EUiAnimSequenceFlags m_oldFlags;
};

/** Undo object stored when keys were selected
*/
class CUndoAnimKeySelection
    : public UiAnimUndoObject
{
    friend class CUndoTrackObject;
public:
    CUndoAnimKeySelection(CUiAnimViewSequence* pSequence);

    // Checks if the selection was actually changed
    bool IsSelectionChanged() const;

protected:
    CUndoAnimKeySelection(CUiAnimViewTrack* pTrack);

    virtual int GetSize() override { return sizeof(*this); }
    virtual const char* GetDescription() override { return "Undo Sequence Key Selection"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

    std::vector<bool> SaveKeyStates(CUiAnimViewSequence* pSequence) const;
    void RestoreKeyStates(CUiAnimViewSequence* pSequence, const std::vector<bool> keyStates);

    CUiAnimViewSequence* m_pSequence;
    std::vector<bool> m_undoKeyStates;
    std::vector<bool> m_redoKeyStates;
};

/** Undo object stored when track is modified.
*/
class CUndoTrackObject
    : public CUndoAnimKeySelection
{
public:
    CUndoTrackObject(CUiAnimViewTrack* pTrack, bool bStoreKeySelection = true);

protected:
    virtual int GetSize() override { return sizeof(*this); }
    virtual const char* GetDescription() override { return "Undo Track Modify"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    CUiAnimViewTrack* m_pTrack;
    bool m_bStoreKeySelection;

    CUiAnimViewTrackMemento m_undo;
    CUiAnimViewTrackMemento m_redo;
};

/** Base class for sequence add/remove
*/
class CAbstractUndoSequenceTransaction
    : public UiAnimUndoObject
{
public:
    CAbstractUndoSequenceTransaction(CUiAnimViewSequence* pSequence);

protected:
    void AddSequence();
    void RemoveSequence(bool bAquireOwnership);

    CUiAnimViewSequence* m_pSequence;

    // This smart pointer is used to hold the sequence if not in AnimView anymore
    std::unique_ptr<CUiAnimViewSequence> m_pStoredUiAnimViewSequence;
};

/** Undo for adding a sequence
*/
class CUndoSequenceAdd
    : public CAbstractUndoSequenceTransaction
{
public:
    CUndoSequenceAdd(CUiAnimViewSequence* pNewSequence)
        : CAbstractUndoSequenceTransaction(pNewSequence) {}

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Add Sequence"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for remove a sequence
*/
class CUndoSequenceRemove
    : public CAbstractUndoSequenceTransaction
{
public:
    CUndoSequenceRemove(CUiAnimViewSequence* pRemovedSequence);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Remove Sequence"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for changing current sequence
*/
class CUndoSequenceChange
    : public UiAnimUndoObject
{
public:
    CUndoSequenceChange(CUiAnimViewSequence* oldSequence, CUiAnimViewSequence* newSequence);

protected:
    int GetSize() override { return sizeof(*this); }
    const char* GetDescription() override { return "Undo Change Sequence"; }

    virtual void ChangeSequence(CUiAnimViewSequence* sequence);
    void Undo(bool undo) override;
    void Redo() override;

    CUiAnimViewSequence* m_oldSequence;
    CUiAnimViewSequence* m_newSequence;
};

/** Base class for anim node add/remove
*/
class CAbstractUndoAnimNodeTransaction
    : public UiAnimUndoObject
{
public:
    CAbstractUndoAnimNodeTransaction(CUiAnimViewAnimNode* pNode);

protected:
    void AddNode();
    void RemoveNode(bool bAquireOwnership);

    CUiAnimViewAnimNode* m_pParentNode;
    CUiAnimViewAnimNode* m_pNode;

    // This smart pointer is used to hold the node if not in the sequence anymore.
    std::unique_ptr<CUiAnimViewNode> m_pStoredUiAnimViewNode;
};

/** Undo for adding a sub node to a node
*/
class CUndoAnimNodeAdd
    : public CAbstractUndoAnimNodeTransaction
{
public:
    CUndoAnimNodeAdd(CUiAnimViewAnimNode* pNewNode)
        : CAbstractUndoAnimNodeTransaction(pNewNode) {}

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Add Animation Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for remove a sub node from a node
*/
class CUndoAnimNodeRemove
    : public CAbstractUndoAnimNodeTransaction
{
public:
    CUndoAnimNodeRemove(CUiAnimViewAnimNode* pRemovedNode);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Remove Animation Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Base class for anim track add/remove
*/
class CAbstractUndoTrackTransaction
    : public UiAnimUndoObject
{
public:
    CAbstractUndoTrackTransaction(CUiAnimViewTrack* pTrack);

protected:
    void AddTrack();
    void RemoveTrack(bool bAquireOwnership);

private:
    CUiAnimViewAnimNode* m_pParentNode;
    CUiAnimViewTrack* m_pTrack;

    // This smart pointers is used to hold the track if not in the sequence anymore.
    std::unique_ptr<CUiAnimViewNode> m_pStoredUiAnimViewTrack;
};

/** Undo for adding a track to a node
*/
class CUndoTrackAdd
    : public CAbstractUndoTrackTransaction
{
public:
    CUndoTrackAdd(CUiAnimViewTrack* pNewTrack)
        : CAbstractUndoTrackTransaction(pNewTrack) {}

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Add Animation Track"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for removing a track from a node
*/
class CUndoTrackRemove
    : public CAbstractUndoTrackTransaction
{
public:
    CUndoTrackRemove(CUiAnimViewTrack* pRemovedTrack);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Remove Animation Track"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;
};

/** Undo for re-parenting an anim node
*/
class CUndoAnimNodeReparent
    : public CAbstractUndoAnimNodeTransaction
{
public:
    CUndoAnimNodeReparent(CUiAnimViewAnimNode* pAnimNode, CUiAnimViewAnimNode* pNewParent);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Reparent Animation Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    void Reparent(CUiAnimViewAnimNode* pNewParent);
    void AddParentsInChildren(CUiAnimViewAnimNode* pCurrentNode);

    CUiAnimViewAnimNode* m_pNewParent;
    CUiAnimViewAnimNode* m_pOldParent;
};

/* Undo for renaming an anim node
*/
class CUndoAnimNodeRename
    : public UiAnimUndoObject
{
public:
    CUndoAnimNodeRename(CUiAnimViewAnimNode* pNode, const string& oldName);

protected:
    virtual int GetSize() override { return sizeof(*this); };
    virtual const char* GetDescription() override { return "Undo Rename Animation Node"; };

    virtual void Undo(bool bUndo) override;
    virtual void Redo() override;

private:
    CUiAnimViewAnimNode* m_pNode;
    string m_newName;
    string m_oldName;
};

/** Base class for track event transactions
*/
class CAbstractUndoTrackEventTransaction
    : public UiAnimUndoObject
{
public:
    CAbstractUndoTrackEventTransaction(CUiAnimViewSequence* pSequence, const QString& eventName);

protected:
    CUiAnimViewSequence* m_pSequence;
    QString m_eventName;
};

/** Undo for adding a track event
*/
class CUndoTrackEventAdd
    : public CAbstractUndoTrackEventTransaction
{
public:
    CUndoTrackEventAdd(CUiAnimViewSequence* pSequence, const QString& eventName)
        : CAbstractUndoTrackEventTransaction(pSequence, eventName) {}

protected:
    int GetSize() override { return sizeof(*this); };
    const char* GetDescription() override { return "Undo Add Track Event"; };

    void Undo(bool bUndo) override;
    void Redo() override;
};

/** Undo for removing a track event
*/
class CUndoTrackEventRemove
    : public CAbstractUndoTrackEventTransaction
{
public:
    CUndoTrackEventRemove(CUiAnimViewSequence* pSequence, const QString& eventName);

protected:
    int GetSize() override { return sizeof(*this); };
    const char* GetDescription() override { return "Undo Remove Track Event"; };

    void Undo(bool bUndo) override;
    void Redo() override;

    CUiAnimViewKeyBundle m_changedKeys;
};

/** Undo for renaming a track event
*/
class CUndoTrackEventRename
    : public CAbstractUndoTrackEventTransaction
{
public:
    CUndoTrackEventRename(CUiAnimViewSequence* pSequence, const QString& eventName, const QString& newEventName);

protected:
    int GetSize() override { return sizeof(*this); };
    const char* GetDescription() override { return "Undo Rename Track Event"; };

    void Undo(bool bUndo) override;
    void Redo() override;

    QString m_newEventName;
};

/** Base class for undoing moving a track event
*/
class CAbstractUndoTrackEventMove
    : public CAbstractUndoTrackEventTransaction
{
public:
    CAbstractUndoTrackEventMove(CUiAnimViewSequence* pSequence, const QString& eventName)
        : CAbstractUndoTrackEventTransaction(pSequence, eventName) {}

protected:
    void MoveUp();
    void MoveDown();
};

/** Undo for moving up a track event
*/
class CUndoTrackEventMoveUp
    : public CAbstractUndoTrackEventMove
{
public:
    CUndoTrackEventMoveUp(CUiAnimViewSequence* pSequence, const QString& eventName)
        : CAbstractUndoTrackEventMove(pSequence, eventName) {}

protected:
    int GetSize() override { return sizeof(*this); };
    const char* GetDescription() override { return "Undo Move Up Track Event"; };

    void Undo(bool bUndo) override;
    void Redo() override;
};

/** Undo for moving down a track event
*/
class CUndoTrackEventMoveDown
    : public CAbstractUndoTrackEventMove
{
public:
    CUndoTrackEventMoveDown(CUiAnimViewSequence* pSequence, const QString& eventName)
        : CAbstractUndoTrackEventMove(pSequence, eventName) {}

protected:
    int GetSize() override { return sizeof(*this); };
    const char* GetDescription() override { return "Undo Move Down Track Event"; };

    void Undo(bool bUndo) override;
    void Redo() override;
};
