/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include "TrackViewNode.h"
#include "TrackViewTrack.h"

class CTrackViewAnimNode;
class QWidget;

// Represents a bundle of anim nodes
class CTrackViewAnimNodeBundle
{
public:
    unsigned int GetCount() const { return static_cast<unsigned int>(m_animNodes.size()); }
    CTrackViewAnimNode* GetNode(const unsigned int index) { return m_animNodes[index]; }
    const CTrackViewAnimNode* GetNode(const unsigned int index) const { return m_animNodes[index]; }

    void Clear();
    const bool DoesContain(const CTrackViewNode* pTargetNode);

    void AppendAnimNode(CTrackViewAnimNode* pNode);
    void AppendAnimNodeBundle(const CTrackViewAnimNodeBundle& bundle);

    void ExpandAll(bool bAlsoExpandParentNodes = true);
    void CollapseAll();

private:
    AZStd::vector<CTrackViewAnimNode*> m_animNodes;
};

// Callback called by animation node when its animated.
class IAnimNodeAnimator
{
public:
    virtual ~IAnimNodeAnimator() {}

    virtual void Animate(CTrackViewAnimNode* pNode, const SAnimContext& ac) = 0;
    virtual void Render([[maybe_unused]] CTrackViewAnimNode* pNode, [[maybe_unused]] const SAnimContext& ac) {}

    // Called when binding/unbinding the owning node
    virtual void Bind([[maybe_unused]] CTrackViewAnimNode* pNode) {}
    virtual void UnBind([[maybe_unused]] CTrackViewAnimNode* pNode) {}
};

