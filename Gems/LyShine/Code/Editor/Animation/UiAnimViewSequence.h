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
#include "Undo/IUndoManagerListener.h"

struct IUiAnimViewSequenceListener
{
    // Called when sequence settings (time range, flags) have changed
    virtual void OnSequenceSettingsChanged([[maybe_unused]] CUiAnimViewSequence* pSequence) {}

    enum ENodeChangeType
    {
        eNodeChangeType_Added,
        eNodeChangeType_Removed,
        eNodeChangeType_Expanded,
        eNodeChangeType_Collapsed,
        eNodeChangeType_Hidden,
        eNodeChangeType_Unhidden,
        eNodeChangeType_Enabled,
        eNodeChangeType_Disabled,
        eNodeChangeType_Muted,
        eNodeChangeType_Unmuted,
        eNodeChangeType_Selected,
        eNodeChangeType_Deselected,
        eNodeChangeType_SetAsActiveDirector,
        eNodeChangeType_NodeOwnerChanged
    };

    // Called when a node is changed
    virtual void OnNodeChanged([[maybe_unused]] CUiAnimViewNode* pNode, [[maybe_unused]] ENodeChangeType type) {}

    // Called when a node is added
    virtual void OnNodeRenamed([[maybe_unused]] CUiAnimViewNode* pNode, [[maybe_unused]] const char* pOldName) {}

    // Called when selection of nodes changed
    virtual void OnNodeSelectionChanged([[maybe_unused]] CUiAnimViewSequence* pSequence) {}

    // Called when selection of keys changed.
    virtual void OnKeySelectionChanged([[maybe_unused]] CUiAnimViewSequence* pSequence) {}

