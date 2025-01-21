/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

class CTrackViewAnimNode;
class CTrackViewSequence;
class CTrackViewTrack;
struct IKey;

#include <AzCore/std/containers/vector.h>

class CTrackViewKeyConstHandle
{
public:
    CTrackViewKeyConstHandle()
        : m_keyIndex(0)
        , m_pTrack(nullptr) {}

    CTrackViewKeyConstHandle(const CTrackViewTrack* pTrack, unsigned int keyIndex)
        : m_keyIndex(keyIndex)
        , m_pTrack(pTrack) {}

    void GetKey(IKey* pKey) const;
    float GetTime() const;
    const CTrackViewTrack* GetTrack() const { return m_pTrack; }

private:
    unsigned int m_keyIndex;
    const CTrackViewTrack* m_pTrack;
};

// Represents one CryMovie key
class CTrackViewKeyHandle
{
public:
    CTrackViewKeyHandle()
        : m_bIsValid(false)
        , m_keyIndex(0)
        , m_pTrack(nullptr) {}

    CTrackViewKeyHandle(CTrackViewTrack* pTrack, unsigned int keyIndex)
        : m_bIsValid(true)
        , m_keyIndex(keyIndex)
        , m_pTrack(pTrack) {}

    void SetKey(IKey* pKey);
    void GetKey(IKey* pKey) const;

    CTrackViewTrack* GetTrack() { return m_pTrack; }
    const CTrackViewTrack* GetTrack() const { return m_pTrack; }

    bool IsValid() const { return m_bIsValid; }
    unsigned int GetIndex() const { return m_keyIndex; }

    void Select(bool bSelect);
    bool IsSelected() const;

    void SetTime(float time, bool notifyListeners = true);
    float GetTime() const;

    float GetDuration() const;

    const char* GetDescription() const;

    void Offset(float offset, bool notifyListeners);

    bool operator==(const CTrackViewKeyHandle& keyHandle) const;
    bool operator!=(const CTrackViewKeyHandle& keyHandle) const;

    // Deletes key. Note that handle will be invalid afterwards
    void Delete();

    CTrackViewKeyHandle Clone();

    // Get next/prev/above/below key in expanded node tree
    // Note: Key is assumed to be already visible
    CTrackViewKeyHandle GetNextKey();
    CTrackViewKeyHandle GetPrevKey();
    CTrackViewKeyHandle GetAboveKey() const;
    CTrackViewKeyHandle GetBelowKey() const;

private:
    bool m_bIsValid;
    unsigned int m_keyIndex;
    CTrackViewTrack* m_pTrack;
};

// Abstract base class that defines common
// operations for key bundles and tracks

// Represents a bundle of keys
class CTrackViewKeyBundle
{
    friend class CTrackViewTrack;
    friend class CTrackViewAnimNode;

public:
    CTrackViewKeyBundle()
        : m_bAllOfSameType(true) {}
    virtual ~CTrackViewKeyBundle() = default;

    bool AreAllKeysOfSameType() const { return m_bAllOfSameType; }

    unsigned int GetKeyCount() const { return static_cast<unsigned int>(m_keys.size()); }
    CTrackViewKeyHandle GetKey(unsigned int index) const { return m_keys[index]; }

    void SelectKeys(const bool bSelected);

    CTrackViewKeyHandle GetSingleSelectedKey();

private:
    void AppendKey(const CTrackViewKeyHandle& keyHandle);
    void AppendKeyBundle(const CTrackViewKeyBundle& bundle);

    bool m_bAllOfSameType;
    AZStd::vector<CTrackViewKeyHandle> m_keys;
};

// Types of nodes that derive from CTrackViewNode
enum ETrackViewNodeType
{
    eTVNT_Sequence,
    eTVNT_AnimNode,
    eTVNT_Track
};

//////////////////////////////////////////////////////////////////////////
//
// This is the base class for all sequences, nodes and tracks in TrackView,
// which provides a interface for common operations
//
//////////////////////////////////////////////////////////////////////////
class CTrackViewNode
{
public:
    CTrackViewNode(CTrackViewNode* pParent);
    virtual ~CTrackViewNode() {}

    // Name
    virtual AZStd::string GetName() const = 0;
    virtual bool SetName([[maybe_unused]] const char* pName) { return false; }
    virtual bool CanBeRenamed() const { return false; }

    // CryMovie node type
    virtual ETrackViewNodeType GetNodeType() const = 0;

    // Get the sequence of this node
    CTrackViewSequence* GetSequence();
    const CTrackViewSequence* GetSequenceConst() const;

    // Get parent
    CTrackViewNode* GetParentNode() const { return m_pParentNode; }

    // Children
    unsigned int GetChildCount() const { return static_cast<unsigned int>(m_childNodes.size()); }
    CTrackViewNode* GetChild(unsigned int index) const { return m_childNodes[index].get(); }

    // Snap time value to prev/next key in sequence
    virtual bool SnapTimeToPrevKey(float& time) const = 0;
    virtual bool SnapTimeToNextKey(float& time) const = 0;

    // Selection state
    virtual void SetSelected(bool bSelected);
    virtual bool IsSelected() const { return m_bSelected; }

    // Clear selection of this node and all sub nodes
    void ClearSelection();

    // Expanded state interface
    virtual void SetExpanded(bool expanded) = 0;
    virtual bool GetExpanded() const = 0;

    // Disabled state
    virtual bool CanBeEnabled() const { return true; }
    virtual void SetDisabled([[maybe_unused]] bool bDisabled) {}
    virtual bool IsDisabled() const { return false; }

    // Hidden state
    void SetHidden(bool bHidden);
    bool IsHidden() const;

    // Key getters
    virtual CTrackViewKeyBundle GetSelectedKeys() = 0;
    virtual CTrackViewKeyBundle GetAllKeys() = 0;
    virtual CTrackViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) = 0;

    // Check if node itself is obsolete, or any child is an obsolete track
    bool HasObsoleteTrack() const;

    // Get above/below nodes in pCurrentNode node tree
    CTrackViewNode* GetAboveNode() const;
    CTrackViewNode* GetBelowNode() const;

    // Get previous or next sibling of this node
    CTrackViewNode* GetPrevSibling() const;
    CTrackViewNode* GetNextSibling() const;

    // Check if it's a group node
    virtual bool IsGroupNode() const { return false; }

    // Copy selected keys to XML representation for clipboard
    virtual void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) = 0;

    // Sorting
    bool operator<(const CTrackViewNode& pOtherNode) const;

    // Get first selected node in tree
    CTrackViewNode* GetFirstSelectedNode();

    // Get director of this node
    CTrackViewAnimNode* GetDirector();

protected:
    void AddNode(CTrackViewNode* pNode);
    void SortNodes();

    bool HasObsoleteTrackRec(const CTrackViewNode* pCurrentNode) const;

    CTrackViewNode* m_pParentNode;
    AZStd::vector<AZStd::unique_ptr<CTrackViewNode> > m_childNodes;

    bool m_bSelected;
    bool m_bHidden;
};