//////////////////////////////////////////////////////////////////////////
//
// This class represents a IAnimNode in TrackView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IAnimNode, therefore deleting it
// will not destroy the CryMovie track
//
//////////////////////////////////////////////////////////////////////////
class CTrackViewAnimNode
    : public CTrackViewNode
    , public IAnimNodeOwner
    , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , public AZ::EntityBus::Handler
    , private AZ::TransformNotificationBus::Handler
    , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
{
public:
    CTrackViewAnimNode(IAnimSequence* pSequence, IAnimNode* pAnimNode, CTrackViewNode* pParentNode);
    ~CTrackViewAnimNode();

    // Rendering
    virtual void Render(const SAnimContext& ac);

    // Playback
    virtual void Animate(const SAnimContext& animContext);

    // Binding/Unbinding
    virtual void BindToEditorObjects();
    virtual void UnBindFromEditorObjects();
    virtual bool IsBoundToEditorObjects() const;

    // Console sync
    virtual void SyncToConsole(SAnimContext& animContext);

    // CTrackViewAnimNode
    ETrackViewNodeType GetNodeType() const override { return eTVNT_AnimNode; }

    // Create & remove sub anim nodes
    virtual CTrackViewAnimNode* CreateSubNode(
        const QString& name, const AnimNodeType animNodeType, AZ::EntityId entityId = AZ::EntityId(),
        AZ::Uuid componentTypeId = AZ::Uuid::CreateNull(), AZ::ComponentId componenId=AZ::InvalidComponentId);
    virtual void RemoveSubNode(CTrackViewAnimNode* pSubNode);

    // Create & remove sub tracks
    virtual CTrackViewTrack* CreateTrack(const CAnimParamType& paramType);
    virtual void RemoveTrack(CTrackViewTrack* pTrack);

    // Add selected entities from scene to group node
    virtual CTrackViewAnimNodeBundle AddSelectedEntities(const AZStd::vector<AnimParamType>& tracks);

    // Add current layer to group node
    virtual void AddCurrentLayer();

    // Director related
    virtual void SetAsActiveDirector();
    virtual bool IsActiveDirector() const;

    // Checks if anim node is part of active sequence and of an active director
    virtual bool IsActive();

    // Name setter/getter
    AZStd::string GetName() const override { return m_animNode->GetName(); }
    bool SetName(const char* pName) override;
    bool CanBeRenamed() const override;

    // Node owner setter/getter
    virtual void SetNodeEntityId(AZ::EntityId entityId);
    virtual AZ::EntityId GetNodeEntityId(const bool bSearch = true);

    AZ::EntityId GetAzEntityId() const { return m_animNode ? m_animNode->GetAzEntityId() : AZ::EntityId(); }
    bool IsBoundToAzEntity() const { return m_animNode ? m_animNode->GetAzEntityId().IsValid(): false; }

    // Snap time value to prev/next key in sequence
    bool SnapTimeToPrevKey(float& time) const override;
    bool SnapTimeToNextKey(float& time) const override;

    // Expanded state interface
    void SetExpanded(bool expanded) override;
    bool GetExpanded() const override;

    // Node getters
    CTrackViewAnimNodeBundle GetAllAnimNodes();
    CTrackViewAnimNodeBundle GetSelectedAnimNodes();
    CTrackViewAnimNodeBundle GetAllOwnedNodes(AZ::EntityId entityId);
    CTrackViewAnimNodeBundle GetAnimNodesByType(AnimNodeType animNodeType);
    CTrackViewAnimNodeBundle GetAnimNodesByName(const char* pName);

    // Track getters
    virtual CTrackViewTrackBundle GetAllTracks();
    virtual CTrackViewTrackBundle GetSelectedTracks();
    virtual CTrackViewTrackBundle GetTracksByParam(const CAnimParamType& paramType) const;

    // Key getters
    CTrackViewKeyBundle GetAllKeys() override;
    CTrackViewKeyBundle GetSelectedKeys() override;
    CTrackViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) override;

    // Type getters
    AnimNodeType GetType() const;

    // Flags
    EAnimNodeFlags GetFlags() const;
    bool AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const;

    // Disabled state
    void SetDisabled(bool bDisabled) override;
    bool IsDisabled() const override;
    bool CanBeEnabled() const override;

    // Return track assigned to the specified parameter.
    CTrackViewTrack* GetTrackForParameter(const CAnimParamType& paramType, uint32 index = 0) const;
    
    // Param
    unsigned int GetParamCount() const;
    CAnimParamType GetParamType(unsigned int index) const;
    AZStd::string GetParamName(const CAnimParamType& paramType) const;
    bool IsParamValid(const CAnimParamType& param) const;
    IAnimNode::ESupportedParamFlags GetParamFlags(const CAnimParamType& paramType) const;
    AnimValueType GetParamValueType(const CAnimParamType& paramType) const;
    void UpdateDynamicParams();

    // Parameter getters/setters
    template <class Type>
    bool SetParamValue(const float time, const CAnimParamType& param, const Type& value)
    {
        AZ_Assert(m_animNode, "Expected valid m_animNode");
        return m_animNode->SetParamValue(time, param, value);
    }

    template <class Type>
    bool GetParamValue(const float time, const CAnimParamType& param, Type& value)
    {
        AZ_Assert(m_animNode, "Expected valid m_animNode");
        return m_animNode->GetParamValue(time, param, value);
    }

    // Check if it's a group node
    bool IsGroupNode() const override;

    // Generate a new node name
    virtual QString GetAvailableNodeNameStartingWith(const QString& name) const;

    // Copy/Paste nodes
    virtual void CopyNodesToClipboard(const bool bOnlySelected, QWidget* context);
    virtual bool PasteNodesFromClipboard(QWidget* context);

    // Set new parent
    virtual void SetNewParent(CTrackViewAnimNode* pNewParent);

    // Check if this node may be moved to new parent
    virtual bool IsValidReparentingTo(CTrackViewAnimNode* pNewParent);

    int GetDefaultKeyTangentFlags() const { return m_animNode ? m_animNode->GetDefaultKeyTangentFlags() : SPLINE_KEY_TANGENT_UNIFIED; }

    void SetComponent(AZ::ComponentId componentId, const AZ::Uuid& componentTypeId);

    // returns the AZ::ComponentId of the component associated with this node if it is of type AnimNodeType::Component, InvalidComponentId otherwise
    AZ::ComponentId GetComponentId() const;

    // IAnimNodeOwner
    void MarkAsModified() override;
    // ~IAnimNodeOwner

    // Compares all of the node's track values at the given time with the associated property value and
    //     sets a key at that time if they are different to match the latter
    // Returns the number of keys set
    int SetKeysForChangedTrackValues(float time) { return m_animNode->SetKeysForChangedTrackValues(time); }

    // returns true if this node is associated with an AnimNodeType::AzEntity node and contains a component with the given id
    bool ContainsComponentWithId(AZ::ComponentId componentId) const;

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus implementation
    void OnStartPlayInEditor() override;
    void OnStopPlayInEditor() override;
    //~AzToolsFramework::EditorEntityContextNotificationBus implementation

    //////////////////////////////////////////////////////////////////////////
    // AZ::EntityBus
    void OnEntityActivated(const AZ::EntityId& entityId) override;
    void OnEntityDestruction(const AZ::EntityId& entityId) override;
    //~AZ::EntityBus

    //////////////////////////////////////////////////////////////////////////
    //! AZ::TransformNotificationBus::Handler
    void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent) override;
    void OnParentTransformWillChange(AZ::Transform oldTransform, AZ::Transform newTransform) override;
    //////////////////////////////////////////////////////////////////////////

    void OnEntityRemoved();

    // Creates a sub-node for the given component. Returns a pointer to the created component sub-node
    CTrackViewAnimNode* AddComponent(const AZ::Component* component, bool disabled);

    // Depth-first search for TrackViewAnimNode associated with the given animNode. Returns the first match found or nullptr if not found
    CTrackViewAnimNode* FindNodeByAnimNode(const IAnimNode* animNode);