    // Called when keys in a track changed
    virtual void OnKeysChanged([[maybe_unused]] CUiAnimViewSequence* pSequence) {}
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IUiAnimSequence in UiAnimView and contains
// the editor side code for changing it
//
////////////////////////////////////////////////////////////////////////////
class CUiAnimViewSequence
    : public CUiAnimViewAnimNode
    , public IUndoManagerListener
{
    friend class CUiAnimViewSequenceManager;
    friend class CUiAnimViewNode;
    friend class CUiAnimViewAnimNode;
    friend class CUiAnimViewTrack;
    friend class CUiAnimViewSequenceNotificationContext;
    friend class CUiAnimViewSequenceNoNotificationContext;

    // Undo friends
    friend class CAbstractUndoTrackTransaction;
    friend class CAbstractUndoAnimNodeTransaction;
    friend class CUndoAnimNodeReparent;
    friend class CUndoTrackObject;
    friend class CAbstractUndoSequenceTransaction;

public:
    CUiAnimViewSequence(IUiAnimSequence* pSequence);
    ~CUiAnimViewSequence();

    // Called after de-serialization of IUiAnimSequence
    void Load();

    // IUiAnimViewNode
    virtual EUiAnimViewNodeType GetNodeType() const override { return eUiAVNT_Sequence; }

    AZStd::string GetName() const override { return m_pAnimSequence->GetName(); }
    virtual bool SetName(const char* pName) override;
    virtual bool CanBeRenamed() const override { return true; }

    // Binding/Unbinding
    virtual void BindToEditorObjects() override;
    virtual void UnBindFromEditorObjects() override;
    virtual bool IsBoundToEditorObjects() const override;

    // Time range
    void SetTimeRange(Range timeRange);
    Range GetTimeRange() const;

    // Current time in sequence. Note that this can be different from the time
    // of the animation context, if this sequence is used as a sub sequence
    const float GetTime() const;

    // UI Animation system Flags
    void SetFlags(IUiAnimSequence::EUiAnimSequenceFlags flags);
    IUiAnimSequence::EUiAnimSequenceFlags GetFlags() const;

    // Check if this node belongs to a sequence
    bool IsAncestorOf(CUiAnimViewSequence* pSequence) const;

    // Get single selected key if only one key is selected
    CUiAnimViewKeyHandle FindSingleSelectedKey();

    // Get UI Animation system sequence ID
    uint32 GetSequenceId() const { return m_pAnimSequence->GetId(); }

    // Rendering
    virtual void Render(const SUiAnimContext& animContext) override;

    // Playback control
    virtual void Animate(const SUiAnimContext& animContext) override;
    void Resume() { m_pAnimSequence->Resume(); }
    void Pause() { m_pAnimSequence->Pause(); }
    void StillUpdate() { m_pAnimSequence->StillUpdate(); }

    void OnLoop() { m_pAnimSequence->OnLoop(); }

    // Active & deactivate
    void Activate() { m_pAnimSequence->Activate(); }
    void Deactivate() { m_pAnimSequence->Deactivate(); }
    void PrecacheData(const float time) { m_pAnimSequence->PrecacheData(time); }

    // Begin & end cut scene
    void BeginCutScene(const bool bResetFx) const;
    void EndCutScene() const;

    // Reset
    void Reset(const bool bSeekToStart) { m_pAnimSequence->Reset(bSeekToStart); }
    void ResetHard() { m_pAnimSequence->ResetHard(); }

    // Check if it's a group node
    virtual bool IsGroupNode() const override { return true; }

    int GetTrackEventsCount() const { return m_pAnimSequence->GetTrackEventsCount(); }
    const char* GetTrackEvent(int index) { return m_pAnimSequence->GetTrackEvent(index); }
    bool AddTrackEvent(const char* szEvent) { return m_pAnimSequence->AddTrackEvent(szEvent); }
    bool RemoveTrackEvent(const char* szEvent) { return m_pAnimSequence->RemoveTrackEvent(szEvent); }
    bool RenameTrackEvent(const char* szEvent, const char* szNewEvent) { return m_pAnimSequence->RenameTrackEvent(szEvent, szNewEvent); }
    bool MoveUpTrackEvent(const char* szEvent) { return m_pAnimSequence->MoveUpTrackEvent(szEvent); }
    bool MoveDownTrackEvent(const char* szEvent) { return m_pAnimSequence->MoveDownTrackEvent(szEvent); }
    void ClearTrackEvents() { m_pAnimSequence->ClearTrackEvents(); }

    // Deletes all selected nodes (re-parents childs if group node gets deleted)
    void DeleteSelectedNodes();

    // Deletes all selected keys
    void DeleteSelectedKeys();


    // Listeners
    void AddListener(IUiAnimViewSequenceListener* pListener);
    void RemoveListener(IUiAnimViewSequenceListener* pListener);

    // Checks if this is the active sequence in TV
    bool IsActiveSequence() const;

    // The root sequence node is always an active director
    virtual bool IsActiveDirector() const override { return true; }

    // Stores track undo objects for tracks with selected keys
    void StoreUndoForTracksWithSelectedKeys();

    // Copy keys to clipboard (in XML form)
    void CopyKeysToClipboard(const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks);

    // Paste keys from clipboard. Tries to match the given data to the target track first,
    // then the target anim node and finally the whole sequence. If it doesn't find any
    // matching location, nothing will be pasted. Before pasting the given time offset is
    // applied to the keys.
    void PasteKeysFromClipboard(CUiAnimViewAnimNode* pTargetNode, CUiAnimViewTrack* pTargetTrack, const float timeOffset = 0.0f);

    // Returns a vector of pairs that match the XML track nodes in the clipboard to the tracks in the sequence for pasting.
    // It is used by PasteKeysFromClipboard directly and to preview the locations of the to be pasted keys.
    typedef std::pair<CUiAnimViewTrack*, XmlNodeRef> TMatchedTrackLocation;
    std::vector<TMatchedTrackLocation> GetMatchedPasteLocations(XmlNodeRef clipboardContent, CUiAnimViewAnimNode* pTargetNode, CUiAnimViewTrack* pTargetTrack);

    // Adjust the time range
    void AdjustKeysToTimeRange(Range newTimeRange);

    // Clear all key selection
    void DeselectAllKeys();

    // Offset all key selection
    void OffsetSelectedKeys(const float timeOffset);
    // Scale all selected keys by this offset.
    void ScaleSelectedKeys(const float timeOffset);
    //! Push all the keys which come after the first key in time among selected ones by this offset.
    void SlideKeys(const float timeOffset);
    //! Clone all selected keys
    void CloneSelectedKeys();

    // Limit the time offset so as to keep all involved keys in range when offsetting.
    float ClipTimeOffsetForOffsetting(const float timeOffset);
    // Limit the time offset so as to keep all involved keys in range when scaling.
    float ClipTimeOffsetForScaling(const float timeOffset);
    // Limit the time offset so as to keep all involved keys in range when sliding.
    float ClipTimeOffsetForSliding(const float timeOffset);

    // Notifications
    void OnSequenceSettingsChanged();
    void OnKeySelectionChanged();
    void OnKeysChanged();
    void OnNodeSelectionChanged();
    void OnNodeChanged(CUiAnimViewNode* pNode, IUiAnimViewSequenceListener::ENodeChangeType type);
    void OnNodeRenamed(CUiAnimViewNode* pNode, const char* pOldName);

private:
    // These are used to avoid listener notification spam via CUiAnimViewSequenceNotificationContext.
    // For recursion there is a counter that increases on QueueListenerNotifications
    // and decreases on SubmitPendingListenerNotifcations
    // Only when the counter reaches 0 again SubmitPendingListenerNotifcations
    // will submit the notifications
    void QueueNotifications();
    void SubmitPendingNotifcations();

    // Called when an animation updates needs to be schedules
    void ForceAnimation();

    void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

    std::deque<CUiAnimViewTrack*> GetMatchingTracks(CUiAnimViewAnimNode* pAnimNode, XmlNodeRef trackNode);
    void GetMatchedPasteLocationsRec(std::vector<TMatchedTrackLocation>& locations, CUiAnimViewNode* pCurrentNode, XmlNodeRef clipboardNode);

    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;
    void BeginRestoreTransaction() override;
    void EndRestoreTransaction() override;

    // Current time when animated
    float m_time;

    // Stores if sequence is bound
    bool m_bBoundToEditorObjects;

    AZStd::intrusive_ptr<IUiAnimSequence> m_pAnimSequence;
    std::vector<IUiAnimViewSequenceListener*> m_sequenceListeners;

    // Notification queuing
    unsigned int m_selectionRecursionLevel;
    bool m_bNoNotifications;
    bool m_bQueueNotifications;
    bool m_bNodeSelectionChanged;
    bool m_bForceUiAnimation;
    bool m_bKeySelectionChanged;
    bool m_bKeysChanged;
};

////////////////////////////////////////////////////////////////////////////
class CUiAnimViewSequenceNotificationContext
{
public:
    CUiAnimViewSequenceNotificationContext(CUiAnimViewSequence* pSequence)
        : m_pSequence(pSequence)
    {
        if (m_pSequence)
        {
            m_pSequence->QueueNotifications();
        }
    }

    ~CUiAnimViewSequenceNotificationContext()
    {
        if (m_pSequence)
        {
            m_pSequence->SubmitPendingNotifcations();
        }
    }

private:
    CUiAnimViewSequence* m_pSequence;
};

////////////////////////////////////////////////////////////////////////////
class CUiAnimViewSequenceNoNotificationContext
{
public:
    CUiAnimViewSequenceNoNotificationContext(CUiAnimViewSequence* pSequence)
        : m_pSequence(pSequence)
        , m_bNoNotificationsPreviously(false)
    {
        if (m_pSequence)
        {
            m_bNoNotificationsPreviously = m_pSequence->m_bNoNotifications;
            m_pSequence->m_bNoNotifications = true;
        }
    }

    ~CUiAnimViewSequenceNoNotificationContext()
    {
        if (m_pSequence)
        {
            m_pSequence->m_bNoNotifications = m_bNoNotificationsPreviously;
        }
    }

private:
    CUiAnimViewSequence* m_pSequence;

    // Reentrance could happen if there are overlapping sub-sequences controlling
    // the same camera.
    bool m_bNoNotificationsPreviously;
};

