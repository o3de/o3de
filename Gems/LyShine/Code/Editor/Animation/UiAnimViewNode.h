/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <LyShine/Animation/IUiAnimation.h>

class CUiAnimViewTrack;
class CUiAnimViewSequence;
class CUiAnimViewAnimNode;

class CUiAnimViewKeyConstHandle
{
public:
    CUiAnimViewKeyConstHandle()
        : m_keyIndex(0)
        , m_pTrack(nullptr) {}

    CUiAnimViewKeyConstHandle(const CUiAnimViewTrack* pTrack, unsigned int keyIndex)
        : m_keyIndex(keyIndex)
        , m_pTrack(pTrack) {}

    void GetKey(IKey* pKey) const;
    float GetTime() const;
    const CUiAnimViewTrack* GetTrack() const { return m_pTrack; }

private:
    unsigned int m_keyIndex;
    const CUiAnimViewTrack* m_pTrack;
};

// Represents one UI Animation system key
class CUiAnimViewKeyHandle
{
public:
    CUiAnimViewKeyHandle()
        : m_bIsValid(false)
        , m_keyIndex(0)
        , m_pTrack(nullptr) {}

    CUiAnimViewKeyHandle(CUiAnimViewTrack* pTrack, unsigned int keyIndex)
        : m_bIsValid(true)
        , m_keyIndex(keyIndex)
        , m_pTrack(pTrack) {}

    void SetKey(IKey* pKey);
    void GetKey(IKey* pKey) const;

    CUiAnimViewTrack* GetTrack() { return m_pTrack; }
    const CUiAnimViewTrack* GetTrack() const { return m_pTrack; }

    bool IsValid() const { return m_bIsValid; }
    unsigned int GetIndex() const { return m_keyIndex; }

    void Select(bool bSelect);
    bool IsSelected() const;

    void SetTime(float time);
    float GetTime() const;

    float GetDuration() const;

    const char* GetDescription() const;

    void Offset(float offset);

    bool operator==(const CUiAnimViewKeyHandle& keyHandle) const;
    bool operator!=(const CUiAnimViewKeyHandle& keyHandle) const;

    // Deletes key. Note that handle will be invalid afterwards
    void Delete();

    CUiAnimViewKeyHandle Clone();

    // Get next/prev/above/below key in expanded node tree
    // Note: Key is assumed to be already visible
    CUiAnimViewKeyHandle GetNextKey();
    CUiAnimViewKeyHandle GetPrevKey();
    CUiAnimViewKeyHandle GetAboveKey() const;
    CUiAnimViewKeyHandle GetBelowKey() const;

private:
    bool m_bIsValid;
    unsigned int m_keyIndex;
    CUiAnimViewTrack* m_pTrack;
};

// Abstract base class that defines common
// operations for key bundles and tracks
class IUiAnimViewKeyBundle
{
public:
    virtual bool AreAllKeysOfSameType() const = 0;

    virtual unsigned int GetKeyCount() const = 0;
    virtual CUiAnimViewKeyHandle GetKey(unsigned int index) = 0;

    virtual void SelectKeys(const bool bSelected) = 0;
};

// Represents a bundle of keys
class CUiAnimViewKeyBundle
    : public IUiAnimViewKeyBundle
{
    friend class CUiAnimViewTrack;
    friend class CUiAnimViewAnimNode;
    friend class CUiAnimViewEventNode;

public:
    CUiAnimViewKeyBundle()
        : m_bAllOfSameType(true) {}
    virtual ~CUiAnimViewKeyBundle() = default;

    virtual bool AreAllKeysOfSameType() const override { return m_bAllOfSameType; }

    virtual unsigned int GetKeyCount() const override { return static_cast<unsigned int>(m_keys.size()); }
    virtual CUiAnimViewKeyHandle GetKey(unsigned int index) override { return m_keys[index]; }

    virtual void SelectKeys(const bool bSelected) override;

    CUiAnimViewKeyHandle GetSingleSelectedKey();

private:
    void AppendKey(const CUiAnimViewKeyHandle& keyHandle);
    void AppendKeyBundle(const CUiAnimViewKeyBundle& bundle);

    bool m_bAllOfSameType;
    std::vector<CUiAnimViewKeyHandle> m_keys;
};