protected:
    IAnimNode* GetAnimNode() { return m_animNode.get(); }

private:
    // Copy selected keys to XML representation for clipboard
    void CopyKeysToClipboard(XmlNodeRef& xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

    void CopyNodesToClipboardRec(CTrackViewAnimNode* pCurrentAnimNode, XmlNodeRef& xmlNode, const bool bOnlySelected);

    void PasteTracksFrom(XmlNodeRef& xmlNodeWithTracks);

    CTrackViewTrackBundle GetTracks(const bool bOnlySelected, const CAnimParamType& paramType) const;

    void PasteNodeFromClipboard(AZStd::map<int, IAnimNode*>& copiedIdToNodeMap, XmlNodeRef xmlNode);

    void SetPosRotScaleTracksDefaultValues(bool positionAllowed = true, bool rotationAllowed = true, bool scaleAllowed = true);

    bool CheckTrackAnimated(const CAnimParamType& paramType) const;

    // IAnimNodeOwner
    void OnNodeVisibilityChanged(IAnimNode* pNode, const bool bHidden) override;
    void OnNodeReset(IAnimNode* pNode) override;
    // ~IAnimNodeOwner

    // Helper for Is<Position/Rotation/Scale>Delegated to call internally
    bool IsTransformAnimParamTypeDelegated(AnimParamType animParamType) const;

    // EntitySelectionEvents
    void OnSelected() override;
    void OnDeselected() override;

    void OnSelectionChanged(bool selected);

    void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM);

    // Used to track Editor object listener registration
    void RegisterEditorObjectListeners(AZ::EntityId entityId);
    void UnRegisterEditorObjectListeners();

    // Helper functions
    static void RemoveChildNode(CTrackViewAnimNode* child);
    static AZ::Transform GetEntityWorldTM(AZ::EntityId entityId);
    static void SetParentsInChildren(CTrackViewAnimNode* currentNode);

    IAnimSequence* m_animSequence;
    AZStd::intrusive_ptr<IAnimNode> m_animNode;
    AZ::EntityId m_nodeEntityId;
    AZStd::unique_ptr<IAnimNodeAnimator> m_pNodeAnimator;

    // used to stash the Editor sequence and node entity Ids when we switch to game mode from the editor
    AZ::EntityId    m_stashedAnimNodeEditorAzEntityId;
    AZ::EntityId    m_stashedAnimSequenceEditorAzEntityId;

    // Used to track Editor object listener registration
    AZ::EntityId m_entityIdListenerRegistered;

    // used to return a const reference to a null Uuid
    static const AZ::Uuid s_nullUuid;
};