// Types of nodes that derive from CUiAnimViewNode
enum EUiAnimViewNodeType
{
    eUiAVNT_Sequence,
    eUiAVNT_AnimNode,
    eUiAVNT_Track
};

////////////////////////////////////////////////////////////////////////////
//
// This is the base class for all sequences, nodes and tracks in UiAnimView,
// which provides a interface for common operations
//
////////////////////////////////////////////////////////////////////////////
class CUiAnimViewNode
{
    friend class CAbstractUndoTrackTransaction;

public:
    CUiAnimViewNode(CUiAnimViewNode* pParent);
    virtual ~CUiAnimViewNode() {}

    // Name
    virtual AZStd::string GetName() const = 0;
    virtual bool SetName([[maybe_unused]] const char* pName) { return false; };
    virtual bool CanBeRenamed() const { return false; }

    // UI Animation system node type
    virtual EUiAnimViewNodeType GetNodeType() const = 0;

    // Get the sequence of this node
    CUiAnimViewSequence* GetSequence();

    // Get parent
    CUiAnimViewNode* GetParentNode() const { return m_pParentNode; }

    // Children
    unsigned int GetChildCount() const { return static_cast<unsigned int>(m_childNodes.size()); }
    CUiAnimViewNode* GetChild(unsigned int index) const { return m_childNodes[index].get(); }

    // Snap time value to prev/next key in sequence
    virtual bool SnapTimeToPrevKey(float& time) const = 0;
    virtual bool SnapTimeToNextKey(float& time) const = 0;

    // Selection state
    virtual void SetSelected(bool bSelected);
    virtual bool IsSelected() const { return m_bSelected; }

    // Clear selection of this node and all sub nodes
    void ClearSelection();

    // Expanded state
    virtual void SetExpanded(bool bExpanded);
    virtual bool IsExpanded() const { return m_bExpanded; }

    // Disabled state
    virtual void SetDisabled([[maybe_unused]] bool bDisabled) {}
    virtual bool IsDisabled() const { return false; }

    // Hidden state
    void SetHidden(bool bHidden);
    bool IsHidden() const;

    // Key getters
    virtual CUiAnimViewKeyBundle GetSelectedKeys() = 0;
    virtual CUiAnimViewKeyBundle GetAllKeys() = 0;
    virtual CUiAnimViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) = 0;

    // Check if node itself is obsolete, or any child is an obsolete track
    bool HasObsoleteTrack() const;

    // Get above/below nodes in pCurrentNode node tree
    CUiAnimViewNode* GetAboveNode() const;
    CUiAnimViewNode* GetBelowNode() const;

    // Get previous or next sibling of this node
    CUiAnimViewNode* GetPrevSibling() const;
    CUiAnimViewNode* GetNextSibling() const;

    // Check if it's a group node
    virtual bool IsGroupNode() const { return false; }

    // Copy selected keys to XML representation for clipboard
    virtual void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) = 0;

    // Sorting
    bool operator<(const CUiAnimViewNode& pOtherNode) const;

    // Get first selected node in tree
    CUiAnimViewNode* GetFirstSelectedNode();

    // Get director of this node
    CUiAnimViewAnimNode* GetDirector();

protected:
    void AddNode(CUiAnimViewNode* pNode);
    void SortNodes();

    bool HasObsoleteTrackRec(const CUiAnimViewNode* pCurrentNode) const;

    CUiAnimViewNode* m_pParentNode;
    std::vector<std::unique_ptr<CUiAnimViewNode> > m_childNodes;

    bool m_bSelected;
    bool m_bExpanded;
    bool m_bHidden;
};
